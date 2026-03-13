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

#include "../third_party/imgui/backends/imgui_impl_opengl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdl2.h"
#include "../include/imgui.h"
#include "../include/implot.h"

#include "../include/mapper.h"



using namespace std;

struct Sharing{
    vector<complex<float>>tx_samples;
    vector<complex<float>>rx_samples;
    atomic<bool> program_running{true};
    atomic<bool>sdr_ready{false};
    mutex mtx;
    vector<float> sync;
};

Sharing shared;

struct IQData {
    vector<double> real;  // I компонента (синий)
    vector<double> imag;  // Q компонента (красный)
    vector<double> count; // Индексы для оси X
};

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

void run_gui(){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        "Backend start", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    glewInit();

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Включить Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Включить Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Включить Docking

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);



        IQData iq;
        {
            lock_guard<mutex> lock(shared.mtx);
            if (!shared.rx_samples.empty()) {
                iq = extractIQ(shared.rx_samples);
            }
        }

ImGui::Begin("IQ Signals");
if (ImPlot::BeginPlot("I/Q Time Domain")) {
                ImPlot::PlotLine("I", iq.count.data(), iq.real.data(), iq.real.size());
                ImPlot::PlotLine("Q", iq.count.data(), iq.imag.data(), iq.imag.size());
                ImPlot::EndPlot();
            }
            ImGui::End();
            ImGui::Begin("I/Q");
            if (ImPlot::BeginPlot("Scatter")){
                ImPlot::PlotScatter("IQ Data", iq.real.data(), iq.imag.data(), iq.real.size());
                ImPlot::EndPlot();
            }
            ImGui::End();
        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
        
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
}

int16_t *read_pcm(const char *filename, size_t *sample_count)
{
    FILE *file = fopen(filename, "rb");

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("file_size = %ld\n", file_size);
    int16_t *samples = (int16_t *)malloc(file_size);

    *sample_count = file_size / sizeof(int16_t);

    size_t sf = fread(samples, sizeof(int16_t), *sample_count, file);

    if (sf == 0){
        printf("file %s empty!", filename);
    }

    fclose(file);

    return samples;
}

std::vector<int16_t> generate_bits (int size){
    
    std::vector<int16_t> bits(size);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 1);
    
    for (int i = 0; i < size; i++) {
        bits[i] = dis(gen);
        
    }
    
    return bits;
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
// std::vector<std::complex<float>> bpsk (std::vector<int16_t>in){
//    std::vector<std::complex<float>>out;
//     for (int i=0; i<in.size(); i++){
//         out.push_back((1/sqrt(2))* (1 - 2*in[i])),
//         ((1/sqrt(2))*(1-2*in[i]));
//     }
//     return out;
// }

