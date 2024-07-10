#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <cstdint>


// Perform dot product
template <typename T>
T dotProduct(const std::vector<T>& v1, const std::vector<T>& v2) {
    T result = 0.0;
    for(size_t i = 0; i < v1.size(); ++i) {
        result += v1[i] * v2[i];
    }
    return result;
};

// Print the siingle vector matrix
template <typename T>
void printMatrix(const std::vector<T> &M, const uint64_t &N) {
    for(uint64_t m = 0; m < N; ++m) {
        for(uint64_t j = 0; j < N; ++j) {
            std::printf("%f ", M[ m * N + j]);
        }
        std::printf("\n");
    }
};

#endif // UTILS_HPP