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
            if(p_start==id)
                task_pool[(int)p_start%COTTON_WORKER].push(Task{func,(double)((int)p_start%COTTON_WORKER), middle, work});
            else
                migrate_pool[(int)p_start].push(Task{func,p_start, middle, work});
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
        task=migrate_pool[id].popFromTail();
        if(task.func == NULL){
            task = steal();
        }
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
    std::uniform_int_distribution dist{0, thread_pool_size}; // set min and max
    int id = dist(gen);
    Task task = task_pool[id].popFromHead();
    if(task.func == NULL){
        task = migrate_pool[id].popFromHead();
    }
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
#include "common.h"
#include "cotton.h"

/* Define ERROR_SUMMARY if you want to check your numerical results */
//#define ERROR_SUMMARY

#define ELEMENT_t double
#define DATA_PRINTF_MODIFIER " %f\n "
int leafmaxcol = 8;

#define f(x,y)     (sin(x)*sin(y))
#define randa(x,t) (0.0)
#define randb(x,t) (exp(-2*(t))*sin(x))
#define randc(y,t) (0.0)
#define randd(y,t) (exp(-2*(t))*sin(y))
#define solu(x,y,t) (exp(-2*(t))*sin(x)*sin(y))

int nx, ny, nt;
ELEMENT_t xu, xo, yu, yo, tu, to;
ELEMENT_t dx, dy, dt;

ELEMENT_t dtdxsq, dtdysq;
ELEMENT_t t;

/*****************   Initialization of grid partition  NEW VERSION ********************/

void initgrid(ELEMENT_t *old, int lb, int ub) {

  int a, b, llb, lub;

  llb = (lb == 0) ? 1 : lb;
  lub = (ub == nx) ? nx - 1 : ub;

  for (a=llb, b=0; a < lub; a++){		/* boundary nodes */
    old[a * ny + b] = randa(xu + a * dx, 0);
  }

  for (a=llb, b=ny-1; a < lub; a++){
    old[a * ny + b] = randb(xu + a * dx, 0);
  }

  if (lb == 0) {
    for (a=0, b=0; b < ny; b++){
      old[a * ny + b] = randc(yu + b * dy, 0);
    }
  }
  if (ub == nx) {
    for (a=nx-1, b=0; b < ny; b++){
      old[a * ny + b] = randd(yu + b * dy, 0);
    }
  }
  for (a=llb; a < lub; a++) { /* inner nodes */
    for (b=1; b < ny-1; b++) {
      old[a * ny + b] = f(xu + a * dx, yu + b * dy);
    }
  }
}

void initialize(ELEMENT_t *old, int size){
    cotton::start_finish();
        irregular_recursion(0, size, leafmaxcol, [=] (int i) {
	    int lb=i, ub=lb+1;
            initgrid(old, lb, ub);
        });
    cotton::end_finish();
}

/***************** Five-Point-Stencil Computation NEW VERSION ********************/

void compstripe(ELEMENT_t *neww, ELEMENT_t *old, int lb, int ub) {
  int a, b, llb, lub;

  llb = (lb == 0) ? 1 : lb;
  lub = (ub == nx) ? nx - 1 : ub;

  for (a=llb; a < lub; a++) {
    for (b=1; b < ny-1; b++) {
        neww[a * ny + b] =   dtdxsq * (old[(a+1) * ny + b] - 2 * old[a * ny +b] + old[(a-1) * ny + b])
        + dtdysq * (old[a * ny + (b+1)] - 2 * old[a * ny + b] + old[a * ny + (b-1)])
        + old[a * ny + b];
    }
  }

  for (a=llb, b=ny-1; a < lub; a++)
    neww[a * ny + b] = randb(xu + a * dx, t);

  for (a=llb, b=0; a < lub; a++)
    neww[a * ny + b] = randa(xu + a * dx, t);

  if (lb == 0) {
    for (a=0, b=0; b < ny; b++)
      neww[a * ny + b] = randc(yu + b * dy, t);
  }
  if (ub == nx) {
    for (a=nx-1, b=0; b < ny; b++)
      neww[a * ny + b] = randd(yu + b * dy, t);
  }
}

/***************** Decomposition of 2D grids in stripes ********************/

