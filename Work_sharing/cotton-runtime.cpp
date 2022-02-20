#include <pthread.h>
#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>

using namespace std;

pthread_mutex_t lock_finish;
pthread_mutex_t lock_push;
pthread_mutex_t lock_pop;

class Task
{
public:
    function<void()> func;
    Task *next;
};

class Queue
{
    Task *front;
    Task *rear;
    int size;
    int capacity;

public:
    void push(Task *task)
    {
        pthread_mutex_lock(&lock_push);
        if (size >= capacity)
        {
            return;
        }
        if (front == NULL && rear == NULL)
        {
            front = task;
            rear = task;
        }
        else
        {
            rear->next = task;
            rear = task;
        }
        size++;
        pthread_mutex_unlock(&lock_push);
    }

    Task *pop()
    {
        pthread_mutex_lock(&lock_pop);
        if (front == NULL)
        {
            return NULL;
        }
        Task *temp = front;
        front = front->next;
        temp->next = NULL;

        if (front == NULL)
        {
            rear = NULL;
        }
        size--;
        pthread_mutex_unlock(&lock_pop);
        return temp;
    }

    void set_capacity(int capacity)
    {
        this->capacity = capacity;
    }
    bool is_empty()
    {
        return front == NULL;
    }
    int get_size()
    {
        return size;
    }

    Queue()
    {
        size = 0;
        capacity = 9999;
        front = NULL;
        rear = NULL;
    }
};

void start_finish();
void end_finish();
void find_and_execute_task();
void *worker_routine(void *arg);
Task *grab_task_from_runtime();
int thread_pool_size();
void init_runtime();
void finalize_runtime();
void async();

volatile bool shutdown = false;
volatile int finish_counter = 0;

Queue task_pool;
pthread_t *thread_pool;
int const COTTON_WORKER = 2;

void execute_task(Task *task)
{
    task->func();
}

Task *grab_task_from_runtime()
{
    if (task_pool.is_empty())
    {
        return NULL;
    }
    return task_pool.pop();
}

void find_and_execute_task()
{
    Task *task = grab_task_from_runtime();
    if (task != NULL)
    {
        execute_task(task);
        free(task);
        pthread_mutex_lock(&lock_finish);
        finish_counter--;
        pthread_mutex_unlock(&lock_finish);
    }
}

void *worker_routine(void *arg)
{
    while (!shutdown)
    {
        find_and_execute_task();
    }
    return NULL;
}

int thread_pool_size()
{
    return COTTON_WORKER;
}

void init_runtime()
{
    int size = thread_pool_size();
    thread_pool = (pthread_t *)malloc(size * sizeof(pthread_t));

    if (pthread_mutex_init(&lock_finish, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return;
    }
    if (pthread_mutex_init(&lock_push, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return;
    }
    if (pthread_mutex_init(&lock_pop, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return;
    }

    for (int i = 1; i < size; i++)
    {
        pthread_create(&thread_pool[i - 1], NULL, &worker_routine, NULL);
    }
}

void finalize_runtime()
{
    shutdown = true;
    pthread_mutex_destroy(&lock_finish);
    pthread_mutex_destroy(&lock_push);
    pthread_mutex_destroy(&lock_pop);

    int size = thread_pool_size();
    // master waits for helpers to join
    for (int i = 1; i < size; i++)
    {
        pthread_join(thread_pool[i - 1], NULL);
    }
}

void start_finish()
{
    finish_counter = 0; // reset
}
void end_finish()
{
    while (finish_counter != 0)
    {
        find_and_execute_task();
    }
}

void push_task_to_runtime(Task *task)
{
    task_pool.push(task);
}
void async(function<void()> &&lambda)
{
    pthread_mutex_lock(&lock_finish);
    finish_counter++;
    pthread_mutex_unlock(&lock_finish);

    // copy task on heap
    Task task;
    task.func = lambda;
    int task_size = sizeof(task);
    Task *p = (Task *)malloc(task_size);
    memcpy(p, &lambda, task_size);

    // thread-safe push_task_to_runtime
    push_task_to_runtime(p);

    return;
}

int main(int argc, char const *argv[])
{
    init_runtime();
    start_finish();
    async([]()
          {
        for(int i = 0; i < 100000; i++) {
            for(int j = 0; j < 100000; j++) {

            }
        }
        cout<<"Done1"<<endl; });
    async([]()
          {
        for(int i = 0; i < 100000; i++) {
            for(int j = 0; j < 100000; j++) {

            }
        }
        cout<<"Done2"<<endl; });

    // Total: 30, each around 15
    // []()
    // {
    //     for(int i = 0; i < 100000; i++) {
    //         for(int j = 0; j < 100000; j++) {

    //         }
    //     }
    //     cout<<"Done1"<<endl;
    // }();
    // []()
    // {
    //     for(int i = 0; i < 100000; i++) {
    //         for(int j = 0; j < 100000; j++) {

    //         }
    //     }
    //     cout<<"Done2"<<endl;
    // }();
    end_finish();
    finalize_runtime();

    return 0;
}
