#include<functional>
#include<pthread.h>
#include<random>

int thread_pool_size();
void find_and_execute_task();
void *worker_routine();
// #define QUEUE_SIZE 1000000;
enum {QUEUE_SIZE=100000};
// Queue data structure to hold our tasks.
class Queue{
    private:
        volatile int head;
        volatile int tail;
        // current size of the queue
        // Maximum size of the queue
        std::function<void()> arr[QUEUE_SIZE];
        // Lock for locking the PUSH operations
        pthread_mutex_t mutex;   
    public:
        Queue();
        ~Queue();
        std::function<void()> popFromHead();
        std::function<void()> popFromTail();
        void push(std::function<void()> func);
};


class TaskPool{
    private:
        Queue task_pool[100];
        int thread_pool_size;
        std::random_device seed;
        std::mt19937 gen{seed()}; // seed the generator
        
    public:
        TaskPool(int size);
        ~TaskPool();
        void pushTask(std::function<void()> func);
        std::function<void()> getTask();
        std::function<void()> steal();
};

