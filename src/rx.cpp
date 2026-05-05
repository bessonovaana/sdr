#include"rx.h"
#include <iostream>

// 🔹 arange: аналог numpy.arange
inline std::vector<double> arange(double start, double stop, double step) {
    std::vector<double> result;
    if (step <= 0) return result;
    for (double val = start; val < stop; val += step) {
        result.push_back(val);
    }
    return result;
}

// 🔹 fftshift_1d: сдвиг нуля в центр спектра
inline void fftshift_1d(std::vector<double>& data, int size) {
    if (size <= 1) return;
    std::vector<double> temp = data;
    const int half = size / 2;
    for (int i = 0; i < size; ++i) {
        data[i] = temp[(i + half) % size];
    }
}

// 🔹 sgn: функция знака
inline float sgn(float val) {
    return (0.0f < val) - (val < 0.0f);
}

vector<float> offset(vector<complex<float>> matched) 
{
    int samples_per_symbol = 10;
    int K1, K2, p1, p2 = 0;
    float BnTs = 0.001;
    float Kp = 0.002;
    float zeta = sqrt(2) / 2;
    float theta = (BnTs / samples_per_symbol) / (zeta + (0.25 / zeta));
    K1 = -4 * zeta * theta / ( (1 + 2 * zeta * theta + pow(theta,2)) * Kp);
    K2 = -4 * pow(theta,2) / ( (1 + 2 * zeta * theta + pow(theta,2))* Kp);
    int tau = 0;
    float err;
    vector<float> errof;


    for (int i = 0; i < matched.size(); i += samples_per_symbol)
    {
        err = (matched[i + samples_per_symbol + tau].real() - matched[i + tau]).real() * matched[i + (samples_per_symbol / 2) + tau].real() + (matched[i + samples_per_symbol + tau].imag() - matched[i + tau]).imag() * matched[i + (samples_per_symbol / 2) + tau].imag(); 
        p1 = err * K1;
        p2 =  p2 + p1 + err * K2;

        if (p2 > 1)
        {
            p2 = p2 - 1;
        }

        if (p2 < -1)
        {
            p2 = p2 + 1;
        }
        tau = ceil(p2 * samples_per_symbol);
        errof.push_back(i + samples_per_symbol + tau);
        }
        
        return errof;
    }

std::vector<std::complex<float>> symbol_sync(std::vector<std::complex<float>> &matched)
{
    int nsps =10;
    std::vector<float> ted_idxs = offset(matched);

    std::vector<std::complex<float>> symb_samples;
    for (float idx : ted_idxs) {
            if (idx < matched.size()) {
                symb_samples.push_back(matched[idx]); 
            }
        }
        

    return symb_samples;

}


std::vector<std::complex<float>> freq_synq(std::vector<std::complex<float>> &in,
                                           double coarse_freq,
                                           int buffer_size,
                                           int sample_rate)
{
    std::vector<std::complex<float>> out(buffer_size);
    int Nt = barker7.size();
    int L = 1;

    std::complex<float> sum = 0.0f;

    for (int l = 0; l < L; ++l) {
        for (int j = 0; j < Nt; ++j) {
            int idx = j + l * Nt;
            if (idx < buffer_size) {
                sum += (float)barker7[j] * in[idx];
            }
        }
    }

    float eta = std::arg(sum) / (2.0f * M_PI * Nt);
    double norm_f = eta * sample_rate;
    double res_f = coarse_freq + norm_f;

    for (int i = 0; i < buffer_size; ++i) {
        double phase = -2.0 * M_PI * res_f * i / sample_rate;
        out[i] = in[i] * std::exp(std::complex<float>(0.0f, (float)phase));
    }

    return out;
}

#include <fftw3.h>

float coarse_max_freq_calculation(const std::vector<std::complex<float>>& samples, 
                                  int buffer_size, 
                                  int sample_rate)
{
    //  Выделение памяти + инициализация
    std::vector<std::complex<double>> fft_in(buffer_size);
    std::vector<std::complex<double>> fft_out(buffer_size);
    std::vector<double> fft_mag(buffer_size);
    
    // 🔥 Копирование с приведением float→double
    for (int i = 0; i < buffer_size && i < static_cast<int>(samples.size()); ++i) {
        fft_in[i] = static_cast<std::complex<double>>(samples[i]);
    }

    // Возведение в квадрат: z*z вместо pow(z,2)
    for (int i = 0; i < buffer_size; ++i) {
        fft_in[i] = fft_in[i] * fft_in[i];  // Для BPSK: убираем модуляцию
    }

    // План FFTW
    fftw_plan plan = fftw_plan_dft_1d(buffer_size,
                                      reinterpret_cast<fftw_complex*>(fft_in.data()),
                                      reinterpret_cast<fftw_complex*>(fft_out.data()),
                                      FFTW_FORWARD,
                                      FFTW_ESTIMATE);
    
    fftw_execute(plan);
    fftw_destroy_plan(plan); 
    
    // Амплитудный спектр
    for (int i = 0; i < buffer_size; ++i) {
        fft_mag[i] = std::abs(fft_out[i]);
    }

    // Сдвиг нуля в центр (если fftshift_1d не реализована — вот простая версия)
    std::vector<double> fft_mag_shifted(buffer_size);
    const int half = buffer_size / 2;
    for (int i = 0; i < buffer_size; ++i) {
        fft_mag_shifted[i] = fft_mag[(i + half) % buffer_size];
    }

    // Вектор частот
    const double df = static_cast<double>(sample_rate) / static_cast<double>(buffer_size);
    std::vector<double> freqs(buffer_size);
    for (int i = 0; i < buffer_size; ++i) {
        freqs[i] = (i - half) * df;  // От -Fs/2 до +Fs/2
    }
    
    //  Поиск пика
    auto max_it = std::max_element(fft_mag_shifted.begin(), fft_mag_shifted.end());
    const int peak_idx = static_cast<int>(std::distance(fft_mag_shifted.begin(), max_it));
    
    // Деление на 2: т.к. сигнал был возведён в квадрат (частота удвоилась)
    return static_cast<float>(freqs[peak_idx] * 0.5);
}

std::vector<std::complex<float>> costas_loop_bpsk(const std::vector<std::complex<float>>& samples) {
    int N = samples.size();
    double phase = 0.0f;
    double freq = 0.0f;
    double alpha = 1.0f;
    double beta = 0.02f;
    std::vector<std::complex<float>> out;
    out.resize(N);

    for (int i = 0; i < N; ++i) {
        std::complex<float> exp_term = std::complex<float>(0.0, -1.0f * phase);
        out[i] = samples[i] * std::exp(exp_term);
        double error = out[i].real() * out[i].imag();
        freq += (beta * error);
        // freq_log.push_back(freq * sampling_rate / (2 * M_PI));
        phase += freq + (alpha * error);
        while (phase >= 2 * M_PI){
            phase -= 2 * M_PI;
        } 
        while (phase < 0){
            phase += 2 * M_PI;
        }
        
    }
    return out;
}