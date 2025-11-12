#include <SoapySDR/Device.h>   // Инициализация устройства
#include <SoapySDR/Formats.h>  // Типы данных, используемых для записи сэмплов
#include <stdio.h>             //printf
#include <stdlib.h>            //free
#include <stdint.h>
#include <complex.h>

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
SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 1, 10.0); // Чувствительность приемника
SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 1, -90.0);// Усиление передатчика

size_t channel_count = sizeof(channels) / sizeof(channels[0]);
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
    //заполнение tx_buff значениями сэмплов первые 16 бит - I, вторые 16 бит - Q.
    // for (int i = 0; i < 2 * tx_mtu; i+=2)
    // {
    //     // ЗДЕСЬ БУДУТ ВАШИ СЭМПЛЫ
    //     double t = (double)(i / 2) / tx_mtu * 2.0 - 1.0;
    //      double triangle_value = -(1.0 - fabs(t)) * (fabs(t) < 1.0);
    //      tx_buff[i] = (int16_t)(triangle_value * 16000);   // I - треугольник
    //      tx_buff[i+1] = (int16_t)(triangle_value * 16000); // Q = 0
    //  }

    //prepare fixed bytes in transmit buffer
    //we transmit a pattern of FFFF FFFF [TS_0]00 [TS_1]00 [TS_2]00 [TS_3]00 [TS_4]00 [TS_5]00 [TS_6]00 [TS_7]00 FFFF FFFF
    //that is a flag (FFFF FFFF) followed by the 64 bit timestamp, split into 8 bytes and packed into the lsb of each of the DAC words.
    //DAC samples are left aligned 12-bits, so each byte is left shifted into place
    // for(size_t i = 0; i < 2; i++)
    // {
    //     tx_buff[0 + i] = 0xffff;
    //     // 8 x timestamp words
    //     tx_buff[10 + i] = 0xffff;
    // }
const long  timeoutUs = 400000;
long long last_time = 0;
// Количество итерация чтения из буфера
size_t iteration_count = 100;
size_t total_rx_samples = 0;
size_t total_tx_samples = 0;

size_t sample_count = 100;
int16_t *my_file = read_pcm("/home/anastasia/Рабочий стол/sdr/pract/dev/music1.pcm", &sample_count);
int cur_sample_in_file = 0;

FILE *rx_file = fopen("received_data.pcm", "wb");  // Полученные данные
FILE *tx_file = fopen("transmitted_data.pcm", "wb"); // Переданные данные 
    



// Начинается работа с получением и отправкой сэмплов
for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++)
{
    void *rx_buffs[] = {rx_buffer};
    int flags;        // flags set by receive operation
    long long timeNs; //timestamp for receive buffer

    // считали буффер RX, записали его в rx_buffer
    int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
     if(sr > 0){
            size_t samples_written = fwrite(rx_buffer, sizeof(int16_t), 2 * sr, rx_file);
            total_rx_samples += sr;
        }

    // Смотрим на количество считаных сэмплов, времени прихода и разницы во времени с чтением прошлого буфера
    //printf("Buffer: %lu - Samples: %i, Flags: %i, Time: %lli, TimeDiff: %lli\n", buffers_read, sr, flags, timeNs, timeNs - last_time);
    

    for (int i = 0; i < 2 * tx_mtu; i += 2)
        {
            if (cur_sample_in_file < sample_count) 
            {
                tx_buff[i] = my_file[cur_sample_in_file];    
                tx_buff[i+1] = my_file[cur_sample_in_file + 1];  
                cur_sample_in_file+=2;
            }
            else
            {
                cur_sample_in_file = 0;
                tx_buff[i] = my_file[cur_sample_in_file];
                tx_buff[i+1] = my_file[cur_sample_in_file + 1];
                cur_sample_in_file += 2;
            }
        }
        fwrite(tx_buff, sizeof(int16_t), 2 * tx_mtu, tx_file);
        total_tx_samples += tx_mtu;
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
       
    if (st > 0) {
        printf("Transmitted: %d samples, total TX: %zu samples\n", st, total_tx_samples);
    } else if (st < 0) {
        printf("TX Error: %d\n", st);
    }

        // Прогресс каждые 10 итераций
    if (buffers_read % 10 == 0) {
        printf("Progress: buffer %lu/%lu, RX: %zu samples, TX: %zu samples\n", 
        buffers_read, iteration_count, total_rx_samples, total_tx_samples);
    }
    
    
}
    int s=ftell(rx_file);
    fclose(rx_file);
    fclose(tx_file);



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