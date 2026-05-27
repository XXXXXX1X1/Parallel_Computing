#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#if __has_include(<cublas_v2.h>)
#include <cublas_v2.h>
#define HEAT2D_HAS_CUBLAS 1
#else
#define HEAT2D_HAS_CUBLAS 0
#endif

namespace {

constexpr double kTopLeft = 10.0;
constexpr double kTopRight = 20.0;
constexpr double kBottomRight = 30.0;
constexpr double kBottomLeft = 20.0;

constexpr const char* kReductionOpenACC = "openacc";
constexpr const char* kReductionCuBLAS = "cublas";

struct Config {
    std::size_t size = 128;
    double epsilon = 1e-6;
    int max_iterations = 1'000'000;
    std::string reduction = kReductionOpenACC;
};

struct Result {
    int iterations = 0;
    double error = 0.0;
    std::vector<double> grid;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

double interpolate(double left, double right, std::size_t pos, std::size_t last) {
    const double t = static_cast<double>(pos) / static_cast<double>(last);
    return left + (right - left) * t;
}

std::size_t index_of(std::size_t row, std::size_t col, std::size_t n) {
    return row * n + col;
}

Config parse_args(int argc, char** argv) {
    namespace po = boost::program_options;

    Config cfg;

    po::options_description options("Options");
    options.add_options()
        ("size", po::value<std::size_t>(&cfg.size)->default_value(cfg.size), "Grid size (NxN)")
        ("epsilon", po::value<double>(&cfg.epsilon)->default_value(cfg.epsilon), "Convergence threshold")
        ("max-iterations", po::value<int>(&cfg.max_iterations)->default_value(cfg.max_iterations), "Maximum iterations")
        ("reduction", po::value<std::string>(&cfg.reduction)->default_value(cfg.reduction), "Reduction mode: openacc or cublas");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    return cfg;
}

void validate_config(const Config& cfg) {
    if (cfg.size < 2) {
        fail("Grid size must be at least 2.");
    }
    if (cfg.epsilon <= 0.0) {
        fail("Epsilon must be positive.");
    }
    if (cfg.max_iterations < 1) {
        fail("Maximum number of iterations must be positive.");
    }
    if (cfg.reduction != kReductionOpenACC && cfg.reduction != kReductionCuBLAS) {
        fail("Reduction mode must be 'openacc' or 'cublas'.");
    }
#if !HEAT2D_HAS_CUBLAS
    if (cfg.reduction == kReductionCuBLAS) {
        fail("This build does not have cuBLAS headers, so cublas reduction is unavailable.");
    }
#endif
}

void initialize_boundaries(double* grid, double* new_grid, std::size_t n) {
    const std::size_t last = n - 1;

    for (std::size_t col = 0; col < n; ++col) {
        const double top = interpolate(kTopLeft, kTopRight, col, last);
        const double bottom = interpolate(kBottomLeft, kBottomRight, col, last);

        grid[index_of(0, col, n)] = top;
        new_grid[index_of(0, col, n)] = top;

        grid[index_of(last, col, n)] = bottom;
        new_grid[index_of(last, col, n)] = bottom;
    }

    for (std::size_t row = 0; row < n; ++row) {
        const double left = interpolate(kTopLeft, kBottomLeft, row, last);
        const double right = interpolate(kTopRight, kBottomRight, row, last);

        grid[index_of(row, 0, n)] = left;
        new_grid[index_of(row, 0, n)] = left;

        grid[index_of(row, last, n)] = right;
        new_grid[index_of(row, last, n)] = right;
    }
}

#if HEAT2D_HAS_CUBLAS
void check_cublas(cublasStatus_t status, const char* step) {
    if (status != CUBLAS_STATUS_SUCCESS) {
        fail(std::string("cuBLAS error at step: ") + step);
    }
}

double max_error_with_cublas(cublasHandle_t handle, double* diff, std::size_t total) {
    int max_index = 0;

#pragma acc host_data use_device(diff)
    {
        check_cublas(
            cublasIdamax(handle, static_cast<int>(total), diff, 1, &max_index),
            "cublasIdamax");
    }

    if (max_index <= 0) {
        return 0.0;
    }

#pragma acc update self(diff[max_index - 1:1])
    return diff[max_index - 1];
}
#endif

Result solve(const Config& cfg) {
    const std::size_t n = cfg.size;
    const std::size_t total = n * n;
    const bool use_cublas = (cfg.reduction == kReductionCuBLAS);

    std::vector<double> grid(total, 0.0);
    std::vector<double> new_grid(total, 0.0);
    std::vector<double> diff(total, 0.0);

    double* grid_a = grid.data();
    double* grid_b = new_grid.data();
    double* diff_ptr = diff.data();

    initialize_boundaries(grid_a, grid_b, n);

    double error = std::numeric_limits<double>::infinity();
    int iterations = 0;
    bool use_a_as_source = true;

#pragma acc data copy(grid_a[0:total], grid_b[0:total]) create(diff_ptr[0:total])
    {
#if HEAT2D_HAS_CUBLAS
        cublasHandle_t cublas_handle = nullptr;
        if (use_cublas) {
            check_cublas(cublasCreate(&cublas_handle), "cublasCreate");
        }
#endif

        while (iterations < cfg.max_iterations && error > cfg.epsilon) {
            const double* src = use_a_as_source ? grid_a : grid_b;
            double* dst = use_a_as_source ? grid_b : grid_a;

            error = 0.0;

            if (use_cublas) {
#pragma acc parallel loop collapse(2) present(src[0:total], dst[0:total], diff_ptr[0:total])
                for (std::size_t row = 1; row < n - 1; ++row) {
                    for (std::size_t col = 1; col < n - 1; ++col) {
                        const std::size_t idx = index_of(row, col, n);
                        const double new_value =
                            0.25 * (src[index_of(row - 1, col, n)] +
                                    src[index_of(row + 1, col, n)] +
                                    src[index_of(row, col - 1, n)] +
                                    src[index_of(row, col + 1, n)]);

                        dst[idx] = new_value;
                        diff_ptr[idx] = std::fabs(new_value - src[idx]);
                    }
                }

#if HEAT2D_HAS_CUBLAS
                error = max_error_with_cublas(cublas_handle, diff_ptr, total);
#endif
            } else {
#pragma acc parallel loop collapse(2) present(src[0:total], dst[0:total])
                for (std::size_t row = 1; row < n - 1; ++row) {
                    for (std::size_t col = 1; col < n - 1; ++col) {
                        dst[index_of(row, col, n)] =
                            0.25 * (src[index_of(row - 1, col, n)] +
                                    src[index_of(row + 1, col, n)] +
                                    src[index_of(row, col - 1, n)] +
                                    src[index_of(row, col + 1, n)]);
                    }
                }

#pragma acc parallel loop collapse(2) reduction(max:error) present(src[0:total], dst[0:total])
                for (std::size_t row = 1; row < n - 1; ++row) {
                    for (std::size_t col = 1; col < n - 1; ++col) {
                        const double diff_value =
                            std::fabs(dst[index_of(row, col, n)] -
                                      src[index_of(row, col, n)]);

                        error = std::max(error, diff_value);
                    }
                }
            }

            use_a_as_source = !use_a_as_source;
            ++iterations;
        }

#if HEAT2D_HAS_CUBLAS
        if (use_cublas) {
            check_cublas(cublasDestroy(cublas_handle), "cublasDestroy");
        }
#endif
    }

    Result result;
    result.iterations = iterations;
    result.error = error;
    result.grid = use_a_as_source ? std::move(grid) : std::move(new_grid);

    return result;
}

void print_grid(const std::vector<double>& grid, std::size_t n) {
    std::cout << "Grid " << n << "x" << n << ":\n";
    std::cout << std::fixed << std::setprecision(6);

    for (std::size_t row = 0; row < n; ++row) {
        for (std::size_t col = 0; col < n; ++col) {
            std::cout << std::setw(11) << grid[index_of(row, col, n)] << ' ';
        }
        std::cout << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Config cfg = parse_args(argc, argv);
        validate_config(cfg);

        const auto start = std::chrono::steady_clock::now();
        const Result result = solve(cfg);
        const auto finish = std::chrono::steady_clock::now();

        const double elapsed_seconds =
            std::chrono::duration<double>(finish - start).count();

        std::cout << std::setprecision(10);
        std::cout << "iterations=" << result.iterations << '\n';
        std::cout << "error=" << result.error << '\n';
        std::cout << "time=" << elapsed_seconds << '\n';

        if (cfg.size == 10 || cfg.size == 13) {
            print_grid(result.grid, cfg.size);
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
