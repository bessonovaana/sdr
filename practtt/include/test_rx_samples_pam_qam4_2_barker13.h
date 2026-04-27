// _pam_qam4_2
// 1. Код Баркера
// std::cout << "Формируем код Баркера (13 бит)" << std::endl;
// std::vector<int> barker_real = {1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1};
// std::vector<int> barker_imag = {1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1};
// std::vector<std::complex<double>> barker_complex;
// for(int i = 0; i < barker_real.size(); i++){
//     barker_complex.push_back(std::complex(barker_real[i] * 3.0f, barker_imag[i] * 3.0f));
//     //std::cout << barker_complex[i] << " ";
// }
// // std::cout << std::endl;

// // 2. "0X00Hello from user10X00" в ASCII\UTF-8 - 104 бита
// std::vector<int> hello_sibguti = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 
//     1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 
//     0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 
//     1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 
//     1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 
//     0, 0, 0, 1, 1, 0, 0, 0, 0};
// std::cout << "text len = " << hello_sibguti.size() << std::endl;

// // 3. Модуляция 4_2_QAM
// std::vector<int> pam_4_data;
// pam_4(hello_sibguti, pam_4_data);
// std::cout << "pam_4_data = " << pam_4_data.size() << std::endl;

// std::vector<std::complex<double>> modulated_data;
// pam_4_to_qam_4_2(pam_4_data, modulated_data);
// // 3. Модуляция QPSK
// // std::vector<std::complex<double>> modulated_data = modulate(hello_sibguti, 1);

// // 4. Объединяем Код Баркера с текстом (после модуляции)
// std::vector<std::complex<double>> frame_data;
// frame_data.reserve(barker_complex.size() + modulated_data.size());
// frame_data.insert(frame_data.end(), barker_complex.begin(), barker_complex.end());
// frame_data.insert(frame_data.end(), modulated_data.begin(), modulated_data.end());

// std::vector<std::complex<double>> summ_vector;
// for (int i = 0; i < 2; i++)
// {
//     summ_vector.insert(summ_vector.end(), frame_data.begin(), frame_data.end());
// }
// // std::cout << "frame_data = " << summ_vector.size() << std::endl;
// // for (int i = 0; i < frame_data.size();i++){
// //     std::cout << frame_data[i] << " ";
// // }
// // std::cout << std::endl;

// // 5. Апсемплинг и Формирующий фильтр
// int nsps = sdr->phy.Nsps;
// int syms = 5;
// double beta = 0.75;
// std::vector<std::complex<double>> upsampled = upsample(summ_vector, nsps);
// std::vector<double> filter = srrc(syms, beta, nsps, 0.0f);
// sdr->test_rx_sdr.pulse_shaped = convolve(upsampled, filter);
// for (int i = 0; i < sdr->test_rx_sdr.pulse_shaped.size(); i++)
// {
//     sdr->test_rx_sdr.samples_to_tx.push_back({  int(sdr->test_rx_sdr.pulse_shaped[i].real() * 2000) << 4, 
//                                                 int(sdr->test_rx_sdr.pulse_shaped[i].imag() * 2000) << 4});
// }
// std::cout << "pulse size = " << sdr->test_rx_sdr.pulse_shaped.size() << std::endl;
#include "main.h"
#pragma once
extern std::vector<std::complex<float>> test_rx_samples;