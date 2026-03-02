#include <vector>
#include <complex>
#include <cstdint>
#include <cmath>
#include <random>
#include <iostream>

std::vector<std::complex<float>> convolve(const std::vector<std::complex<float>>& x) {
    
    std::vector<std::complex<float>> h = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    
    int N = x.size();
    int M = h.size();
    int result_size = N + M - 1;

    std::vector<std::complex<float>> y(result_size, 0.0f);
    
    for (int n = 0; n < result_size; n++) {
        for (int k = 0; k < M; k++) {
            int x_index = n - k;
            
            if (x_index >= 0 && x_index < N) {
                y[n] += h[k] * x[x_index];
            }
        }
    }
    
    return y;
}

void bpsk(const std::vector<int16_t>& in, std::vector<std::complex<float>>& out) {
    std::complex<float> val;
    for (size_t i = 0; i < in.size(); i++) {
        if (in[i] == 1) {
            val = std::complex<float>(1.0f, 0.0f);
        } else {
            val = std::complex<float>(-1.0f, 0.0f);
        }
        out.push_back(val);
    }
}

std::vector<std::complex<float>> modulate(const std::vector<std::complex<float>>& in, int step) {
    std::vector<std::complex<float>> upbits;
    upbits.reserve(in.size() * (step + 1)); 
    
    for (const auto& val : in) {
        upbits.push_back(val);           
        for (int i = 0; i < step; i++) {
            upbits.push_back(std::complex<float>(0.0f, 0.0f));      
        }
    }
    
    return upbits;
}

void to_file(const std::vector<std::complex<float>>& in, int16_t buff[], int size) {
    int k = 0;
    // заполнение tx_buff значениями сэмплов первые 16 бит - I, вторые 16 бит - Q.
    for (int i = 0; i < size && k < (int)in.size(); i += 2) {
        buff[i] = (int16_t)(in[k].real() * 1600);
        buff[i + 1] = (int16_t)(in[k].imag() * 1600);
        k++;
    }
    // Заполняем оставшуюся часть буфера нулями
    for (int i = k * 2; i < size; i++) {
        buff[i] = 0;
    }
}

std::vector<std::complex<float>> from_file(const int16_t buff[], int size) {
    std::vector<std::complex<float>> sv2;
    sv2.reserve(size / 2);
    for (int i = 0; i < size - 1; i += 2) {
        float real_part = (float)buff[i] / 1600.0f;
        float imag_part = (float)buff[i + 1] / 1600.0f;
        sv2.push_back(std::complex<float>(real_part, imag_part));
    }
    return convolve(sv2);
}

std::vector<int16_t> generate_bits(int size) {
    std::vector<int16_t> bits(size);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 1);
    
    for (int i = 0; i < size; i++) {
        bits[i] = dis(gen);
    }
    
    return bits;
}

int16_t* read_pcm(const char* filename, size_t* sample_count) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Cannot open file %s\n", filename);
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("file_size = %ld\n", file_size);
    
    int16_t* samples = (int16_t*)malloc(file_size);
    *sample_count = file_size / sizeof(int16_t);

    size_t sf = fread(samples, sizeof(int16_t), *sample_count, file);

    fclose(file);
    return samples;
}

std::vector<std::complex<float>> sim_sync(const std::vector<std::complex<float>>& y) {
    int offset = 0;
    const int Nsp = 10;

    float BnTs = 0.01f;
    float zeta = std::sqrt(2.0f) / 2.0f;
    float Kp = 0.01f;
    
    float teta = (BnTs / Nsp) / (zeta + (1.0f / (4.0f * zeta)));
    float K1 = (-4.0f * zeta * teta) / (1.0f + 2.0f * teta * zeta + std::pow(teta, 2.0f)) * Kp;
    float K2 = (-4.0f * teta * teta) / (1.0f + 2.0f * teta * zeta + std::pow(teta, 2.0f)) * Kp;
    
    float p2 = 0.0f;
    std::vector<std::complex<float>> result;
    
    for (int ns = 0; ns < (int)y.size() - Nsp; ns += Nsp) {
        int n = offset;
        
        float real_err = (y[ns + n].real() - y[n + ns + Nsp].real()) * y[n + (Nsp / 2) + ns].real();
        float imag_err = (y[ns + n].imag() - y[n + ns + Nsp].imag()) * y[n + (Nsp / 2) + ns].imag();
        float error = imag_err + real_err;

        float p1 = error * K1;
        p2 = p2 + p1 + error * K2;

        if (p2 > 1.0f) {
            p2 = p2 - 1.0f;
        }
        if (p2 < 0.0f) {  
            p2 += 1.0f;
        }

        int new_offset = (int)std::round(p2 * Nsp);
        offset = new_offset;
        
        if (ns + offset < (int)y.size()) {
            result.push_back(y[ns + offset]);
        }
    }
    
    return result;
}