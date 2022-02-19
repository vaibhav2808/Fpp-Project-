#include<functional>
#include<stdio.h>
namespace stamp{
    // accepts two C++11 lambda functions and runs the them in parallel
    void execute_tuple(std::function<void()> &&lambda1, std::function<void()> &&lambda2);
    // parallel_for accepts a C++11 lambda function and runs the loop body (lambda) in
    // parallel by using ‘numThreads’ number of Pthreads created inside StaMp
    void parallel_for(int low, int high, int stride, std::function<void(int)> &&lambda, int numThreads);
    // Shorthand for using parallel_for when lowbound is zero and stride is one.
    void parallel_for(int high, std::function<void(int)> &&lambda, int numThreads);
    // This version of parallel_for is for parallelizing two-dimensional for-loops, i.e., an outter for-i loop and
    // an inner for-j loop. Loop properties, i.e. low, high, and stride are mentioned below for both outter
    // and inner for-loops. The suffixes “1” and “2” represents outter and inner loop properties respectively.
    void parallel_for(int low1, int high1, int stride1, int low2, int high2, int stride2,
    std::function<void(int, int)> &&lambda, int numThreads);
    // Shorthand for using parallel_for if for both outter and inner for-loops, lowbound is zero and stride is one.
    // In that case only highBounds for both these loop should be sufficient to provide. The suffixes “1” and “2”
    // represents outter and inner loop properties respectively.
    void parallel_for(int high1, int high2, std::function<void(int, int)> &&lambda, int numThreads);

}