// #include "hclib_cpp.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include "cotton.h"
static long get_usecs (void)
{  
   struct timeval t;
   gettimeofday(&t,NULL);
   return t.tv_sec*1000000+t.tv_usec;
}

static long ___start_all, ___end_all;
static long ___start, ___end;
static double ___dur=0;

#define REPORT_TIME	{double _dur_ = ((double)(___end-___start))/1000000; printf("Kernel Execution Time = %fsec\n",_dur_); ___dur+=_dur_;}
#define PRINT_HARNESS_FOOTER { 		\
	___end_all = get_usecs();		\
	double ___dur_all = ((double)(___end_all-___start_all))/1000;		\
	printf("Harness Ended...\n");	\
	printf("============================ Statistics Totals ============================\n"); \
	printf("time.kernel\n");		\
	printf("%.3f\n",___dur*1000);		\
	printf("Total time: %.3f ms\n", ___dur_all);	\
	printf("------------------------------ End Statistics -----------------------------\n");	\
	printf("===== TEST PASSED in 0 msec =====\n");	\
}

#define PRINT_HARNESS_HEADER {                                            \
  printf("\n-----\nmkdir timedrun fake\n\n");                                 \
  ___start_all = get_usecs();						\
}
#define START_TIMER	{___start = get_usecs();}
#define END_TIMER	{___end = get_usecs();REPORT_TIME;}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda);

template<typename T>
void __divide4_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold && high-low>=4) {
	int chunk = (high - low) / 4;
        cotton::async([=]() {
            __divide2_1D(low, low+chunk, threshold, lambda);
	});
        cotton::async([=]() {
            __divide2_1D(low+chunk, low + 2 * chunk, threshold, lambda);
	});
        cotton::async([=]() {
            __divide4_1D(low + 2 * chunk, low + 3 * chunk, threshold, lambda);
	});
        cotton::async([=]() {
            __divide2_1D(low + 3 * chunk, high, threshold, lambda);
	});
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
	}
    }
}

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold) {
        cotton::async([=]() {
	    __divide4_1D(low, (low+high)/2, threshold, lambda);
	});
        cotton::async([=]() {
	    __divide2_1D((low+high)/2, high, threshold, lambda);
	});
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
	}
    }
}

template<typename T>
void irregular_recursion(int low, int high, int threshold, T && lambda) {
        __divide2_1D(low, high, threshold, lambda);
}

template<typename T>
void initialization(int low, int high, T* array1d, bool decimal) {
    int numWorkers=cotton::get_num_workers();
    assert(high%numWorkers == 0);
    int batchSize = high / numWorkers;
    int chunk=0;
    cotton::start_finish();
    for(int wid=0; wid<numWorkers; wid++) {
        int start = wid * batchSize;
        int end = start + batchSize;
        cotton::async([=]() {
	    unsigned int seed = wid+1;
            for(int j=start; j<end; j++) {
	        int num = rand_r(&seed);
	        array1d[j] = decimal? T(num/RAND_MAX) : T(num);
            }
        });
    }
   cotton::end_finish();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
