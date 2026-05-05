#pragma once

#include "main.h"

vector<float> offset(vector<complex<float>> matched) ;
std::vector<std::complex<float>> symbol_sync(std::vector<std::complex<float>> &matched);
std::vector<std::complex<float>> freq_synq(std::vector<std::complex<float>> &in,
                                           double coarse_freq,
                                           int buffer_size,
                                           int sample_rate);
std::vector<std::complex<float>> costas_loop_bpsk(const std::vector<std::complex<float>>& samples);