/*
 * Copyright 2017 Rice University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "cotton.h"
int solutions[16] =
{
1,
0,
0,
2,
10, /* 5 */
4,
40,
92,
352,
724, /* 10 */
2680,
14200,
73712,
365596,
2279184, 
14772512
};

volatile int *atomic;

int ok(int n,  int* A) {
  int i, j;
  for (i =  0; i < n; i++) {
    int p = A[i];

    for (j =  (i +  1); j < n; j++) {
      int q = A[j];
      if (q == p || q == p - (j - i) || q == p + (j - i))
      return 1;
    }
  }
  return 0;
}

void nqueens_kernel(int* A, int depth, int size) {
  if (size == depth) {
    __sync_fetch_and_add(atomic, 1);
    return;
  }
  /* try each possible position for queen <depth> */
  for(int i=0; i<size; i++) {
      /* allocate a temporary array and copy <a> into it */
      int* B = (int*) malloc(sizeof(int)*(depth+1));
      memcpy(B, A, sizeof(int)*depth);
      B[depth] = i;
      int failed = ok((depth +  1), B); 
      if (!failed) {
	cotton::async([=]() {
        nqueens_kernel(B, depth+1, size);
	});
      }
  }
  free(A);
}

void verify_queens(int size) {
  if ( *atomic == solutions[size-1] )
    printf("OK\n");
   else
    printf("Incorrect Answer\n");
}

int main(int argc, char* argv[])
{
  PRINT_HARNESS_HEADER
  cotton::init_runtime();
      int n = 13;
      int i, j;
         
      if(argc > 1) n = atoi(argv[1]);
         
      int* a = (int*) malloc(sizeof(int));
      atomic = (int*) malloc(sizeof(int));;
      atomic[0]=0;
      START_TIMER
      cotton::start_finish();
          nqueens_kernel(a, 0, n);  
      cotton::end_finish();
      
      END_TIMER
      verify_queens(n);  
      free((void*)atomic);
      printf("NQueens(%d) done\n",n);
  cotton::finalize_runtime();
  PRINT_HARNESS_FOOTER
  return 0;
}
