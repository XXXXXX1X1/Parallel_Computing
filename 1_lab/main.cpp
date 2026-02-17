#include <cmath>
#include <iostream>
#include <vector>
#include <numbers>

#ifdef USE_DOUBLE
using T = double;
#else
using T = float;
#endif

int main() {
    const int N = 100000000000;

    std::vector<T> a(N);
    T sum = 0;

    const double PI = std::numbers::pi; 

    for (int i = 0; i < N; i++) {
        double x = 2.0 * PI * i / N;
        a[i] = (T)std::sin(x);
        sum += a[i];
    }

    std::cout << "sum = " << sum << std::endl;
    return 0;
}
