#include<functional>
#include<pthread.h>
#include<random>

int thread_pool_size();
void find_and_execute_task();
void *worker_routine();
#define QUEUE_SIZE 1000000;

// Task structure to hold a task.
struct Task{
    struct Task* next;
    struct Task* prev;
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
        Queue *task_pool;
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

TaskPool::TaskPool(int size){
    task_pool = new Queue[size];
    thread_pool_size = size;
}

TaskPool::~TaskPool(){
    delete[] task_pool;
}

void TaskPool::pushTask(std::function<void()> func){
    int id = *(int *)pthread_getspecific(key);
    task_pool[id].push(func);
}

std::function<void()> TaskPool::getTask(){
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