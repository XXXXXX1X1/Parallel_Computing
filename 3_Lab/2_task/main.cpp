#include <iostream>
#include <fstream>
#include <thread>
#include <random>
#include <cmath>
#include <iomanip>
#include <string>
#include <filesystem>

#include "server.hpp"

// Функция для вычисления sin(x)
double f_sin(double x)
{
    return std::sin(x);
}

// Функция для вычисления sqrt(x)
double f_sqrt(double x)
{
    return std::sqrt(x);
}

// Функция для вычисления x^y
double f_pow(double x, double y)
{
    return std::pow(x, y);
}

// Клиент, который генерирует n задач на вычисление sin(x)
// и сохраняет результаты в указанный файл
void client_sin(TaskServer<double>& server, int n, const std::string& filename)
{
    std::ofstream out(filename);
    if (!out)
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    // Генератор случайных чисел с фиксированным seed
    std::mt19937 gen(12345);
    std::uniform_real_distribution<double> dist(-100.0, 100.0);

    // 12 знаков после запятой
    out << std::fixed << std::setprecision(12);

    for (int i = 0; i < n; i++)
    {
        double x = dist(gen);

        // Добавляем задачу на сервер и получаем её id
        size_t id = server.add_task([x]() {
            return f_sin(x);
        });

        // Запрашиваем результат по id
        double result = server.request_result(id);

        // Записываем входные данные и результат в файл
        out << "type=sin x=" << x << " result=" << result << '\n';
    }
}

// Клиент, который генерирует n задач на вычисление sqrt(x)
// и сохраняет результаты в указанный файл
void client_sqrt(TaskServer<double>& server, int n, const std::string& filename)
{
    std::ofstream out(filename);
    if (!out)
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    // Генератор случайных чисел с фиксированным seed
    std::mt19937 gen(54321);
    std::uniform_real_distribution<double> dist(0.0, 10000.0);

    out << std::fixed << std::setprecision(12);

    for (int i = 0; i < n; i++)
    {
        double x = dist(gen);

        // Отправляем задачу на вычисление квадратного корня
        size_t id = server.add_task([x]() {
            return f_sqrt(x);
        });

        // Получаем готовый результат
        double result = server.request_result(id);

        // Пишем результат в файл
        out << "type=sqrt x=" << x << " result=" << result << '\n';
    }
}

// Клиент, который генерирует n задач на вычисление x^y
// и сохраняет результаты в указанный файл
void client_pow(TaskServer<double>& server, int n, const std::string& filename)
{
    std::ofstream out(filename);
    if (!out)
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    // Отдельный генератор случайных чисел для основания и степени
    std::mt19937 gen(77777);
    std::uniform_real_distribution<double> dist_x(0.1, 20.0);
    std::uniform_real_distribution<double> dist_y(0.1, 20.0);

    out << std::fixed << std::setprecision(12);

    for (int i = 0; i < n; i++)
    {
        double x = dist_x(gen);
        double y = dist_y(gen);

        // Добавляем задачу на вычисление степени x^y
        size_t id = server.add_task([x, y]() {
            return f_pow(x, y);
        });

        // Получаем результат выполнения задачи
        double result = server.request_result(id);

        // Сохраняем параметры и результат
        out << "type=pow x=" << x << " y=" << y << " result=" << result << '\n';
    }
}

int main()
{
    // Количество задач для каждого клиента
    int n = 100;

    // Папка, куда будут складываться результаты
    std::filesystem::path result_dir = "result";
    std::filesystem::create_directories(result_dir);

    // Создаём сервер задач и запускаем рабочий поток
    TaskServer<double> server;
    server.start();

    // Поток клиента для sin
    std::thread t1(
        client_sin,
        std::ref(server),
        n,
        (result_dir / "client_sin.txt").string()
    );

    // Поток клиента для sqrt
    std::thread t2(
        client_sqrt,
        std::ref(server),
        n,
        (result_dir / "client_sqrt.txt").string()
    );

    // Поток клиента для pow
    std::thread t3(
        client_pow,
        std::ref(server),
        n,
        (result_dir / "client_pow.txt").string()
    );

    // Ждём завершения всех клиентов
    t1.join();
    t2.join();
    t3.join();

    // Останавливаем сервер после завершения работы
    server.stop();

    return 0;
}