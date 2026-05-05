#include <vector>
#include <complex>

#ifndef MAPPER_H
#define MAPPER_H
std::vector<std::complex<float>> bpsk (std::vector<int16_t>in);
std::vector<std::complex<float>> qpsk (std::vector<int16_t>in);
std::vector<std::complex<float>> qam16 (std::vector<int16_t>in);
#endif