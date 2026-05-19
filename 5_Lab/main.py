import argparse
import os
import time
import threading
from concurrent.futures import ThreadPoolExecutor

import cv2

MODEL_PATH = "yolov8s-pose.pt"

RESULTS_DIR = "results"

thread_data = threading.local()


# -----------------------------
# Работа с видеофайлом
# -----------------------------

class VideoFile:
    def __init__(self, input_path, output_path):
        # Открываем входное видео
        self.cap = cv2.VideoCapture(input_path)

        # Если видео не открылось — завершаем программу с ошибкой
        if not self.cap.isOpened():
            raise RuntimeError("Не удалось открыть видео")

        # Получаем ширину и высоту видео
        self.width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        self.height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

        # Получаем FPS входного видео
        self.fps = self.cap.get(cv2.CAP_PROP_FPS)

        # Если FPS не определился, ставим 30
        if self.fps <= 0:
            self.fps = 30

        # Кодек для записи mp4-файла
        fourcc = cv2.VideoWriter_fourcc(*"mp4v")

        # Создаём объект для записи выходного видео
        self.writer = cv2.VideoWriter(
            output_path,
            fourcc,
            self.fps,
            (self.width, self.height),
        )

        # Если выходной файл создать не удалось — ошибка
        if not self.writer.isOpened():
            raise RuntimeError("Не удалось создать выходное видео")

    def read_frames(self):
        # Список для всех кадров видео
        frames = []

        # Читаем кадры, пока видео не закончится
        while True:
            ok, frame = self.cap.read()

            # Если кадр не считался — значит видео закончилось
            if not ok:
                break

            # Добавляем кадр в список
            frames.append(frame)

        return frames

    def write_frame(self, frame):
        # Записываем один обработанный кадр в выходное видео
        self.writer.write(frame)

    def close(self):
        # Освобождаем входное видео
        if self.cap is not None:
            self.cap.release()
            self.cap = None

        # Освобождаем выходное видео
        if self.writer is not None:
            self.writer.release()
            self.writer = None

    def __del__(self):
        # Деструктор: освобождает ресурсы при удалении объекта
        self.close()

    def __enter__(self):
        # Нужно для использования конструкции with
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        # При выходе из with автоматически закрываем видео
        self.close()


# -----------------------------
# Модель YOLO
# -----------------------------

def load_model():
    from ultralytics import YOLO
    return YOLO(MODEL_PATH)


def process_frame(model, frame):
    # Обрабатываем один кадр 
    result = model.predict(frame, device="cpu", verbose=False)[0]

    # Возвращаем кадр с нарисованными KeyPoints
    return result.plot()


# -----------------------------
# Модель для каждого потока
# -----------------------------

def get_model_for_thread():
    # Проверяем, есть ли у текущего потока своя модель
    if not hasattr(thread_data, "model"):
        thread_data.model = load_model()

    # Возвращаем модель текущего потока
    return thread_data.model


def process_frame_multi(frame):
    model = get_model_for_thread()
    return process_frame(model, frame)


# -----------------------------
# Однопоточная обработка
# -----------------------------

def run_single(video, frames):
    model = load_model()
    for frame in frames:
        result_frame = process_frame(model, frame)
        video.write_frame(result_frame)


# -----------------------------
# Многопоточная обработка
# -----------------------------

def run_multi(video, frames, workers):
    with ThreadPoolExecutor(max_workers=workers) as executor:
        # Раздаём кадры потокам.
        results = executor.map(process_frame_multi, frames)

        # Записываем обработанные кадры в видео
        for result_frame in results:
            video.write_frame(result_frame)


# -----------------------------
# Аргументы командной строки
# -----------------------------

def parse_args():
    parser = argparse.ArgumentParser()

    # Путь к входному видео
    parser.add_argument("video")

    # Режим работы: один поток или несколько потоков
    parser.add_argument("mode", choices=["single", "multi"])

    # Имя выходного видеофайла
    parser.add_argument("output")

    # Количество потоков.
    # Необязательный аргумент, по умолчанию 2.
    parser.add_argument("workers", nargs="?", type=int, default=2)

    return parser.parse_args()


# -----------------------------
# Основная программа
# -----------------------------

def main():
    # Считываем аргументы запуска
    args = parse_args()

    # Количество потоков должно быть положительным
    if args.workers < 1:
        raise RuntimeError("Количество потоков должно быть больше 0")

    # Создаём папку для результатов, если её нет
    os.makedirs(RESULTS_DIR, exist_ok=True)

    # Формируем путь к выходному файлу
    output_path = os.path.join(RESULTS_DIR, args.output)

    # Открываем входное видео и создаём выходное
    with VideoFile(args.video, output_path) as video:
        # Считываем все кадры видео в память
        frames = video.read_frames()

        # Выводим информацию о видео
        print("Видео:", args.video)
        print("Кадров:", len(frames))
        print("Разрешение:", video.width, "x", video.height)
        print("Режим:", args.mode)

        # Если режим многопоточный, выводим число потоков
        if args.mode == "multi":
            print("Потоков:", args.workers)

        # Запускаем замер времени
        start = time.perf_counter()

        # Выбираем режим обработки
        if args.mode == "single":
            run_single(video, frames)
        else:
            run_multi(video, frames, args.workers)

        # Останавливаем замер времени
        end = time.perf_counter()

    # Выводим итоговые результаты
    print("Время обработки:", round(end - start, 2), "сек")
    print("Выходной файл:", output_path)


# -----------------------------
# Точка входа
# -----------------------------

if __name__ == "__main__":
    main()