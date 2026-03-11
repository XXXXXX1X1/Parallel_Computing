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
static RunTimes run_once() {
    double *A = new double[(long long)NVAL * NVAL];   // матрица A в линейной памяти
    double *x = new double[NVAL];                     // вектор x
    double *y = new double[NVAL];                     // вектор y

    auto start1 = std::chrono::steady_clock::now();   // старт замера инициализации

    // Параллельная инициализация A, x, y
    #pragma omp parallel                              // создаём команду потоков
    {
        #pragma omp single                            // выполняется одним потоком
        std::cout << "threads=" << omp_get_num_threads() << std::endl; // печать числа потоков

        int num_threads = omp_get_num_threads();      
        int thread_id   = omp_get_thread_num();        // id текущего потока: 

        // ===== ИНИЦИАЛИЗАЦИЯ A  =====
        long long totalA = (long long)NVAL * NVAL;     // всего элементов в A
        long long sizeA  = totalA / num_threads;       // базовый размер куска на поток
        long long startA = (long long)thread_id * sizeA; // начало куска A для этого потока
        long long endA   = (thread_id == num_threads - 1) // если поток последний,
                ? totalA                                 // то он забирает хвост до конца
                : (long long)(thread_id + 1) * sizeA;    // иначе конец куска = (id+1)*sizeA

        for (long long i = startA; i < endA; i++) {    
            A[i] = 1.0;                                
        }

        // ===== ИНИЦИАЛИЗАЦИЯ x (вектор) =====
        long long sizeX  = NVAL / num_threads;         // базовый размер куска x на поток
        long long startX = (long long)thread_id * sizeX; // начало диапазона x для потока
        long long endX   = (thread_id == num_threads - 1) // последний поток
                         ? NVAL                          // забирает хвост
                         : (long long)(thread_id + 1) * sizeX; // иначе конец = (id+1)*sizeX

        for (long long i = startX; i < endX; i++) {    // идём по диапазону x
            x[i] = 1.0;                                // заполняем единицами
        }

        // ===== ИНИЦИАЛИЗАЦИЯ y  =====
        long long sizeY  = NVAL / num_threads;        
        long long startY = (long long)thread_id * sizeY; 
        long long endY   = (thread_id == num_threads - 1) 
                         ? NVAL                        
                         : (long long)(thread_id + 1) * sizeY; 

        for (long long i = startY; i < endY; i++) {   
            y[i] = 0.0;                                
        }
    }

    auto end1 = std::chrono::steady_clock::now();      // конец замера инициализации

    // Параллельное умножение матрицы на вектор:
    #pragma omp parallel                               
    {
        int num_threads = omp_get_num_threads();        
        int thread_id   = omp_get_thread_num();        

        long long rows_per_thread = NVAL / num_threads; // базовый размер блока строк на поток
        long long start = (long long)thread_id * rows_per_thread; // первая строка i для потока
        long long end   = (thread_id == num_threads - 1) // последний поток
                        ? NVAL                           // берёт строки до конца (хвост)
                        : (long long)(thread_id + 1) * rows_per_thread; // иначе конец 

        for (long long i = start; i < end; i++) {       // считаем строки матрицы A
            double sum = 0.0;                           // сумма для строки i (скалярное произведение)
            for (long long j = 0; j < NVAL; j++) {      // пробегаем все столбцы j
                sum += A[i * (long long)NVAL + j] * x[j]; 
            }
            y[i] = sum;                               
        }
    }

    auto end2 = std::chrono::steady_clock::now();       // конец замера вычисления

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

    return { elapsed_seconds1.count(), elapsed_seconds2.count(), res }; // возвращаем времена и checksum
}

int main(int argc, char** argv) {
    // Обычный режим: один прогон (без флага --bench), N берётся из #define N
    if (!has_flag(argc, argv, "--bench")) {
        (void)run_once<N>();                             // прогон с N (по умолчанию 20000)
        return 0;                                        // выходим
    }

    std::vector<int> Ts = {1, 2, 4, 7, 8, 16, 20, 40};   // список потоков для прогонов

    std::ofstream f("results.csv", std::ios::out | std::ios::trunc); // открываем CSV на перезапись
    if (!f.is_open()) {                                  
        std::cerr << "ERROR: cannot open results.csv" << std::endl; 
        return 2;                                     
    }

    f << "N,threads,work_time_s,status\n";             

    omp_set_dynamic(0);                                  // запрещаем OpenMP менять число потоков

    auto bench_one = [&](int n_tag, auto runner) {       
        for (int t : Ts) {                               // перебираем все числа потоков
            omp_set_num_threads(t);                      // задаём желаемое число потоков на следующий parallel
            RunTimes r = runner();                       // запускаем прогон, получаем времена
            f << n_tag << "," << t << "," << r.work_s << ",OK\n"; // пишем строку CSV
        }
    };

    bench_one(20000, []() { return run_once<20000>(); }); // прогон N=20000 по всем потокам
    bench_one(40000, []() { return run_once<40000>(); }); // прогон N=40000 по всем потокам

    std::cout << "Saved CSV: results.csv" << std::endl;  // подтверждение сохранения CSV
    return 0;                                            // успех
}

