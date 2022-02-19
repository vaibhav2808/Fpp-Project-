#include"./stamp.h"
#include<pthread.h>
#include<functional>
#include<iostream>
#include <chrono>
using namespace std::chrono;

    //structure to hold arguements for 1 dimensional for loop function
    struct arguements{
        std::function<void(int)> func;
        int threadNum,high,low,stride,numThreads;
    };

    //structure to hold arguement for execute tuple function
    struct tupleArgs{
        std::function<void()> func;
    };
    
    //strcuture to hold arguements for 2 dimensional for loop function
    struct arguements2{
        std::function<void(int,int)> func;
        int threadNum,high,low,stride,numThreads,high2,low2,stride2;
    };

    int calcIterationsForThread(int low, int high,int stride,int numThreads){
        int num=(int)((high-low)/stride);
        if((high-low)%stride!=0)
            num+=1;
        int quotient=(int)(num/numThreads);
        if(num%numThreads!=0)
            quotient+=1;
        return quotient;
    }

    //function executed by thread for execute tuple
    void* executeFunction(void* arg){
        struct tupleArgs* args= (struct tupleArgs*)arg;
        args->func();
        pthread_exit(NULL);
    }

    //function executed by each thread for 1 dimensional for loop
    void* executeFunctionWithArguement(void* arg){
        struct arguements *args=(struct arguements*) arg;

        //calculate the number of iteration that needs to be assigned to each thread
        int num=calcIterationsForThread(args->low,args->high,args->stride,args->numThreads);

        //calculate start of loop based on the thread number
        int base=args->low+num*args->threadNum*args->stride;

        //execute the loop and call the lambda function
        for(int i=0;i<num;i++){
            int ind=base+ i*args->stride;
            if(ind < args->high){
                args->func(ind);
            }
        }
        pthread_exit(NULL);
    }

    //function executed by each thread for 2 dimensional for loop
    void* executeFunctionWith2Arguements(void* arg){
        struct arguements2 *args=(struct arguements2*) arg;

        //calculate the number of iteration that needs to be assigned to each thread. In this case fo outer loop
        int num=calcIterationsForThread(args->low,args->high,args->stride,args->numThreads);

        //calculate start of loop based on the thread number
        int base=args->low+num*args->threadNum*args->stride;

        //execute the loop and call the lambda function
        for(int i=0;i<num;i++){
            int ind=base+ i*args->stride;
            if(!(ind < args->high)){
                break;
            }
            for(int j=args->low2;j<args->high2;j+=args->stride2)
                args->func(ind,j);
        }
        pthread_exit(NULL);
    }

namespace stamp{
    void execute_tuple(std::function<void()> &&lambda1, std::function<void()> &&lambda2){
        auto start = high_resolution_clock::now();
        pthread_t thread1,thread2;

        //initialise structures to hold the arguements
        struct tupleArgs arg1,arg2;
        arg1.func=lambda1;
        arg2.func=lambda2;

        //create two threads
        pthread_create(&thread1,NULL,&executeFunction,&arg1);
        pthread_create(&thread2,NULL,&executeFunction,&arg2);

        //wait for them to complete the execution
        pthread_join(thread1,NULL);
        pthread_join(thread2,NULL);
        
        //calculate execution time
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        std::cout<<"StaMp Statistics: Threads = "<<2<<" Parallel execution time = "<<((double)duration.count()/1000)<<" seconds ";
    }

    void parallel_for(int low, int high, int stride, std::function<void(int)> &&lambda, int numThreads){
        auto start = high_resolution_clock::now();
        pthread_t threads[numThreads];
        struct arguements argsArray[numThreads];

        for(int threadNum=0;threadNum<numThreads;threadNum++){

            //add elements to structure to pass to pthread function
            argsArray[threadNum].threadNum=threadNum;
            argsArray[threadNum].high=high;
            argsArray[threadNum].low=low;
            argsArray[threadNum].stride=stride;

            argsArray[threadNum].numThreads=numThreads;
            argsArray[threadNum].func=lambda;

            //create the thread and pass the structure as the arguement
            pthread_create(&threads[threadNum],NULL,&executeFunctionWithArguement,(void*)&argsArray[threadNum]);

        }

        //wait for them to complete the execution
        for(int threadNum=0;threadNum<numThreads;threadNum++){
            pthread_join(threads[threadNum],NULL);
        }

        //calculate execution time
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        std::cout<<"StaMp Statistics: Threads = "<<numThreads<<" Parallel execution time = "<<((double)duration.count()/1000)<<" seconds ";
        return;
    }

    void parallel_for(int high, std::function<void(int)> &&lambda, int numThreads){
        parallel_for(0,high,1,[&lambda](int i){lambda(i);},numThreads);
    }

    void parallel_for(int low1, int high1, int stride1, int low2, int high2, int stride2,
    std::function<void(int, int)> &&lambda, int numThreads){
        auto start = high_resolution_clock::now();
        pthread_t threads[numThreads];
        struct arguements2 argsArray[numThreads];

        for(int threadNum=0;threadNum<numThreads;threadNum++){
            
            //add elements to structure to pass to pthread function
            argsArray[threadNum].threadNum=threadNum;
            argsArray[threadNum].high=high1;
            argsArray[threadNum].low=low1;
            argsArray[threadNum].stride=stride1;

            argsArray[threadNum].high2=high2;
            argsArray[threadNum].low2=low2;
            argsArray[threadNum].stride2=stride2;

            argsArray[threadNum].numThreads=numThreads;
            argsArray[threadNum].func=lambda;

            //create the thread and pass the structure as the arguement
            pthread_create(&threads[threadNum],NULL,&executeFunctionWith2Arguements,(void*)&argsArray[threadNum]);
        }

        //wait for them to complete the execution
        for(int threadNum=0;threadNum<numThreads;threadNum++){
            pthread_join(threads[threadNum],NULL);
        }

        //calculate execution time
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        std::cout<<"StaMp Statistics: Threads = "<<numThreads<<" Parallel execution time = "<<((double)duration.count()/1000)<<" seconds ";
        return;
    }

    void parallel_for(int high1, int high2, std::function<void(int, int)> &&lambda, int numThreads){
        parallel_for(0,high1,1,0,high2,1,[&lambda](int i,int j){lambda(i,j);},numThreads);
    }
}