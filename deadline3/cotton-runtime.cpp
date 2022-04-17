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
int COTTON_WORKER =2;
// Task pool to store all the tasks
TaskPool *TASK_POOL;
int *workerIds;

// int arr[100];

Queue::Queue(){
    head = 0;
    tail = 0;
    leftRange = -1;
    rightRange = -1;
    work = -1;
    pthread_mutex_init(&mutex, NULL);
}
Queue::~Queue(){
    pthread_mutex_destroy(&mutex);
}
// For Local Pop
Task Queue::popFromTail(){
    pthread_mutex_lock(&mutex);
    if(head==tail){
        pthread_mutex_unlock(&mutex);
        Task task;
        task.func = NULL;
        return task;
    }
    // To return the task
    Task toReturn=arr[(tail-1+QUEUE_SIZE)%QUEUE_SIZE];
    tail--;
    pthread_mutex_unlock(&mutex);
    
    return toReturn;
}
// For Remote Pop
Task Queue::popFromHead(){
    pthread_mutex_lock(&mutex);
    if(head==tail){
        pthread_mutex_unlock(&mutex);
        Task task;
        task.func = NULL;
        return task;
    }
    Task toReturn=arr[head%QUEUE_SIZE];
    head++;
    pthread_mutex_unlock(&mutex);
    
    return toReturn;
}
// Local Push
void Queue::push(Task func){
    pthread_mutex_lock(&mutex);
    if((tail-head+1)==QUEUE_SIZE){
        // Throw error if the Queue is full
        throw "Error: Task pool is Full";
    }
    arr[tail%QUEUE_SIZE]=func;
    tail++;
    pthread_mutex_unlock(&mutex);
}

TaskPool::TaskPool(int size){
    thread_pool_size = size;
}

void TaskPool::pushTask(std::function<void()> func, int work){
    
    int id = *(int *)pthread_getspecific(key);

    double p_work = task_pool[id].work;
    double p_start = task_pool[id].leftRange;
    double p_end = task_pool[id].rightRange;

    // printf("Hello %d %f %f %f\n", id, p_work, p_start, p_end);
    if(p_work == -1) {
        // task_pool[id].work = work;
        // task_pool[id].leftRange = 0;
        // task_pool[id].rightRange = COTTON_WORKER - 1;
        
        try {
            task_pool[0].push(Task{func, 0, (double)COTTON_WORKER - 1, work});
        }
        catch(const char* msg) {
            std::cerr<<msg<<std::endl;
            exit(1);
        }
    }else {
        if(p_work<0){
            p_work=1;
        }

        int worker_amount = (int)(p_end - p_start+1); 
        double middle = (int)(p_start + (double)(((double)work/(p_work+1))*(double)worker_amount))%COTTON_WORKER;
        task_pool[id].leftRange = (int)middle%COTTON_WORKER;

        task_pool[id].rightRange = (int)p_end%COTTON_WORKER;
        // task_pool[id].work -= work;
        // printf("Inside %d %f %f %f %d\n", id, task_pool[id].work, task_pool[id].leftRange, task_pool[id].rightRange,(int)p_start%COTTON_WORKER);
        try {
            // if(p_start==id)
            if(p_start<0)
              p_start=0;
            // if(p_start==id)
                task_pool[(int)p_start%COTTON_WORKER].push(Task{func,(double)((int)p_start%COTTON_WORKER), middle, work});
            // else
            //     migrate_pool[(int)p_start].push(Task{func,p_start, middle, work});
            // arr[(int)p_start]+=1;
        }
        
        catch(const char* msg) {
            std::cerr<<msg<<std::endl;
            exit(1);
        }
    }

}

std::function<void()> TaskPool::getTask(){
    int id = *(int *)pthread_getspecific(key);
    Task task = task_pool[id].popFromTail();
    
    if(task.func == NULL){
        // task=migrate_pool[id].popFromTail();
        // if(task.func == NULL){
            task = steal();
        // }
    }
    if(task.func != NULL) {
        task_pool[id].leftRange = task.leftRange;
        task_pool[id].rightRange = task.rightRange;
        task_pool[id].work = task.work;
        // arr[id]+=1;
    }
    return task.func;
}
// Steal operation
Task TaskPool::steal(){
    std::uniform_int_distribution<> dist{0, thread_pool_size}; // set min and max
    int id = dist(gen);
    Task task = task_pool[id].popFromHead();
    // if(task.func == NULL){
    //    task = migrate_pool[id].popFromHead();
    // }
    return task;
}

// To return the no. of COTTON_WORKER
int thread_pool_size(){
    return COTTON_WORKER;
}
// This will find if there is any task in the task pool and execute it
void find_and_execute_task() {
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
    int get_num_workers(){
        return COTTON_WORKER;
    }
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
        // for(int i = 0; i < COTTON_WORKER; i++) {
        //     printf("Worker %d: %d\n", i, arr[i]);
        // }
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
    
    void async(std::function<void()> &&lambda, int work) {
        pthread_mutex_lock(&lock_finish);
        finish_counter++;
        pthread_mutex_unlock(&lock_finish);

        TASK_POOL->pushTask(lambda, work);
        
    }
}
