#include <pthread.h>
#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include "cotton-runtime.h"

// Lock for locking the finish_counter
pthread_mutex_t lock_finish;
// Flag to shutdown the program
volatile bool shutdown = false;
// Variable to store the no. of Async tasks spawned
volatile int finish_counter = 0;
// A pool of threads
pthread_t *thread_pool;
// No. of worker threads to be created
int COTTON_WORKER = 1;
// Task pool to store all the tasks
Queue task_pool;

Queue::Queue(){
    head = NULL;
    tail = NULL;
    int size = 0;
    CAPACITY = QUEUE_SIZE;
    pthread_mutex_init(&mutex_pop, NULL);
    pthread_mutex_init(&mutex_push, NULL);
}

Queue::~Queue(){
    pthread_mutex_destroy(&mutex_pop);
    pthread_mutex_destroy(&mutex_push);
    while(head != NULL){
        Task* temp = head;
        head = head->next;
        delete temp;
    }
    tail=NULL;
}

std::function<void()> Queue::pop(){
    // Acquiring the mutex_push lock as well in case there is only one element in the queue to prevent the race condition 
    // of simultaeously doing both push and pop operations when there is only one element in the queue.
    bool isPushLockAcquired = false;
    if(size == 1) {
        pthread_mutex_lock(&mutex_push);
        isPushLockAcquired = true;
    }
    pthread_mutex_lock(&mutex_pop);

    if(head == NULL){
        if(isPushLockAcquired){
            pthread_mutex_unlock(&mutex_push);
        }
        pthread_mutex_unlock(&mutex_pop);
        return NULL;
    }

    Task* task = head;
    head = head->next;
    size--;
    // Check if mutex_push lock was acquired or not
    if(isPushLockAcquired){
        pthread_mutex_unlock(&mutex_push);
    }
    pthread_mutex_unlock(&mutex_pop);
    std::function<void()> toReturn=task->func;
    delete task;
    return toReturn;
}

void Queue::push(std::function<void()> func){
    // Creating a new Task to push in the task pool
    Task* task = new Task;
    task->func = func;
    task->next = NULL;
    
    pthread_mutex_lock(&mutex_push);
    if(size > CAPACITY){
        throw "Error: Task pool is Full";
    }
    // Acquiring the mutex_pop lock as well in case there is only one element in the queue to prevent the race condition 
    // of simultaeously doing both push and pop operations when there is only one element in the queue.
    bool isPopLockAcquired = false;
    if(size == 1) {
        pthread_mutex_lock(&mutex_pop);
        isPopLockAcquired = true;
    }
    if(head == NULL){
        head = tail = task;
    } else{
        tail->next = task;
        tail = task;
    }
    size++;
    // Check if mutex_pop lock was acquired or not
    if(isPopLockAcquired){
        pthread_mutex_unlock(&mutex_pop);
    }
    pthread_mutex_unlock(&mutex_push);
}

// To return the no. of COTTON_WORKER
int thread_pool_size(){
    return COTTON_WORKER;
}
// This will find if there is any task in the task pool and execute it
void find_and_execute_task(){
    std::function<void()> task = task_pool.pop();
    if(task != NULL){
        task();
        pthread_mutex_lock(&lock_finish);
        finish_counter--;
        pthread_mutex_unlock(&lock_finish);
    }
}
// It is a thread function that is continously querying the task pool to check if there are any tasks present
void *worker_routine(void *arg){
    while (!shutdown){
        find_and_execute_task();
    }
    return NULL;
}

namespace cotton{
    void init_runtime() {
        const char *nworkers_str = getenv("COTTON_WORKERS");
        if (nworkers_str) {
            COTTON_WORKER = atoi(nworkers_str);
        }
        std::cout<<COTTON_WORKER<<" workers"<<std::endl;
        int size = thread_pool_size();
        thread_pool = (pthread_t *)malloc(size * sizeof(pthread_t));

        if (pthread_mutex_init(&lock_finish, NULL) != 0){
            printf("\n mutex init has failed\n");
            return;
        }

        for (int i = 1; i < size; i++){
            pthread_create(&thread_pool[i - 1], NULL, &worker_routine, NULL);
        }
    }

    void finalize_runtime() {
        shutdown = true;
        pthread_mutex_destroy(&lock_finish);
        int size = thread_pool_size();
        // master waits for helpers to join
        for (int i = 1; i < size; i++){
            pthread_join(thread_pool[i - 1], NULL);
        }
        free(thread_pool);
    }

    void start_finish() {
        finish_counter = 0; // reset
    }

    void end_finish() {
        while (finish_counter != 0){
            find_and_execute_task();
        }
    }
    
    void async(std::function<void()> &&lambda) {
        pthread_mutex_lock(&lock_finish);
        finish_counter++;
        pthread_mutex_unlock(&lock_finish);
        try {
            task_pool.push(lambda);
        }
        catch(const char* msg) {
            std::cerr<<msg<<std::endl;
            exit(1);
        }
    }
}