#include <omp.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <math.h>
#include <vector>
#include <string>
#include <cstring>

#ifndef NSTEPS
#define NSTEPS 40000000LL
#endif

// Результаты одного прогона: время вычисления, значение интеграла
struct RunTimes {
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

// Функция под интеграл 
static inline double f(double x) {
    return sin(x);
}


static double integrate_omp(long long nsteps, double a, double b) {
    double h = (b - a) / (double)nsteps;
    double sum = 0.0;

    #pragma omp parallel
    {
        // локальная сумма каждого потока
        double local = 0.0;

        int num_threads = omp_get_num_threads();
        int thread_id   = omp_get_thread_num();

        // ручное разбиение итераций по потокам 
        long long chunk = nsteps / num_threads;
        long long start = (long long)thread_id * chunk;
        long long end   = (thread_id == num_threads - 1)
                        ? nsteps
                        : (long long)(thread_id + 1) * chunk;

        for (long long i = start; i < end; i++) {
            // метод средних точек:
            double x = a + (i + 0.5) * h;
            local += f(x);
        }

        //  добавляем локальную сумму к общей
        #pragma omp atomic
        sum += local;
    }

    return sum * h;
}

static RunTimes run_once(long long nsteps) {
    // печать числа потоков как раньше
    #pragma omp parallel
    {
        #pragma omp single
        std::cout << "threads=" << omp_get_num_threads() << std::endl;
    }

    auto start = std::chrono::steady_clock::now();

    
    double a = 0.0;
    double b = M_PI;

    double res = integrate_omp(nsteps, a, b);

    auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed{end - start};

    std::cout << "nsteps=" << nsteps << std::endl;
    std::cout << "Work time" << elapsed.count() << std::endl;
    std::cout << res << std::endl;

    return { elapsed.count(), res };
}

int main(int argc, char** argv) {
    long long nsteps = (long long)NSTEPS;

    // Обычный режим: один прогон 
    if (!has_flag(argc, argv, "--bench")) {
        (void)run_once(nsteps);
        return 0;
    }

    // Bench режим: прогоны на 1,2,4,7,8,16,20,40 потоках + CSV
    std::vector<int> Ts = {1, 2, 4, 7, 8, 16, 20, 40};

    std::ofstream f("results.csv", std::ios::out | std::ios::trunc);
    if (!f.is_open()) {
        std::cerr << "ERROR: cannot open results.csv" << std::endl;
        return 2;
    }

    // CSV минимально для графика ускорения
    f << "nsteps,threads,work_time_s,status\n";

    omp_set_dynamic(0);

    for (int t : Ts) {
        omp_set_num_threads(t);
        RunTimes r = run_once(nsteps);
        f << nsteps << "," << t << "," << r.work_s << ",OK\n";
    }

    std::cout << "Saved CSV: results.csv" << std::endl;
    return 0;
}

