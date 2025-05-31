#ifndef __UTILS_H__
#define __UTILS_H__

#include <iostream>
#include <string>
#include <chrono>


class Timer {
public:
    Timer() : start_time(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    double elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end_time - start_time).count();
    }

    void printElapsed(const std::string& message = "Elapsed time: ") const {
        std::cout << message << elapsed() << " seconds" << std::endl;
    }
private:
    std::chrono::high_resolution_clock::time_point start_time;
};

#endif