#include <iostream>
#include <vector>
#include <complex>
#include "mapper.h"


std::vector<std::complex<float>> bpsk (std::vector<int16_t>in){
   std::vector<std::complex<float>>out;
    for (int i=0; i<in.size(); i++){
        
        out.emplace_back(((1/sqrt(2))* (1 - 2*in[i])),
        ((1/std::sqrt(2))*(1-2*in[i])));
    }
    return out;
}
std::vector<std::complex<float>> qpsk (std::vector<int16_t>in){
    std::vector<std::complex<float>>out;
    for (int i=0; i<in.size(); i+=2){
        float I = (1/sqrt(2))* (1 - 2*in[i]);
        float Q = (1/std::sqrt(2))*(1-2*in[i+1]);
        out.emplace_back(I,Q);
    }
    return out;
}
std::vector<std::complex<float>> qam16 (std::vector<int16_t>in){
    std::vector<std::complex<float>>out;
    for (int i=0; i<in.size(); i+=4){
        float I = (1/sqrt(10))* (1 - 2*in[i]) *(2 -(1 -2*in[i+1]));
        float Q = (1/sqrt(10))*(1 - 2*in[i+2]) *(2 -(1 -2*in[i+3]));
        out.emplace_back(I,Q);
    }
    return out;
}