void compute(int size, ELEMENT_t *neww, ELEMENT_t *old, int timestep){
    cotton::start_finish();
        irregular_recursion(0, size, leafmaxcol, [=] (int i) {
	    int lb=i, ub=lb+1;
            if (timestep % 2) //this sqitches back and forth between the two arrays by exchanging neww and old
                compstripe(neww, old, lb, ub);
            else
                compstripe(old, neww, lb, ub);
        });
    cotton::end_finish();
}

int heat(ELEMENT_t *old_v, ELEMENT_t *new_v, int run) {
  int  c;

#ifdef ERROR_SUMMARY
  ELEMENT_t tmp, *mat;
  ELEMENT_t mae = 0.0;
  ELEMENT_t mre = 0.0;
  ELEMENT_t me = 0.0;
  int a, b;
#endif

  /* Initialization */
  initialize(old_v, nx);

  /* Jacobi Iteration (divide x-dimension of 2D grid into stripes) */
  /* Timing. "Start" timers */
  printf("Heat started...\n");
 
  START_TIMER 
  for (c = 1; c <= nt; c++) {
    t = tu + c * dt;
    compute(nx, new_v, old_v, c);
  }
  END_TIMER

  printf("Heat ended...\n");	

#ifdef ERROR_SUMMARY
  /* Error summary computation: Not parallelized! */
  mat = (c % 2) ? old_v : new_v;
  printf("\n Error summary of last time frame comparing with exact solution:");
  for (a=0; a<nx; a++)
    for (b=0; b<ny; b++) {
      tmp = fabs(mat[a * nx + b] - solu(xu + a * dx, yu + b * dy, to));
      //printf("error: %10e\n", tmp);
      if (tmp > mae)
        mae = tmp;
    }

  printf("\n   Local maximal absolute error  %10e ", mae);

  for (a=0; a<nx; a++)
    for (b=0; b<ny; b++) {
      tmp = fabs(mat[a * nx + b] - solu(xu + a * dx, yu + b * dy, to));
      if (mat[a * nx + b] != 0.0)
        tmp = tmp / mat[a * nx + b];
      if (tmp > mre)
        mre = tmp;
    }

  printf("\n   Local maximal relative error  %10e %s ", mre * 100, "%");

  me = 0.0;
  for (a=0; a<nx; a++)
    for (b=0; b<ny; b++) {
      me += fabs(mat[a * nx + b] - solu(xu + a * dx, yu + b * dy, to));
    }

  me = me / (nx * ny);
  printf("\n   Global Mean absolute error    %10e\n\n", me);
#endif
  return 0;
}

int main(int argc, char *argv[]) {
  PRINT_HARNESS_HEADER
  cotton::init_runtime();
  int ret, help;

  nx = 16384;
  ny = 16384;
  nt = 50;
  xu = 0.0;
  xo = 1.570796326794896558;
  yu = 0.0;
  yo = 1.570796326794896558;
  tu = 0.0;
  to = 0.0000001;

  // use the math related function before parallel region;
  // there is some benigh race in initalization code for the math functions.
  fprintf(stderr, "Testing exp: %f\n", randb(nx, nt));

  printf("Input nx=%d, ny=%d, nt=%d, leafmaxcol=%d\n",nx,ny,nt,leafmaxcol);

  dx = (xo - xu) / (nx - 1);
  dy = (yo - yu) / (ny - 1);
  dt = (to - tu) / nt;	/* nt effective time steps! */

  dtdxsq = dt / (dx * dx);
  dtdysq = dt / (dy * dy);

   /* Memory Allocation */
  ELEMENT_t * old_v = new ELEMENT_t[nx * ny];
  ELEMENT_t * new_v = new ELEMENT_t[nx * ny];

  ret = heat(old_v, new_v, 0);

  printf("\nHeat");
  printf("\n dx = ");printf(DATA_PRINTF_MODIFIER, dx);
  printf(" dy = ");printf(DATA_PRINTF_MODIFIER, dy);
  printf(" dt = ");printf(DATA_PRINTF_MODIFIER, dt);
  printf(" Stability Value for explicit method must be > 0:"); 
  printf(DATA_PRINTF_MODIFIER,(0.5 - (dt / (dx * dx) + dt / (dy * dy))));
  printf("Options: granularity = %d\n", leafmaxcol);
  printf("         nx          = %d\n", nx);
  printf("         ny          = %d\n", ny);
  printf("         nt          = %d\n", nt);
  delete[] old_v;
  delete[] new_v;
  cotton::finalize_runtime();
  PRINT_HARNESS_FOOTER
  return 0;
}
