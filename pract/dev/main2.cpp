#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <math.h>

int16_t *read_pcm(const char *filename, size_t *sample_count)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Cannot open file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("file_size = %ld\n", file_size);
    
    int16_t *samples = (int16_t *)malloc(file_size);
    if (!samples) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    *sample_count = file_size / sizeof(int16_t);
    size_t sf = fread(samples, sizeof(int16_t), *sample_count, file);

    if (sf != *sample_count) {
        printf("File read error: read %zu of %zu samples\n", sf, *sample_count);
        free(samples);
        fclose(file);
        return NULL;
    }

    fclose(file);
    printf("Successfully read %zu samples from file\n", *sample_count);
    return samples;
}

int main() {
    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    
    if (1) {
        SoapySDRKwargs_set(&args, "uri", "usb:");
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.1");
    }
    
    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");
    SoapySDRKwargs_set(&args, "loopback", "0");
    
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    if (!sdr) {
        printf("Failed to create SDR device\n");
        SoapySDRKwargs_clear(&args);
        return -1;
    }
    SoapySDRKwargs_clear(&args);

    int sample_rate = 1e6;
    int carrier_freq = 800e6;
    
    // Параметры RX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq, NULL);

    // Параметры TX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq, NULL);

    // Настройки усилителей
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, 10.0);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, -10.0);

    size_t channels[] = {0};
    size_t channel_count = sizeof(channels) / sizeof(channels[0]);
    
    // Формирование потоков
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    
    if (!rxStream || !txStream) {
        printf("Failed to setup streams\n");
        return -1;
    }

    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0);
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0);

    // Получение MTU
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);
    printf("RX MTU: %zu, TX MTU: %zu\n", rx_mtu, tx_mtu);

    // Выделение памяти под буферы
    int16_t *tx_buff = (int16_t *)malloc(2 * tx_mtu * sizeof(int16_t));
    int16_t *rx_buffer = (int16_t *)malloc(2 * rx_mtu * sizeof(int16_t));
    
    if (!tx_buff || !rx_buffer) {
        printf("Memory allocation failed for buffers\n");
        return -1;
    }

    // Загрузка аудиофайла для передачи
    size_t sample_count = 0;
    int16_t *my_file = read_pcm("/home/anastasia/Рабочий стол/sdr/pract/dev/music1.pcm", &sample_count);
    if (!my_file) {
        printf("Failed to read audio file\n");
        return -1;
    }

    int cur_sample_in_file = 0;
    
    // Файлы для записи полученных и переданных данных
    FILE *rx_file = fopen("received_data.pcm", "wb");  // Полученные данные
    FILE *tx_file = fopen("transmitted_data.pcm", "wb"); // Переданные данные (для отладки)
    
    if (!rx_file || !tx_file) {
        printf("Cannot create output files\n");
        return -1;
    }

    const long timeoutUs = 400000;
    long long last_time = 0;
    size_t iteration_count = 100;
    size_t total_rx_samples = 0;
    size_t total_tx_samples = 0;

    printf("Starting transmission and reception...\n");

    for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++)
    {
        void *rx_buffs[] = {rx_buffer};
        int flags;
        long long timeNs;

        // Чтение RX буфера - ЗАПИСЫВАЕМ ПОЛУЧЕННЫЕ ДАННЫЕ
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        if (sr > 0) {
            // КОРРЕКТНАЯ запись полученных данных
            size_t samples_written = fwrite(rx_buffer, sizeof(int16_t), 2 * sr, rx_file);
            total_rx_samples += sr;
            printf("Received: %d samples, total RX: %zu samples\n", sr, total_rx_samples);
        } else if (sr < 0) {
            printf("RX Error: %d\n", sr);
        }

        // Подготовка TX буфера с данными из файла
        for (size_t i = 0; i < 2 * tx_mtu; i += 2)
        {
            if (cur_sample_in_file < sample_count - 1) 
            {
                tx_buff[i] = my_file[cur_sample_in_file];     // I
                tx_buff[i+1] = my_file[cur_sample_in_file + 1]; // Q
                cur_sample_in_file += 2;
            }
            else
            {
                // Достигнут конец файла - начинаем сначала
                cur_sample_in_file = 0;
                tx_buff[i] = my_file[cur_sample_in_file];
                tx_buff[i+1] = my_file[cur_sample_in_file + 1];
                cur_sample_in_file += 2;
            }
        }

        // Записываем переданные данные в файл (для отладки)
        fwrite(tx_buff, sizeof(int16_t), 2 * tx_mtu, tx_file);
        total_tx_samples += tx_mtu;

        // Передача с временной задержкой
        long long tx_time = timeNs + (10000 * 1000); // 10ms в будущее

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

    // Очистка ресурсов
    fclose(rx_file);
    fclose(tx_file);
    free(tx_buff);
    free(rx_buffer);
    free(my_file);

    // Остановка потоков
    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);

    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_closeStream(sdr, txStream);
    SoapySDRDevice_unmake(sdr);

    printf("Program completed successfully\n");
    printf("Total samples received: %zu (saved to received_data.pcm)\n", total_rx_samples);
    printf("Total samples transmitted: %zu (saved to transmitted_data.pcm)\n", total_tx_samples);

    return 0;
}