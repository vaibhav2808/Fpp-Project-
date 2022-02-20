#include <pthread.h>
#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include "cotton-runtime.h"

pthread_mutex_t lock_finish;
volatile bool shutdown = false;
volatile int finish_counter = 0;
pthread_t *thread_pool;
int COTTON_WORKER = 1;
Queue task_pool;

Queue::Queue(){
    head = NULL;
    tail = NULL;
    pthread_mutex_init(&mutex, NULL);
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

std::function<void()> Queue::pop(){
    pthread_mutex_lock(&mutex);
    if(head == NULL){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    Task* task = head;
    head = head->next;
    pthread_mutex_unlock(&mutex);
    std::function<void()> toReturn=task->func;
    delete task;
    return toReturn;
}

void Queue::push(std::function<void()> func){
    pthread_mutex_lock(&mutex);
    Task* task = new Task;
    task->func = func;
    task->next = NULL;
    if(head == NULL){
        head = tail = task;
    } else{
        tail->next = task;
        tail = task;
    }
    pthread_mutex_unlock(&mutex);
}

int thread_pool_size(){
    return COTTON_WORKER;
}

void find_and_execute_task(){
    std::function<void()> task = task_pool.pop();
    if(task != NULL){
        task();
        pthread_mutex_lock(&lock_finish);
        finish_counter--;
        pthread_mutex_unlock(&lock_finish);
    }
}

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
        task_pool.push(lambda);
    }
}