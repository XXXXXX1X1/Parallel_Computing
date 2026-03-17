#include <iostream>
#include <fstream>
#include <thread>
#include <random>
#include <cmath>
#include <iomanip>
#include <string>

#include "server.hpp"

double f_sin(double x)
{
    return std::sin(x);
}

double f_sqrt(double x)
{
    return std::sqrt(x);
}

double f_pow(double x, double y)
{
    return std::pow(x, y);
}

void client_sin(TaskServer<double>& server, int n, const std::string& filename)
{
    std::ofstream out(filename);
    std::mt19937 gen(12345);
    std::uniform_real_distribution<double> dist(-100, 100);

    out << std::fixed <<std::setprecision(12);
    for (int i = 0; i < n; i++)
    {
        double x = dist(gen);

        size_t id = server.add_task([x]() {
            return f_sin(x);
        });

        double result = server.request_result(id);

        out << "type=sin x=" << x << "result=" << result << std::endl;
    }
}

void client_sqrt(TaskServer<double>& server, int n, const std::string& filename)
{
    std::ofstream out(filename);
    std::mt19937 gen(54321);
    std::uniform_real_distribution<double> dist(0, 10000.0);

    out << std::fixed <<std::setprecision(12);
    for (int i = 0; i < n; i++)
    {
        double x = dist(gen);

        size_t id = server.add_task([x]() {
            return f_sqrt(x);
        });

        double result = server.request_result(id);

        out << "type=sqrt x=" << x << "result=" << result << std::endl;
    }
}

void client_pow(TaskServer<double>& server, int n, const std::string& filename)
{
    std::ofstream out(filename);
    std::mt19937 gen(77777);
    std::uniform_real_distribution<double> dist_x(0.1, 20.0);
    std::uniform_real_distribution<double> dist_y(0.1, 20.0);


    out << std::fixed <<std::setprecision(12);
    for (int i = 0; i < n; i++)
    {
        double x = dist_x(gen);
        double y = dist_y(gen);

        size_t id = server.add_task([x, y]() {
            return f_pow(x, y);
        });

        double result = server.request_result(id);

        out << "type=pow x=" << x << "y=" << y << "result=" << result << std::endl;
    }
}

int main()
{
    int n = 100;

    TaskServer<double> server;
    server.start();
     
    std::thread t1(client_sin, std::ref(server), n, "client_sin.txt");
    std::thread t2(client_sqrt, std::ref(server), n, "client_sqrt.txt");
    std::thread t3(client_pow, std::ref(server), n, "client_pow.txt");

    t1.join();
    t2.join();
    t3.join();

    server.stop();
    
    return 0;
}