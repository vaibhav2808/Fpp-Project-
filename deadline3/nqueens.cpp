/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/*                                                                                            */
/*  This program is free software; you can redistribute it and/or modify                      */
/*  it under the terms of the GNU General Public License as published by                      */
/*  the Free Software Foundation; either version 2 of the License, or                         */
/*  (at your option) any later version.                                                       */
/*                                                                                            */
/*  This program is distributed in the hope that it will be useful,                           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             */
/*  GNU General Public License for more details.                                              */
/*                                                                                            */
/*  You should have received a copy of the GNU General Public License                         */
/*  along with this program; if not, write to the Free Software                               */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            */
/**********************************************************************************************/

/*
 * Original code from the Cilk project (by Keith Randall)
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
 */

/**
 * Nqueens was ported from the BOTS nqueens.c benchmark.  See below for provenance.
 * This program computes all solutions to the n-queens problem where n is specified in argv[1] (default = 11)
 * The program uses the count of the total number of solutions as a correctness check and also prints the execution time
 * for each repetition. 
 * <p>
 * Note the use of single "finish" statement in main() that awaits termination of all async's created by the
 * recursive calls to nqueens_kernel.
 * <p>
 * To study scalability on a multi-core processor, you can execute "COTTON_WORKERS=<N> ./nqueens 11" by 
 * varying the number of worker (=N) threads.
 *
 * @author Jun Shirako, Rice University
 * @author Vivek Sarkar, Rice University
 *
 * Modifed by Vivek Kumar for CSE 502 at IIITD
 *
 */

#include "cotton.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

// Solutions for different board sizes
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

/*
 * <a> contains array of <n> queen positions.  Returns 0
 * if none of the queens conflict, and returns 1 otherwise.
 */
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
    // atomic increment using gcc inbuilt atomics
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
        int a=1;
        cotton::async([=]() {
                nqueens_kernel(B, depth+1, size);
        },(size/depth)*10000000);
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

long get_usecs (void)
{
   struct timeval t;
   gettimeofday(&t,NULL);
   return t.tv_sec*1000000+t.tv_usec;
}

int main(int argc, char* argv[])
{
  cotton::init_runtime();
  int n = 11;
  int i, j;
     
  if(argc > 1) n = atoi(argv[1]);
     
  double dur = 0;
  int* a = (int*) malloc(sizeof(int));
  atomic = (int*) malloc(sizeof(int));;
  atomic[0]=0;
  // Timing for parallel run
  long start = get_usecs();

  cotton::start_finish();
  nqueens_kernel(a, 0, n);  
  cotton::end_finish();

  // Timing for parallel run
  long end = get_usecs();
  dur = ((double)(end-start))/1000000;
  verify_queens(n);  
  free((void*)atomic);
  printf("NQueens(%d) Time = %fsec\n",n,dur);

  cotton::finalize_runtime();
  return 0;
}
