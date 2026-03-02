#include <SoapySDR/Device.h>   // Инициализация устройства
#include <SoapySDR/Formats.h>  // Типы данных, используемых для записи сэмплов
#include <stdio.h>             //printf
#include <stdlib.h>            //free
#include <stdint.h>
#include <vector>
#include <random>
#include <complex.h>
#include<iostream>


using namespace std;

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
    
    vector<complex<float>> h = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    
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
void bpsk (std::vector<int16_t>in, std::vector<std::complex<float>>&out){
    
    std::complex<float> val;
    for (int i=0; i<in.size();i++){
        if (in[i]==1){
            val = 1.0 + 0.0f;
        }
        else{
            val = -1.0 + 0.0f;
        }
        out.push_back(val);
    }

}

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
            buff[i]=(int16_t)(in[k].real()*1600);
            buff[i+1]=(int16_t)(in[k].imag()*1600);
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
    for (int i=0; i<size-1; i+=2){
        float real_part = (float)buff[i]/1600.0f;
        float imag_part = (float)buff[i+1]/1600.0f;

        sv2.push_back(complex<float>(real_part,imag_part));
    }
    return convolve(sv2);
}

vector<complex<float>> sim_sync(std::vector<complex<float>> y){
    
    auto offset = 0;
    const int Nsp =10;

    float BnTs = 0.01;
    float zeta = sqrt(2) / 2;
    float Kp = 0.01;
    
    float teta =(BnTs/Nsp)/(zeta+(1/4*zeta));
    float K1=(-4*zeta*teta)/(1 + 2 * teta*zeta + pow(teta,2))*Kp;
    float K2=(-4*teta*teta)/(1 + 2 * teta*zeta + pow(teta,2))*Kp;
    for (int ns =0; ns<y.size(); ns+=10){
        auto n = offset;
        float real_err = (y[ns+n].real()-y[n+ns+Nsp].real()) * y[n+(Nsp)/2+ns].real();
        float imag_err = (y[ns+n].imag()-y[n+ns+Nsp].imag()) * y[n+(Nsp)/2+ns].imag();
        float error = imag_err+real_err;

        float p1 = error*K1;
        float p2 = p2 + p1 + error*K2;

        if (p2>1) {p2=p2 - 1;}
        if (p2<1 ){p2+=1;}

        int offset =round(p2*Nsp);
    }

}
int main(){
    
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
    int carrier_freq = 800e6;
    // Параметры RX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq , NULL);

    // Параметры TX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq , NULL);

    // Инициализация количества каналов RX\\TX (в AdalmPluto он один, нулевой)
    size_t channels[] = {0};
    // Настройки усилителей на RX\\TX
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 1, 50.0); // Чувствительность приемника
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 1, -10.0);// Усиление передатчика

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

    vector<int16_t> bits = generate_bits(100);
    vector<complex<float>>bbits;
    
    bpsk(bits, bbits);

    

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
    for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++)
    {
        void *rx_buffs[] = {rx_buffer};
        int flags;        // flags set by receive operation
        long long timeNs; //timestamp for receive buffer

        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        vector<complex<float>> f = from_file(rx_buffer, 2*sr);
        to_file(f,rx_cbuffer, 2*sr);
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
        

        // Прогресс каждые 10 итераций
        if (buffers_read % 10 == 0) {
            printf("Progress: buffer %lu/%lu, RX: %zu samples, TX: %zu samples\n", 
            buffers_read, iteration_count, total_rx_samples, total_tx_samples);
        }
        
        
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

    return 0;
}