vector<complex<float>> modulate(const vector<complex<float>>& in, int step) {
    vector<complex<float>> upbits;
    upbits.reserve(in.size() * (step + 1)); 
    
    for (const auto& val : in) {
        upbits.push_back(val);           
        for (int i = 0; i < step; i++) {
            upbits.push_back(0.0f);      
        }
    }
    
    return upbits;
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

void sdr_work(){
SoapySDRKwargs args = {};

    SoapySDRKwargs_set(&args, "driver", "plutosdr");        // Говорим какой Тип устройства 
    if (1) {
        SoapySDRKwargs_set(&args, "uri", "usb:");           // Способ обмена сэмплами (USB)
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.1"); // Или по IP-адресу
    }
    SoapySDRKwargs_set(&args, "direct", "1");               // 
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");   // Размер буфера + временные метки
    SoapySDRKwargs_set(&args, "loopback", "0");             // Используем антенны или нет
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);       // Инициализация
    SoapySDRKwargs_clear(&args);

    int sample_rate = 1e6;
    int carrier_freq = 900e6;
    // Параметры RX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq , NULL);

    // Параметры TX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq , NULL);

    // Инициализация количества каналов RX\\TX (в AdalmPluto он один, нулевой)
    size_t channels[] = {0};
    // Настройки усилителей на RX\\TX
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, 50.0); // Чувствительность приемника
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, 60.0);// Усиление передатчика

    int channel_count = 1;
    // Формирование потоков для передачи и приема сэмплов
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);

    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); //start streaming
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0); //start streaming
    // Получение MTU (Maximum Transmission Unit), в нашем случае - размер буферов. 
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);


    // Выделяем память под буферы RX и TX
    int16_t tx_buff[2*tx_mtu];
    int16_t rx_buffer[2*rx_mtu];
    int16_t rx_cbuffer[2*rx_mtu];

    vector<int16_t> bits = generate_bits(1000);
    vector<complex<float>>bbits=bpsk(bits);

    

    auto upsampled = modulate(bbits, 9);

    cout<<upsampled.size()<<endl;

    auto filtred = convolve(upsampled);


    to_file(filtred, tx_buff, 2 * tx_mtu);
    
        //prepare fixed bytes in transmit buffer
        //we transmit a pattern of FFFF FFFF [TS_0]00 [TS_1]00 [TS_2]00 [TS_3]00 [TS_4]00 [TS_5]00 [TS_6]00 [TS_7]00 FFFF FFFF
        //that is a flag (FFFF FFFF) followed by the 64 bit timestamp, split into 8 bytes and packed into the lsb of each of the DAC words.
        //DAC samples are left aligned 12-bits, so each byte is left shifted into place
        for(size_t i = 0; i < 2; i++)
        {
            tx_buff[0 + i] = 0xffff;
        // 8 x timestamp words
            tx_buff[10 + i] = 0xffff;
        }


    const long  timeoutUs = 400000;
    long long last_time = 0;


    FILE *txfile = fopen("../tx.pcm", "wb");
    FILE *rxfile = fopen("../rx.pcm", "wb");

    //заполнение файла на передачу
    fwrite(tx_buff, sizeof(int16_t), 2 * tx_mtu, txfile);
    
    // Количество итерация чтения из буфера
    size_t iteration_count = 100;
    size_t total_rx_samples = 0;
    size_t total_tx_samples = 0;

    size_t sample_count = 100;

    int cur_sample_in_file = 0;

    

    // Начинается работа с получением и отправкой сэмплов
   //while (shared.program_running){
   for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++){
        
        void *rx_buffs[] = {rx_buffer};
        int flags;        // flags set by receive operation
        long long timeNs; //timestamp for receive buffer

        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        if (sr>0){
        vector<complex<float>> f = from_file(rx_buffer, 2u*sr);
        auto filtered1 = convolve(f);  
        auto sync_indices = offset(filtered1); 
        
        vector<complex<float>> synced_samples;

        for (float idx : sync_indices) {
            if (idx < filtered1.size()) {
                synced_samples.push_back(filtered1[idx]); 
            }
        }

        
        {
        lock_guard<mutex> lock(shared.mtx);
        shared.rx_samples = std::move(synced_samples);
        }
        to_file(shared.rx_samples,rx_cbuffer, 2u*sr);
    }
        
        

        size_t samples_written = fwrite(rx_cbuffer, sizeof(int16_t), 2 * sr, rxfile);
               
           

        // Смотрим на количество считаных сэмплов, времени прихода и разницы во времени с чтением прошлого буфера
        //printf("Buffer: %lu - Samples: %i, Flags: %i, Time: %lli, TimeDiff: %lli\n", buffers_read, sr, flags, timeNs, timeNs - last_time);
        

    
        // Переменная для времени отправки сэмплов относительно текущего приема
        long long tx_time = timeNs + (4 * 1000 * 1000); // на 4 [мс] в будущее

        // Добавляем время, когда нужно передать блок tx_buff, через tx_time -наносекунд
        for(size_t i = 0; i < 8; i++)
        {
            uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
            tx_buff[2 + i] = tx_time_byte << 4;
        }

        // Здесь отправляем наш tx_buff массив
        void *tx_buffs[] = {tx_buff};
        int tx_flags = SOAPY_SDR_HAS_TIME;
        int st = SoapySDRDevice_writeStream(sdr, txStream, tx_buffs, tx_mtu, &tx_flags, tx_time, timeoutUs);
        
        //this_thread::sleep_for(chrono::milliseconds(1000));
        
    }
        
        fclose(rxfile);
        fclose(txfile);




    //stop streaming
    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);

    //shutdown the stream
    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_closeStream(sdr, txStream);

    //cleanup device handle
    SoapySDRDevice_unmake(sdr);

   
}
int main(){
    thread sdr_thread(sdr_work);  // Все используют глобальную shared_data
    run_gui();

    shared.program_running = false;
    sdr_thread.join();
    return 0;
}