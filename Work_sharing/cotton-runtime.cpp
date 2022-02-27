#include <pthread.h>
#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include "cotton-runtime.h"

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
int COTTON_WORKER = 1;
// Task pool to store all the tasks
TaskPool *TASK_POOL;
int *workerIds;

Queue::Queue(){
    head = NULL;
    tail = NULL;
    int size = 0;
    CAPACITY = QUEUE_SIZE;
    pthread_mutex_init(&mutex, NULL);
    //std::cout<<"init"<<std::endl;
}
Queue::~Queue(){
    pthread_mutex_destroy(&mutex);
    while(head != NULL){
        Task* temp = head;
        head = head->next;
        delete temp;
    }
    tail=NULL;
}
std::function<void()> Queue::popFromTail(){
    // Acquiring the mutex_push lock as well in case there is only one element in the queue to prevent the race condition 
    // of simultaeously doing both push and pop operations when there is only one element in the queue.
    pthread_mutex_lock(&mutex);
    if(tail == NULL){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    Task* task = tail; // Previously it was task = head;
    tail = tail->prev;
    size--;
    if(tail == NULL){
        head = NULL;
    } else{
        tail->next = NULL;
    }
    // Check if mutex_push lock was acquired or not
    pthread_mutex_unlock(&mutex);
    std::function<void()> toReturn=task->func;
    delete task;
    return toReturn;
}
std::function<void()> Queue::popFromHead(){
    pthread_mutex_lock(&mutex);
    if(head == NULL){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    Task* task = head;
    head = head->next;
    size--;
    if(head==NULL){
        tail=NULL;
    } else{
        head->prev = NULL;
    }
    // Check if mutex_push lock was acquired or not
    pthread_mutex_unlock(&mutex);
    std::function<void()> toReturn=task->func;
    delete task;
    return toReturn;
}
void Queue::push(std::function<void()> func){
    // Creating a new Task to push in the task pool
    Task* task = new Task;
    task->func = func;
    task->next = NULL;
    task->prev = tail;
    
    if(size > CAPACITY){
        throw "Error: Task pool is Full";
    }
    if(head == NULL){
        head = tail = task;
    } else{
        tail->next = task;
        tail = task;
    }
    size++;
}

TaskPool::TaskPool(int size){
    // for(int i=0;i<size;i++){
    //     task_pool.push_back(Queue());
    // }
    thread_pool_size = size;
}

// TaskPool::~TaskPool(){
//     delete[] task_pool;
// }

void TaskPool::pushTask(std::function<void()> func){
    void* task=pthread_getspecific(key);
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
    void* task2=pthread_getspecific(key);
    int id = *(int *)pthread_getspecific(key);
    std::function<void()> task = task_pool[id].popFromTail();
    if(task == NULL){
        task = steal();
    }
    return task;
}

std::function<void()> TaskPool::steal(){
    std::uniform_int_distribution dist{0, thread_pool_size}; // set min and max
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

        workerIds = (int*)malloc(COTTON_WORKER * sizeof(int));
        for(int i = 0; i < COTTON_WORKER; i++) {
            workerIds[i] = i;
        }

        for (int i = 1; i < size; i++){
            pthread_create(&thread_pool[i - 1], NULL, &worker_routine, (void *)&workerIds[i]);
        }

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
        delete[] workerIds;
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


// int main(int argc, char const *argv[])
// {
//     cotton::init_runtime();
//     cotton::start_finish();
//     for (int i = 0; i < 100; i++){
//         cotton::async([](){
//             std::cout<<"Hello World"<<std::endl;
//         });
//     }
//     cotton::end_finish();
//     cotton::finalize_runtime();
//     return 0;
// }
