#include <math.h>
#include <stdio.h>
#include <stdint.h>


int main(void) {
    puts("Tune table:");
    printf("const uint32_t tune_table[] = {\n\t");
    for (int i=0; i<128; i++) {
        if (i > 0 && !(i % 12))
            printf("\n\t");
        printf("%ld, ", (uint32_t) ceil((pow(2.,(i-69.)/12.)*1000)));
    }
    puts("};");
    
    puts("Min/max frequencies per block:\n");
    int fs = 49716;
    for (int block=0; block<8; block++)
    {
        double factor = pow(2,19)/fs/pow(2,(block-1)); 
        double fmin = 1./factor;
        double fmax = 1023./factor;
        printf("\t{%ld, %ld},\n", (uint32_t)(fmin*10000.), (uint32_t)(fmax*10000.));
        //printf("Block %i: fmin:%f  fmax:%f\n", block, fmin, fmax);
    }
}