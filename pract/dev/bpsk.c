#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <math.h>

double bpsk(int *bits, float i[], float q[]){
    for (int j=0; j<length(bits);j++){
        if (bits[j]==0){
            i[j]=-1;
            q[j]=0;
        } else{
            i[j]=1;
            q[j]=0;
        }
        printf("I = %lf, Q = %lf\t",i,q);
    }
    printf("\n");
}


int main(){
    int bits={1,0,0,1,0,1,0,1,0,0};
}