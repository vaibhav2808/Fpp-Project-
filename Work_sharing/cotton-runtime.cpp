#include <pthread.h>
#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include "cotton-runtime.h"
#include <sys/time.h>

// Lock for locking the finish_counter
pthread_mutex_t lock_finish;
pthread_key_t key;
// Flag to shutdown the program
volatile bool shutdown = false;
// Variable to store the no. of Async tasks spawned
volatile int finish_counter = 0;
// A pool of threads
pthread_t *thread_pool;
// No. of worker threads to be created
int COTTON_WORKER =1;
// Task pool to store all the tasks
TaskPool *TASK_POOL;
int *workerIds;

Queue::Queue(){
    head = 0;
    tail = 0;
    pthread_mutex_init(&mutex, NULL);
}
Queue::~Queue(){
    pthread_mutex_destroy(&mutex);
}
// For Local Pop
std::function<void()> Queue::popFromTail(){
    pthread_mutex_lock(&mutex);
    if(head==tail){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    // To return the task
    std::function<void()> toReturn=arr[(tail-1+QUEUE_SIZE)%QUEUE_SIZE];
    tail--;
    pthread_mutex_unlock(&mutex);
    
    return toReturn;
}
// For Remote Pop
std::function<void()> Queue::popFromHead(){
    pthread_mutex_lock(&mutex);
    if(head==tail){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    std::function<void()> toReturn=arr[head%QUEUE_SIZE];
    head++;
    pthread_mutex_unlock(&mutex);
    
    return toReturn;
}
// Local Push
void Queue::push(std::function<void()> func){
    if((tail-head+1)==QUEUE_SIZE){
        // Throw error if the Queue is full
        throw "Error: Task pool is Full";
    }
    arr[tail%QUEUE_SIZE]=func;
    tail++;
}

TaskPool::TaskPool(int size){
    thread_pool_size = size;
}

void TaskPool::pushTask(std::function<void()> func){
    int id = *(int *)pthread_getspecific(key);
    try {
        task_pool[id].push(func);
    }
    catch(const char* msg) {
        std::cerr<<msg<<std::endl;
        exit(1);
    }
}

std::function<void()> TaskPool::getTask(){
    int id = *(int *)pthread_getspecific(key);
    std::function<void()> task = task_pool[id].popFromTail();
    if(task == NULL){
        task = steal();
    }
    return task;
}
// Steal operation
std::function<void()> TaskPool::steal(){
    std::uniform_int_distribution dist{0, thread_pool_size-1}; // set min and max
    int id = dist(gen);
    std::function<void()> task = task_pool[id].popFromHead();
    return task;
}

// To return the no. of COTTON_WORKER
int thread_pool_size(){
    return COTTON_WORKER;
}
// This will find if there is any task in the task pool and execute it
void find_and_execute_task(){
    std::function<void()> task = TASK_POOL->getTask();
    if(task != NULL){
        task();
        pthread_mutex_lock(&lock_finish);
        finish_counter--;
        pthread_mutex_unlock(&lock_finish);
    }
}
// It is a thread function that is continously querying the task pool to check if there are any tasks present
void *worker_routine(void *arg){
    int id=*(int *)arg;
    pthread_setspecific(key, (void *)&id);
    while (!shutdown){
        find_and_execute_task();
    }
    return NULL;
}

namespace cotton{
    void init_runtime() {
        pthread_key_create(&key, NULL);
        const char *nworkers_str = getenv("COTTON_WORKERS");
        if (nworkers_str) {
            COTTON_WORKER = atoi(nworkers_str);
        }

        std::cout<<COTTON_WORKER<<" workers"<<std::endl;
        TASK_POOL = new TaskPool(COTTON_WORKER);
        int size = thread_pool_size();
        thread_pool = (pthread_t *)malloc(size * sizeof(pthread_t));

        if (pthread_mutex_init(&lock_finish, NULL) != 0){
            printf("\n mutex init has failed\n");
            return;
        }

        // Storing the Keys in an array and dynamically assigning memory to it
        workerIds = (int*)malloc(COTTON_WORKER * sizeof(int));
        for(int i = 0; i < COTTON_WORKER; i++) {
            workerIds[i] = i;
        }

        for (int i = 1; i < size; i++){
            pthread_create(&thread_pool[i - 1], NULL, &worker_routine, (void *)&workerIds[i]);
        }
        // Setting the key for master thread
        pthread_setspecific(key, (void *)&workerIds[0]);
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
        free(workerIds);
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

        TASK_POOL->pushTask(lambda);
        
    }
}

