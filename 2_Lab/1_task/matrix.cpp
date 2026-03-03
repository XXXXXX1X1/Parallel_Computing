#include <omp.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <math.h>
#include <vector>
#include <string>
#include <cstring>

#ifndef N
#define N 20000
#endif

struct RunTimes {
    double init_s;
    double work_s;
    double res;
};

static bool has_flag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

template<int NVAL>
static RunTimes run_once() {
    // ====== ТВОЙ КОД (логика/вывод те же), только N берём как NVAL ======
    double *A = new double[(long long)NVAL * NVAL];
    double *x = new double[NVAL];
    double *y = new double[NVAL];

    auto start1 = std::chrono::steady_clock::now();

    // Параллельная инициализация
    #pragma omp parallel
    {
        #pragma omp single
        std::cout << "threads=" << omp_get_num_threads() << std::endl;

        int num_threads = omp_get_num_threads();
        int thread_id   = omp_get_thread_num();

        // Инициализация A (матрица N x N)
        long long totalA = (long long)NVAL * NVAL;
        long long sizeA  = totalA / num_threads;
        long long startA = (long long)thread_id * sizeA;
        long long endA   = (thread_id == num_threads - 1)
                         ? totalA
                         : (long long)(thread_id + 1) * sizeA;

        for (long long i = startA; i < endA; i++) {
            A[i] = 1.0;
        }

        // Инициализация x (вектор)
        long long sizeX  = NVAL / num_threads;
        long long startX = (long long)thread_id * sizeX;
        long long endX   = (thread_id == num_threads - 1)
                         ? NVAL
                         : (long long)(thread_id + 1) * sizeX;

        for (long long i = startX; i < endX; i++) {
            x[i] = 1.0;
        }
        // Инициализация y (результат)
        long long sizeY  = NVAL / num_threads;
        long long startY = (long long)thread_id * sizeY;
        long long endY   = (thread_id == num_threads - 1)
                         ? NVAL
                         : (long long)(thread_id + 1) * sizeY;

        for (long long i = startY; i < endY; i++) {
            y[i] = 0.0;
        }
    }

    auto end1 = std::chrono::steady_clock::now();

    // Параллельное умножение матрицы на вектор: y = A * x
    #pragma omp parallel
    {
        int num_threads = omp_get_num_threads();
        int thread_id   = omp_get_thread_num();

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
    }

    auto end2 = std::chrono::steady_clock::now();

    // Контрольная сумма
    double res = 0.0;
    for (long long i = 0; i < NVAL; i++) {
        res += y[i];
    }

    const std::chrono::duration<double> elapsed_seconds1{end1 - start1};
    const std::chrono::duration<double> elapsed_seconds2{end2 - end1};

    std::cout << "N:" << NVAL << std::endl;
    std::cout << "Init time" << elapsed_seconds1.count() << std::endl;
    std::cout << "Work time" << elapsed_seconds2.count() << std::endl;
    std::cout << res << std::endl;

    delete[] A;
    delete[] x;
    delete[] y;

    return { elapsed_seconds1.count(), elapsed_seconds2.count(), res };
}

int main(int argc, char** argv) {
    // ===== обычный режим: как сейчас (один прогон) =====
    if (!has_flag(argc, argv, "--bench")) {
        (void)run_once<N>();
        return 0;
    }

    // ===== bench режим: два N (20000, 40000) * все потоки, минимальный CSV =====
    std::vector<int> Ts = {1, 2, 4, 7, 8, 16, 20, 40};

    std::ofstream f("results.csv", std::ios::out | std::ios::trunc);
    if (!f.is_open()) {
        std::cerr << "ERROR: cannot open results.csv" << std::endl;
        return 2;
    }

    // минимально для графика
    f << "N,threads,work_time_s,status\n";

    omp_set_dynamic(0);

    auto bench_one = [&](int n_tag, auto runner) {
        for (int t : Ts) {
            omp_set_num_threads(t);
            RunTimes r = runner();
            f << n_tag << "," << t << "," << r.work_s << ",OK\n";
        }
    };

    // прогон N=20000
    bench_one(20000, []() { return run_once<20000>(); });

    // прогон N=40000
    bench_one(40000, []() { return run_once<40000>(); });

    std::cout << "Saved CSV: results.csv" << std::endl;
    return 0;
}

/*LIBOMP="$(brew --prefix libomp)"

clang++ -O3 -std=c++17 -DN=20000 matrix.cpp \
  -Xpreprocessor -fopenmp \
  -I"$LIBOMP/include" \
  -L"$LIBOMP/lib" -lomp \
  -Wl,-rpath,"$LIBOMP/lib" \
  -o matrix
  */