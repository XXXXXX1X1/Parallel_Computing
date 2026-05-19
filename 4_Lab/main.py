import argparse
import logging
import os
import queue
import threading
import time

import cv2
import numpy as np

from sensor_x import Sensor, SensorX


# -----------------------------
# Логирование
# -----------------------------

def setup_logging():
    # Создаём папку для логов, если её нет
    os.makedirs("log", exist_ok=True)

    # Настраиваем запись ошибок и сообщений в файл log/app.log
    logging.basicConfig(
        filename="log/app.log",
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        encoding="utf-8",
    )


# -----------------------------
# Камера
# -----------------------------

class SensorCam(Sensor):
    def __init__(self, camera_name, resolution):
        # Сохраняем имя камеры и нужное разрешение
        self.camera_name = camera_name
        self.width, self.height = resolution
        self.cam = None

        # Если передано 0, 1 ... — используем как номер камеры
        # Если передан путь  — используем как строку
        source = int(camera_name) if str(camera_name).isdigit() else camera_name

        # Открываем камеру 
        self.cam = cv2.VideoCapture(source)

        # Если камера не открылась
        if not self.cam.isOpened():
            logging.error("Камера не открылась: %s", camera_name)
            raise RuntimeError("Камера не открылась")

        # Устанавливаем желаемое разрешение
        self.cam.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
        self.cam.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)

    def get(self):
        # Считываем один кадр с камеры
        ok, frame = self.cam.read()

        # Если кадр не считался — пишем ошибку
        if not ok or frame is None:
            logging.error("Ошибка чтения кадра с камеры")
            return None

        return frame

    def close(self):
        # Освобождаем камеру
        if self.cam is not None:
            self.cam.release()
            self.cam = None

    def __del__(self):
        # Деструктор нужен, чтобы камера закрылась при удалении объекта
        self.close()


# -----------------------------
# Окно
# -----------------------------

class WindowImage:
    def __init__(self, fps, name="Sensors"):
        self.name = name

        # Задержка между кадрами в миллисекундах
        self.delay = max(1, int(1000 / fps))

        try:
            # Создаём окно OpenCV
            cv2.namedWindow(self.name, cv2.WINDOW_NORMAL)
        except Exception as e:
            logging.error("Ошибка создания окна: %s", e)
            raise

    def show(self, img):
        try:
            # Показываем изображение в окне
            cv2.imshow(self.name, img)

            # waitKey нужен и для задержки, и для считывания клавиши
            return cv2.waitKey(self.delay) & 0xFF
        except Exception as e:
            logging.error("Ошибка вывода изображения: %s", e)
            raise

    def close(self):
        # Закрываем окно
        try:
            cv2.destroyWindow(self.name)
        except Exception:
            pass

    def __del__(self):
        self.close()


# -----------------------------
# Работа с очередью
# -----------------------------

def put_latest(q, value):

    # Если очередь заполнена, удаляем старое значение
    if q.full():
        try:
            q.get_nowait()
        except queue.Empty:
            pass

    # Кладём новое значение
    try:
        q.put_nowait(value)
    except queue.Full:
        pass


def get_latest(q, old_value):
    
    # Если новые данные есть — берём их.
    # Если новых данных нет — используем старое значение.
    

    try:
        return q.get_nowait()
    except queue.Empty:
        return old_value


# -----------------------------
# Поток камеры
# -----------------------------

def camera_thread(cam, q, stop_event):
    # Постоянно читаем кадры с камеры
    while not stop_event.is_set():
        frame = cam.get()

        # Если камера перестала отдавать кадры — завершаем программу
        if frame is None:
            stop_event.set()
            break

        # Кладём в очередь только последний кадр
        put_latest(q, frame)


# -----------------------------
# Поток SensorX
# -----------------------------

def sensor_x_thread(sensor, q, stop_event):
    # Запускаем сам датчик SensorX
    sensor.start()

    try:
        # Постоянно читаем последнее значение датчика
        while not stop_event.is_set():
            value = sensor.get()

            # Если значение есть — кладём его в очередь
            if value is not None:
                put_latest(q, value)

            #пауза, чтобы поток не грузил процессор на 100%
            time.sleep(0.001)

    finally:
        # При завершении останавливаем датчик
        sensor.stop()


# -----------------------------
# Формирование изображения
# -----------------------------

