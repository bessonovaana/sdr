#pragma once

#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdint.h>
#include <vector>
#include <complex>
#include <atomic>
#include <mutex>

using namespace std;

#pragma once

struct Sharing {
    vector<complex<float>> tx_samples;
    vector<complex<float>> rx_samples;
    atomic<bool> program_running{true};
    atomic<bool> sdr_ready{false};
    mutex mtx;
    vector<float> sync;
    atomic<long long> tx_timestamp{0};
};

extern Sharing shared;

const std::vector<float> barker7 = {1, 1, 1, 1, 1, -1, -1};

struct IQData {
    vector<double> real;  // I
    vector<double> imag;  // Q
    vector<double> count; // X
};



IQData extractIQ(const vector<complex<float>>& samples);