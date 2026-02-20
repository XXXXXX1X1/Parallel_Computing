#include <omp.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <math.h>

#define N 20000   

int main() {
    double *A = new double[(long long)N * N];
    double *x = new double[N];
    double *y = new double[N];

    auto start1 = std::chrono::steady_clock::now();

    // Параллельная инициализация
    #pragma omp parallel
    {
        #pragma omp single
        std::cout << "threads=" << omp_get_num_threads() << std::endl;

        int num_threads = omp_get_num_threads();
        int thread_id   = omp_get_thread_num();

        // Инициализация A (матрица N x N)
        long long totalA = (long long)N * N;
        long long sizeA  = totalA / num_threads;
        long long startA = (long long)thread_id * sizeA;
        long long endA   = (thread_id == num_threads - 1)
                         ? totalA
                         : (long long)(thread_id + 1) * sizeA;

        for (long long i = startA; i < endA; i++) {
            A[i] = 1.0;
        }

        // Инициализация x (вектор)
        long long sizeX  = N / num_threads;
        long long startX = (long long)thread_id * sizeX;
        long long endX   = (thread_id == num_threads - 1)
                         ? N
                         : (long long)(thread_id + 1) * sizeX;

        for (long long i = startX; i < endX; i++) {
            x[i] = 1.0;
        }

        // Инициализация y (результат)
        long long sizeY  = N / num_threads;
        long long startY = (long long)thread_id * sizeY;
        long long endY   = (thread_id == num_threads - 1)
                         ? N
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

        long long rows_per_thread = N / num_threads;
        long long start = (long long)thread_id * rows_per_thread;
        long long end   = (thread_id == num_threads - 1)
                        ? N
                        : (long long)(thread_id + 1) * rows_per_thread;

        for (long long i = start; i < end; i++) {
            double sum = 0.0;
            for (long long j = 0; j < N; j++) {
                sum += A[i * (long long)N + j] * x[j];
            }
            y[i] = sum;
        }
    }

    auto end2 = std::chrono::steady_clock::now();

    // Контрольная сумма
    double res = 0.0;
    for (long long i = 0; i < N; i++) {
        res += y[i];
    }

    const std::chrono::duration<double> elapsed_seconds1{end1 - start1};
    const std::chrono::duration<double> elapsed_seconds2{end2 - end1};

    std::cout << "Init time" << elapsed_seconds1.count() << std::endl;
    std::cout << "Work time" << elapsed_seconds2.count() << std::endl;
    std::cout << res << std::endl;

    delete[] A;
    delete[] x;
    delete[] y;
    return 0;
}