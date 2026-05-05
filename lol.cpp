void rx(int argc, char** argv){
        // Парсинг аргументов
    const char* uri = "usb:";
    double freq_MHz = 700.0;
    double samp_rate = 1e6;
    double rx_gain = 50.0;
    
    // Обработка аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0 && i+1 < argc) {
            uri = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0 && i+1 < argc) {
            freq_MHz = atof(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i+1 < argc) {
            samp_rate = atof(argv[++i]);
        } else if (strcmp(argv[i], "-g") == 0 && i+1 < argc) {
            rx_gain = atof(argv[++i]);
        }
    }

    SoapySDRKwargs args = {};

    SoapySDRKwargs_set(&args, "driver", "plutosdr");        // Говорим какой Тип устройства 
   
    SoapySDRKwargs_set(&args, "uri");           // Способ обмена сэмплами (USB)
 
    SoapySDRKwargs_set(&args, "direct", "1");               // 
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");   // Размер буфера + временные метки
    SoapySDRKwargs_set(&args, "loopback", "0");             // Используем антенны или нет
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);       // Инициализация
    SoapySDRKwargs_clear(&args);

    int sample_rate = 1e6;
    int carrier_freq = 700e6;
    // Параметры RX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq , NULL);

    size_t channels[] = {0};
    // Настройки усилителей на RX\\TX
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, 50.0); // Чувствительность приемника
        int channel_count = 1;
    // Формирование потоков для передачи и приема сэмплов
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); //start streaming

    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    int16_t rx_buffer[2*rx_mtu];
    int16_t rx_cbuffer[2*rx_mtu];

    const long  timeoutUs = 400000;
    long long last_time = 0;
    size_t iteration_count = 100;
    size_t total_rx_samples = 0;
    size_t total_tx_samples = 0;
 //while (shared.program_running){
   for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++){
        
        void *rx_buffs[] = {rx_buffer};
        int flags;        // flags set by receive operation
        long long timeNs; //timestamp for receive buffer

        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        if (sr>0){
        vector<complex<float>> f = from_file(rx_buffer, 2*sr);
        auto filtered1 = convolve(f);  
        auto sync_sam = symbol_sync(filtered1); 
        auto freq_sam = freq_synq(sync_sam, carrier_freq, sync_sam.size(),sample_rate);
        
        {
            lock_guard<mutex> lock(shared.mtx);
        shared.rx_samples = std::move(freq_sam);
        }
      
    }
       

        // Смотрим на количество считаных сэмплов, времени прихода и разницы во времени с чтением прошлого буфера
        //printf("Buffer: %lu - Samples: %i, Flags: %i, Time: %lli, TimeDiff: %lli\n", buffers_read, sr, flags, timeNs, timeNs - last_time);
        
      
        //this_thread::sleep_for(chrono::milliseconds(1000));
        
    }
        


    //stop streaming
    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
   

    //shutdown the stream
    SoapySDRDevice_closeStream(sdr, rxStream);
 
    //cleanup device handle
    SoapySDRDevice_unmake(sdr);

   
}


void tx(int argc, char** argv)
{
    const char* uri = "usb:";
    double freq_MHz = 700.0;
    double samp_rate = 1e6;
    double tx_gain = 50.0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0 && i+1 < argc) uri = argv[++i];
        else if (strcmp(argv[i], "-f") == 0 && i+1 < argc) freq_MHz = atof(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i+1 < argc) samp_rate = atof(argv[++i]);
        else if (strcmp(argv[i], "-g") == 0 && i+1 < argc) tx_gain = atof(argv[++i]);
    }
    SoapySDRKwargs args = {};

    SoapySDRKwargs_set(&args, "driver", "plutosdr");        // Говорим какой Тип устройства 
   
    SoapySDRKwargs_set(&args, "uri");           // Способ обмена сэмплами (USB)
 
    SoapySDRKwargs_set(&args, "direct", "1");               // 
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");   // Размер буфера + временные метки
    SoapySDRKwargs_set(&args, "loopback", "0");             // Используем антенны или нет
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);       // Инициализация
    SoapySDRKwargs_clear(&args);

    int sample_rate = 1e6;
    int carrier_freq = 700e6;
    // Параметры TX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq , NULL);

    size_t channels[] = {0};
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, 50.0);// Усиление передатчика
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0); //start streaming
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);
    int16_t tx_buff[2*tx_mtu];

    vector<int16_t> preambule = generate_bits(10);
    vector<int16_t> bits = generate_bits(500);
   vector<complex<float>>bbits=qpsk(bits);

    

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

      //while (shared.program_running){
   for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++){
        
     
        long long timeNs; //timestamp for receive buffer


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
      //stop streaming
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);
        SoapySDRDevice_closeStream(sdr, txStream);

    //cleanup device handle
    SoapySDRDevice_unmake(sdr);
}