def make_image(frame, values, width, height):
    # Если кадра с камеры нет — создаём чёрную картинку
    if frame is None:
        img = np.zeros((height, width, 3), dtype=np.uint8)

        cv2.putText(
            img,
            "No camera signal",
            (40, height // 2),
            cv2.FONT_HERSHEY_SIMPLEX,
            1,
            (0, 0, 255),
            2,
        )
    else:
        # Подгоняем кадр камеры под нужное разрешение
        img = cv2.resize(frame, (width, height))

    # Рисуем чёрный прямоугольник под текст
    cv2.rectangle(img, (20, 20), (460, 190), (0, 0, 0), -1)

    # Выводим значение датчика 100 Hz
    cv2.putText(
        img,
        f"Sensor 100 Hz: {values['100Hz']}",
        (40, 70),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 255, 0),
        2,
    )

    # Выводим значение датчика 10 Hz
    cv2.putText(
        img,
        f"Sensor 10 Hz : {values['10Hz']}",
        (40, 120),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 255, 0),
        2,
    )

    # Выводим значение датчика 1 Hz
    cv2.putText(
        img,
        f"Sensor 1 Hz  : {values['1Hz']}",
        (40, 170),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 255, 0),
        2,
    )

    # Подсказка для выхода
    cv2.putText(
        img,
        "q - exit",
        (40, 230),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255, 255, 255),
        2,
    )

    return img


# -----------------------------
# Аргументы командной строки
# -----------------------------

def parse_resolution(text):
    # Парсим строку вида 1280x720
    try:
        w, h = text.lower().split("x")
        return int(w), int(h)
    except Exception:
        raise argparse.ArgumentTypeError("Формат разрешения: 1280x720")


def parse_args():
    parser = argparse.ArgumentParser()

    # Имя или номер камеры
    parser.add_argument(
        "--camera",
        default="0",
        help="Камера: 0, 1 или /dev/video0",
    )

    # Разрешение камеры
    parser.add_argument(
        "--resolution",
        type=parse_resolution,
        default="1280x720",
        help="Разрешение камеры, например 1280x720",
    )

    # Частота обновления окна
    parser.add_argument(
        "--display-hz",
        type=float,
        default=30.0,
        help="Частота отображения картинки",
    )

    return parser.parse_args()


# -----------------------------
# Основная программа
# -----------------------------

def main():
    # Настраиваем логирование и читаем аргументы
    setup_logging()
    args = parse_args()

    width, height = args.resolution

    # Событие для безопасной остановки всех потоков
    stop_event = threading.Event()

    cam = None
    window = None

    try:
        # Создаём камеру и окно
        cam = SensorCam(args.camera, (width, height))
        window = WindowImage(args.display_hz)

        # Очереди для обмена данными между потоками
        q_cam = queue.Queue(maxsize=1)
        q_100 = queue.Queue(maxsize=1)
        q_10 = queue.Queue(maxsize=1)
        q_1 = queue.Queue(maxsize=1)

        # Создаём три датчика с разной частотой
        sensor_100 = SensorX(0.01, "Sensor_100Hz")  # 100 Hz
        sensor_10 = SensorX(0.1, "Sensor_10Hz")     # 10 Hz
        sensor_1 = SensorX(1.0, "Sensor_1Hz")       # 1 Hz

        # Создаём потоки: камера + три датчика
        threads = [
            threading.Thread(
                target=camera_thread,
                args=(cam, q_cam, stop_event),
            ),
            threading.Thread(
                target=sensor_x_thread,
                args=(sensor_100, q_100, stop_event),
            ),
            threading.Thread(
                target=sensor_x_thread,
                args=(sensor_10, q_10, stop_event),
            ),
            threading.Thread(
                target=sensor_x_thread,
                args=(sensor_1, q_1, stop_event),
            ),
        ]

        # Запускаем все потоки
        for t in threads:
            t.start()

        # Последний полученный кадр
        last_frame = None

        # Последние значения датчиков
        values = {
            "100Hz": None,
            "10Hz": None,
            "1Hz": None,
        }

        # Главный цикл программы
        while not stop_event.is_set():
            # Получаем последний кадр с камеры
            last_frame = get_latest(q_cam, last_frame)

            # Получаем последние значения датчиков
            values["100Hz"] = get_latest(q_100, values["100Hz"])
            values["10Hz"] = get_latest(q_10, values["10Hz"])
            values["1Hz"] = get_latest(q_1, values["1Hz"])

            # Формируем итоговую картинку
            img = make_image(last_frame, values, width, height)

            # Показываем картинку и проверяем нажатую клавишу
            key = window.show(img)

            # Если нажали q — завершаем программу
            if key == ord("q"):
                stop_event.set()

        # Ждём завершения потоков
        for t in threads:
            t.join(timeout=1)

    except Exception as e:
        # Любую критическую ошибку пишем в лог и выводим в терминал
        logging.error("Ошибка программы: %s", e)
        print("Ошибка:", e)

    finally:
        # В любом случае останавливаем потоки и освобождаем ресурсы
        stop_event.set()

        if cam is not None:
            cam.close()

        if window is not None:
            window.close()

        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()