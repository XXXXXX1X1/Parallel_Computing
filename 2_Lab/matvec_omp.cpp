// matvec_omp.cpp
// Run: OMP_NUM_THREADS=8 ./matvec 20000

#include <omp.h>
#include <cstdio>
#include <cstdlib>
#include <cstddef>

static void* xmalloc(std::size_t bytes) {
    void* p = std::malloc(bytes);
    if (!p) { std::perror("malloc"); std::exit(1); }
    return p;
}

int main(int argc, char** argv) {
    int N = (argc > 1) ? std::atoi(argv[1]) : 4000;
    if (N <= 0) { std::fprintf(stderr, "N must be > 0\n"); return 2; }

    std::size_t NN = (std::size_t)N * (std::size_t)N;

    double* A = (double*)xmalloc(NN * sizeof(double));
    double* b = (double*)xmalloc((std::size_t)N * sizeof(double));
    double* c = (double*)xmalloc((std::size_t)N * sizeof(double));

    // parallel init A
#pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        std::size_t base = (std::size_t)i * (std::size_t)N;
        for (int j = 0; j < N; j++) {
            A[base + (std::size_t)j] = (double)(i + j) * 0.001;
        }
    }

    // parallel init b
#pragma omp parallel for schedule(static)
    for (int j = 0; j < N; j++) {
        b[j] = (double)j * 0.001;
    }

    int threads = 1;
#pragma omp parallel
    {
#pragma omp single
        threads = omp_get_num_threads();
    }

    // matvec
    double t0 = omp_get_wtime();
#pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        std::size_t base = (std::size_t)i * (std::size_t)N;
        double sum = 0.0;
        for (int j = 0; j < N; j++) {
            sum += A[base + (std::size_t)j] * b[j];
        }
        c[i] = sum;
    }
    double t1 = omp_get_wtime();

    std::printf("N=%d threads=%d time_sec=%.6f c0=%.10e\n", N, threads, (t1 - t0), c[0]);

    std::free(A);
    std::free(b);
    std::free(c);
    return 0;
}