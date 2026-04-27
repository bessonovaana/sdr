#include "tx.h"


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