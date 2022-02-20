#include<functional>

int thread_pool_size();
void find_and_execute_task();
void *worker_routine();
#define QUEUE_SIZE 1;

struct Task{
    struct Task* next;
    std::function<void()> func;
};

class Queue{
    private:
        Task* head;
        Task* tail;
        int size;
        int CAPACITY;
        pthread_mutex_t mutex;
    
    public:
        Queue();
        ~Queue();
        std::function<void()> pop();
        void push(std::function<void()> func);
};