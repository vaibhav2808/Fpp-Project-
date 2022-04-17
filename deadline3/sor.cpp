#include "common.h"
#include "cotton.h"

#define ELEMENT_t float
#define OMEGA 1.25
#define THRESHOLD 2
ELEMENT_t* G;
 
void compute(int i, int size) {
    ELEMENT_t omega_over_four = OMEGA * 0.25;
    ELEMENT_t one_minus_omega = 1.0 - OMEGA;
    int Mm1 = size-1;
    int Nm1 = size-1;
    ELEMENT_t* Gi = &G[i*size];
    const int im1 = i-1;
    ELEMENT_t* Gim1 = &G[im1*size];
    const int ip1 = i+1;
    ELEMENT_t* Gip1 = &G[ip1*size];
    for (int j=1; j<Nm1; j++) {
        Gi[j] = omega_over_four * (Gim1[j] + Gip1[j] + Gi[j-1] + Gi[j+1]) + one_minus_omega * Gi[j];
    }
}

void parallel_kernel(int size) {
    irregular_recursion(1, size-1, THRESHOLD, [=] (int i) {
	compute(i, size);
    });
}

int main(int argc, char **argv) {
    PRINT_HARNESS_HEADER
    cotton::start_finish();
       int size = argc>1?atoi(argv[1]): 16384; //use multiples of PAGE_SIZE
       int iterations = argc>2?atoi(argv[2]): 50; 
       printf("SOR: size=%d, num_iterations=%d\n",size,iterations);
       G = new ELEMENT_t[size * size];
       initialization(0, size, G, true);
       printf("SOR started..\n");
       START_TIMER
       for (int p=0; p<iterations; p++) {
	   parallel_kernel(size);
       }
       END_TIMER
       printf("SOR ended..\n");
       delete[] G;
    cotton::end_finish();
    PRINT_HARNESS_FOOTER
    return 0;
}
