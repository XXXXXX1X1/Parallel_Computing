#include <omp.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <math.h>
#define VEC_SIZE 1000000000

int main() {
    double *vec1 = new double[VEC_SIZE];
    double res  = 0;

    auto start1 = std::chrono::steady_clock::now();
    for (int i = 0; i < VEC_SIZE; i++) {
        vec1[i] =  1;
       
    }
    auto end1 = std::chrono::steady_clock::now();

    #pragma omp parallel
    {
        {
            #pragma omp single
            std::cout << "threads=" << omp_get_num_threads() << std::endl;
        }

        int num_threads = omp_get_num_threads();   
        int thread_id   = omp_get_thread_num();

        int size_per_thread = VEC_SIZE / num_threads;
        int start = thread_id * size_per_thread;
        int end   = (thread_id == num_threads - 1)
                    ? VEC_SIZE
                    : (thread_id + 1) * size_per_thread;
        double sum = 0;

        for (int i = start; i < end; i++) {
            res += vec1[i];
        }

        #pragma omp critical
        {
            res += sum;
        }
    }

    auto end2 = std::chrono::steady_clock::now();

    const std::chrono::duration<double> elapsed_seconds1{end1 - start1};
    const std::chrono::duration<double> elapsed_seconds2{end2 - end1};

    std::cout << "Init time" << elapsed_seconds1.count() << std::endl;
    std::cout << "Work time" << elapsed_seconds2.count() << std::endl;
    std::cout <<res<<std::endl;
    delete[] vec1;
    return 0;
}