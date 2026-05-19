#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <filesystem>

bool close_enough(double a, double b, double eps)
{
    return std::fabs(a - b) <= eps * std::max(1.0, std::max(std::fabs(a), std::fabs(b)));
}

bool test_sin_file(const std::string& filename, double eps)
{
    std::ifstream in(filename);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << '\n';
        return false;
    }

    std::string h1, h2;
    in >> h1 >> h2; // x result

    double x_file, result_file;
    int line = 2;

    while (in >> x_file >> result_file)
    {
        double expected = std::sin(x_file);

        if (!close_enough(result_file, expected, eps))
        {
            std::cerr << "Mismatch in " << filename
                      << " line " << line
                      << ": expected=" << expected
                      << " result=" << result_file << '\n';
            return false;
        }

        line++;
    }

    return true;
}

bool test_sqrt_file(const std::string& filename, double eps)
{
    std::ifstream in(filename);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << '\n';
        return false;
    }

    std::string h1, h2;
    in >> h1 >> h2; // x result

    double x_file, result_file;
    int line = 2;

    while (in >> x_file >> result_file)
    {
        double expected = std::sqrt(x_file);

        if (!close_enough(result_file, expected, eps))
        {
            std::cerr << "Mismatch in " << filename
                      << " line " << line
                      << ": expected=" << expected
                      << " result=" << result_file << '\n';
            return false;
        }

        line++;
    }

    return true;
}

bool test_pow_file(const std::string& filename, double eps)
{
    std::ifstream in(filename);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << '\n';
        return false;
    }

    std::string h1, h2, h3;
    in >> h1 >> h2 >> h3; // x y result

    double x_file, y_file, result_file;
    int line = 2;

    while (in >> x_file >> y_file >> result_file)
    {
        double expected = std::pow(x_file, y_file);

        if (!close_enough(result_file, expected, eps))
        {
            std::cerr << "Mismatch in " << filename
                      << " line " << line
                      << ": expected=" << expected
                      << " result=" << result_file << '\n';
            return false;
        }

        line++;
    }

    return true;
}

int main()
{
    std::string dir = "result";
    double eps = 1e-9;

    bool ok1 = test_sin_file(dir + "/client_sin.txt", eps);
    bool ok2 = test_sqrt_file(dir + "/client_sqrt.txt", eps);
    bool ok3 = test_pow_file(dir + "/client_pow.txt", eps);

    if (ok1 && ok2 && ok3)
    {
        std::cout << "All tests passed\n";
        return 0;
    }

    std::cout << "Tests failed\n";
    return 1;
}