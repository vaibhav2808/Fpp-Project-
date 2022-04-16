#include<functional>
#include<pthread.h>
#include<random>

int thread_pool_size();
void find_and_execute_task();
void *worker_routine();
// #define QUEUE_SIZE 1000000;
enum {QUEUE_SIZE=100000};

struct Task {
    std::function<void()> func;
    double leftRange, rightRange;
    int work;
};

// Queue data structure to hold our tasks.
class Queue {
    private:
        volatile int head;
        volatile int tail;
        Task arr[QUEUE_SIZE];
        // Lock for locking the PUSH operations
        pthread_mutex_t mutex;   
    public:
        double work;
        double leftRange;
        double rightRange;
        Queue();
        ~Queue();
        Task popFromHead();
        Task popFromTail();
        void push(Task func);
};

class TaskGroup {
    private:
        double work;
        double leftRange;
        double rightRange;
    
    public:
        TaskGroup();
        ~TaskGroup();
        void run(std::function<void()> func, int work);
        void wait();
};

class TaskPool{
    private:
        Queue task_pool[100];
        Queue migrate_pool[100];
        int thread_pool_size;
        std::random_device seed;
        std::mt19937 gen{seed()}; // seed the generator
        
    public:
        TaskPool(int size);
        ~TaskPool();
        void pushTask(std::function<void()> func, int work);
        std::function<void()> getTask();
        Task steal();
};