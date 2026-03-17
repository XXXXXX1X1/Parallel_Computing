#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <thread>

#ifndef N
#define N 20000
#endif

// Результаты одного прогона: время инициализации, время вычисления, контрольная сумма
struct RunTimes {
    double init_s;
    double work_s;
    double res;
};

// Проверка наличия флага "--bench"
static bool has_flag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

template<int NVAL>
static RunTimes run_once(int num_threads) {
    double *A = new double[(long long)NVAL * NVAL];   // матрица A в линейной памяти
    double *x = new double[NVAL];                     // вектор x
    double *y = new double[NVAL];                     // вектор y

    auto start1 = std::chrono::steady_clock::now();   // старт замера инициализации

    std::cout << "threads=" << num_threads << std::endl;

    // Параллельная инициализация A, x, y
    std::vector<std::thread> threads;

    for (int thread_id = 0; thread_id < num_threads; thread_id++) {
        threads.emplace_back([A, x, y, thread_id, num_threads]() {
            // ===== ИНИЦИАЛИЗАЦИЯ A =====
            long long totalA = (long long)NVAL * NVAL;
            long long sizeA  = totalA / num_threads;
            long long startA = (long long)thread_id * sizeA;
            long long endA   = (thread_id == num_threads - 1)
                             ? totalA
                             : (long long)(thread_id + 1) * sizeA;

            for (long long i = startA; i < endA; i++) {
                A[i] = 1.0;
            }

            // ===== ИНИЦИАЛИЗАЦИЯ x =====
            long long sizeX  = NVAL / num_threads;
            long long startX = (long long)thread_id * sizeX;
            long long endX   = (thread_id == num_threads - 1)
                             ? NVAL
                             : (long long)(thread_id + 1) * sizeX;

            for (long long i = startX; i < endX; i++) {
                x[i] = 1.0;
            }

            // ===== ИНИЦИАЛИЗАЦИЯ y =====
            long long sizeY  = NVAL / num_threads;
            long long startY = (long long)thread_id * sizeY;
            long long endY   = (thread_id == num_threads - 1)
                             ? NVAL
                             : (long long)(thread_id + 1) * sizeY;

            for (long long i = startY; i < endY; i++) {
                y[i] = 0.0;
            }
        });
    }

    for (int i = 0; i < (int)threads.size(); i++) {
        threads[i].join();
    }

    auto end1 = std::chrono::steady_clock::now();   // конец замера инициализации

    // Параллельное умножение матрицы на вектор
    threads.clear();

    for (int thread_id = 0; thread_id < num_threads; thread_id++) {
        threads.emplace_back([A, x, y, thread_id, num_threads]() {
            long long rows_per_thread = NVAL / num_threads;
            long long start = (long long)thread_id * rows_per_thread;
            long long end   = (thread_id == num_threads - 1)
                            ? NVAL
                            : (long long)(thread_id + 1) * rows_per_thread;

            for (long long i = start; i < end; i++) {
                double sum = 0.0;
                for (long long j = 0; j < NVAL; j++) {
                    sum += A[i * (long long)NVAL + j] * x[j];
                }
                y[i] = sum;
            }
        });
    }

    for (int i = 0; i < (int)threads.size(); i++) {
        threads[i].join();
    }

    auto end2 = std::chrono::steady_clock::now();   // конец замера вычисления

    double res = 0.0;
    for (long long i = 0; i < NVAL; i++) {
        res += y[i];
    }

    const std::chrono::duration<double> elapsed_seconds1{end1 - start1};
    const std::chrono::duration<double> elapsed_seconds2{end2 - end1};

    std::cout << "N:" << NVAL << std::endl;
    std::cout << "Init time " << elapsed_seconds1.count() << std::endl;
    std::cout << "Work time " << elapsed_seconds2.count() << std::endl;
    std::cout << res << std::endl;

    delete[] A;
    delete[] x;
    delete[] y;

    return { elapsed_seconds1.count(), elapsed_seconds2.count(), res };
}

int main(int argc, char** argv) {
    // Обычный режим: один прогон (без флага --bench)
    if (!has_flag(argc, argv, "--bench")) {
        int t = (int)std::thread::hardware_concurrency();
        if (t <= 0) t = 4;
        (void)run_once<N>(t);
        return 0;
    }

    std::vector<int> Ts = {1, 2, 4, 7, 8, 16, 20, 40};

    std::ofstream f("results.csv", std::ios::out | std::ios::trunc);
    if (!f.is_open()) {
        std::cerr << "ERROR: cannot open results.csv" << std::endl;
        return 2;
    }

    f << "N,threads,work_time_s,status\n";

    auto bench_one = [&](int n_tag, auto runner) {
        for (int t : Ts) {
            RunTimes r = runner(t);
            f << n_tag << "," << t << "," << r.work_s << ",OK\n";
        }
    };

    bench_one(20000, [](int t) { return run_once<20000>(t); });
    bench_one(40000, [](int t) { return run_once<40000>(t); });

    std::cout << "Saved CSV: results.csv" << std::endl;
    return 0;
}