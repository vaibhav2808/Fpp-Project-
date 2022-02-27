#include<functional>

int thread_pool_size();
void find_and_execute_task();
void *worker_routine();
#define QUEUE_SIZE 1000000;

// Task structure to hold a task.
struct Task{
    struct Task* next;
    std::function<void()> func;
};
// Queue data structure to hold our tasks.
class Queue{
    private:
        Task* head;
        Task* tail;
        // current size of the queue
        int size;
        // Maximum size of the queue
        int CAPACITY;
        // Lock for locking the PUSH operations
        pthread_mutex_t mutex_push;
        // Lock for locking the POP operations
        pthread_mutex_t mutex_pop;    
    public:
        Queue();
        ~Queue();
        std::function<void()> pop();
        void push(std::function<void()> func);
};