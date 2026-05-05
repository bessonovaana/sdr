#include"rx.h"
#include <iostream>

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