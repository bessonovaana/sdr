#include <SoapySDR/Device.h>   // Инициализация устройства
#include <SoapySDR/Formats.h>  // Типы данных, используемых для записи сэмплов
#include <stdio.h>             //printf
#include <stdlib.h>            //free
#include <stdint.h>
#include <vector>
#include <random>
#include <complex>
#include <iostream>
#include <thread>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <chrono>
#include <atomic>
#include <mutex>
#include <cmath>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <numbers> 


#include "../third_party/imgui/backends/imgui_impl_opengl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdl2.h"
#include "../include/imgui.h"
#include "../include/implot.h"

#include "../include/gui.h"
#include "../include/main.h"
#include "../include/mapper.h"
#include "../include/tx.h"
#include "../include/rx.h"
#include "../include/test_rx_samples_pam_qam4_2_barker13.h"


using namespace std;


Sharing shared;

typedef struct sdr_global_s{
    bool running;
    SoapySDRDevice *sdr;
    SoapySDRStream *rxStream;
    SoapySDRStream *txStream;
} sdr_global_t;


IQData extractIQ(const vector<complex<float>>& samples) {
    IQData iq;
    size_t n = samples.size();
    
    iq.real.reserve(n);
    iq.imag.reserve(n);
    iq.count.reserve(n);
    
    for (size_t i = 0; i < n; i++) {
        iq.count.push_back((double)i);           
        iq.real.push_back((double)samples[i].real());   
        iq.imag.push_back((double)samples[i].imag()); 
        //cout<<iq.real[i]<<endl;  
    }
    return iq;
}



