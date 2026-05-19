import threading
import time
import random


# -----------------------------
# Базовый класс датчика
# -----------------------------

class Sensor:
    def get(self):
        # Общий метод для всех датчиков.
        raise NotImplementedError


# -----------------------------
# Датчик SensorX
# -----------------------------

class SensorX(Sensor):
    def __init__(self, period, name="SensorX"):
        # period — период обновления значения датчика в секундах
        # name — имя датчика
        self._period = period
        self._name = name

        # Последнее значение датчика
        self._value = None

        # Событие для остановки внутреннего потока
        self._stop_event = threading.Event()

        # Внутренний поток датчика
        self._thread = None

    def _run(self):
        # Основной цикл работы датчика.
        # Пока датчик не остановлен, генерируем новое значение.
        while not self._stop_event.is_set():
            self._value = random.randint(0, 100)

            # Ждём заданный период перед следующим обновлением
            time.sleep(self._period)

    def start(self):
        # Сбрасываем флаг остановки
        self._stop_event.clear()

        # Создаём и запускаем внутренний поток датчика
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self):
        # Сообщаем внутреннему потоку, что нужно остановиться
        self._stop_event.set()

        # Ждём завершения потока
        if self._thread is not None:
            self._thread.join()

    def get(self):
        # Возвращаем последнее значение датчика
        return self._value