vector<complex<float>> convolve(const vector<complex<float>>& x) {
    
    vector<complex<float>> h = {0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f};
    
    int N = x.size();
    int M = h.size();
    int result_size = N + M - 1;

    vector<complex<float>> y(result_size, 0.0f);
    
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




void to_file(vector<complex<float>>in, int16_t buff[], int size){
        int k =0;
        //заполнение tx_buff значениями сэмплов первые 16 бит - I, вторые 16 бит - Q.
        for (int i = 0; i < size; i+=2)
        {
            buff[i]=(int16_t)(in[k].real()*16000.0f);
            buff[i+1]=(int16_t)(in[k].imag()*16000.0f);
      k++;
      
        //     // ЗДЕСЬ БУДУТ ВАШИ СЭМПЛЫ
        //     double t = (double)(i / 2) / tx_mtu * 2.0 - 1.0;
        //      double triangle_value = -(1.0 - fabs(t)) * (fabs(t) < 1.0);
        //      tx_buff[i] = (int16_t)(triangle_value * 16000);   // I - треугольник
        //      tx_buff[i+1] = (int16_t)(triangle_value * 16000); // Q = 0
        }
        for (int i=k; i<size;i++){
            buff[i]=0;
        }
}

vector<complex<float>> from_file(int16_t buff[], int size){
    vector<complex<float>> sv2;
    sv2.reserve(size / 2);
    for (int i=0; i<size; i+=2){
        float real_part = (float)buff[i]/16000;
        float imag_part = (float)buff[i+1]/16000;

        sv2.push_back(complex<float>(real_part,imag_part));
    }
    return sv2;
}



void rx(){
    // const char* uri = "usb:1.8.5";
    double freq_MHz = 700.0;
    double samp_rate = 1e6;
    double rx_gain = 50.0;
    
   
    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    SoapySDRKwargs_set(&args, "uri", "usb:");
    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");
    SoapySDRKwargs_set(&args, "loopback", "0");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

    double sample_rate = samp_rate;
    double carrier_freq = freq_MHz * 1e6;
    
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq, NULL);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, rx_gain);

    size_t channels[] = {0};
    int channel_count = 1;
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0);

    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    extern std::vector<std::complex<float>> test_rx_samples;
    size_t num_samples = test_rx_samples.size();
    std::vector<int16_t> rx_buffer(2 * rx_mtu);
    const long timeoutUs = 400000;
    for (size_t buffers_read = 0; buffers_read < 100; buffers_read++){
    //while (shared.program_running) {
        void *rx_buffs[] = {rx_buffer.data()};
        int flags;
        long long timeNs;

        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        if (sr > 0) {
            vector<complex<float>> f = from_file(rx_buffer.data(), 2 * 1920);
            auto filtered1 = convolve(f);  
            auto sync_sam = symbol_sync(filtered1); 
            auto freq_sam = freq_synq(sync_sam, carrier_freq, sync_sam.size(), (int)sample_rate);
            
            {
                lock_guard<mutex> lock(shared.mtx);
                shared.rx_samples = std::move(freq_sam);
            }
        }
    }

    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_unmake(sdr);
}
void rx_test() {
    double carrier_freq = 700.0 * 1e6;
    double sample_rate = 1e6;
    const size_t CHUNK_SIZE = 1920;
    size_t offset = 0;
    size_t total_samples = test_rx_samples.size();

    // Опционально: очистить буферы перед началом, если функция может вызываться повторно
    {
        std::lock_guard<std::mutex> lock(shared.mtx);
        shared.rx_samples_raw.clear();
        shared.rx_samples_fil.clear();
        shared.rx_samples_sync_time.clear();
        shared.rx_samples_freq.clear();
        
        // Оптимизация: зарезервировать память под весь сигнал (избегаем частых реаллокаций)
        shared.rx_samples_raw.reserve(total_samples);
        shared.rx_samples_fil.reserve(total_samples);
        shared.rx_samples_sync_time.reserve(total_samples); // после sync может быть меньше!
        shared.rx_samples_freq.reserve(total_samples);
    }

    while (shared.program_running && offset < total_samples) {
        size_t chunk_size = std::min(CHUNK_SIZE, total_samples - offset);
        
        // Берем данные
        std::vector<std::complex<float>> chunk(
            test_rx_samples.begin() + offset,
            test_rx_samples.begin() + offset + chunk_size
        );
        offset += chunk_size;

        auto filtered = convolve(chunk);
        auto sync_sam = symbol_sync(filtered); 
        auto freq_sam = costas_loop_bpsk(sync_sam);

        {
            std::lock_guard<std::mutex> lock(shared.mtx);
            // Добавляем данные в конец векторов, а не перезаписываем
            shared.rx_samples_raw.insert(shared.rx_samples_raw.end(), 
                                         std::make_move_iterator(chunk.begin()),
                                         std::make_move_iterator(chunk.end()));
            shared.rx_samples_fil.insert(shared.rx_samples_fil.end(), 
                                         std::make_move_iterator(filtered.begin()),
                                         std::make_move_iterator(filtered.end()));
            shared.rx_samples_sync_time.insert(shared.rx_samples_sync_time.end(), 
                                               std::make_move_iterator(sync_sam.begin()),
                                               std::make_move_iterator(sync_sam.end()));
            shared.rx_samples_freq.insert(shared.rx_samples_freq.end(), 
                                          std::make_move_iterator(freq_sam.begin()),
                                          std::make_move_iterator(freq_sam.end()));
            // shared.rx_samples_freq = std::move(freq_sam);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
void tx() {
    // const char* uri = "usb:1.7.5"; 
    
    double freq_MHz = 700.0;
    double samp_rate = 1e6;
    double tx_gain = 50.0;
    

    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    SoapySDRKwargs_set(&args, "uri", "usb:");
    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");
    SoapySDRKwargs_set(&args, "loopback", "0");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

    double sample_rate = samp_rate;
    double carrier_freq = freq_MHz * 1e6;
    
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq, NULL);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, tx_gain);

    size_t channels[] = {0};
    int channel_count = 1;
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0);
    
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);
    int16_t tx_buff[2*tx_mtu];

    vector<int16_t> bits = generate_bits(500);
    
    bits.insert(bits.begin(), barker7.begin(), barker7.end()); 
    vector<complex<float>> bbits = qpsk(bits);
    auto upsampled = modulate(bbits, 9);
    auto filtred = convolve(upsampled);
    to_file(filtred, tx_buff, 2 * tx_mtu);

    const long timeoutUs = 400000;
    
//for (size_t buffers_read = 0; buffers_read < 100; buffers_read++){
   while (shared.program_running) {
        
        long long tx_time = shared.tx_timestamp.load();
        
        if (tx_time == 0) {
            tx_time = 1000000000LL;  
        }
        
       
        shared.tx_timestamp.store(tx_time + (long long)(tx_mtu * 1e9 / sample_rate));
        
     
        for (size_t i = 0; i < 8; i++) {
            uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
            tx_buff[2 + i] = tx_time_byte << 4;
        }

        void *tx_buffs[] = {tx_buff};
        int tx_flags = SOAPY_SDR_HAS_TIME;
       
    }

    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);
    SoapySDRDevice_closeStream(sdr, txStream);
    SoapySDRDevice_unmake(sdr);
}


int main() {

        shared.tx_timestamp.store(0);
        shared.program_running.store(true);
        
        
        thread rx_thread([&]() {
            
            rx_test();
        });
        
    
        this_thread::sleep_for(chrono::seconds(2));
        // thread tx_thread([&]() {
            
        //     tx();
        // });
        
        
        thread gui_thread(run_gui);
        
        
        rx_thread.join();
        // tx_thread.join();
        gui_thread.join();
    
    return 0;
}