#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "thread_pool.h"
#include "../queue/queue.h"

#define SUCCESS 0
#define ERR_NONE -1
#define ERR_THREADPOOL_SHUTTING_DOWN -2
#define ERR_THREADPOOL_QUEUE_FULL -3
#define ERR_THREADPOOL_MALLOC_TASK_FAIL -4



// =========================================================================
// ==============================  TASK  ===================================
// =========================================================================

/**
 * @brief 任务结构体，包含任务处理函数和参数
 */
typedef struct ThreadTask{
    void *(*handle)(void *); // Function pointer to the task handler
    void *arg;  // Pointer to the argument for the task
} ThreadTask;



/// @brief 创建一个任务
/// @param handle 任务需要执行的方法
/// @param arg 执行方法需要携带的参数
/// @return 
static ThreadTask *queue_task_create(void *(*handle)(void *), void *arg) {
    ThreadTask *task = (ThreadTask *)malloc(sizeof(ThreadTask));
    if (task == NULL) {
        return NULL; // Memory allocation failed
    }

    task->handle = handle; // Initialize the function pointer to NULL
    task->arg = arg;    // Initialize the argument pointer to NULL
    return task;
}

/// @brief 执行一个任务
/// @param task 任务
/// @return 任务执行后的数据
static void* queue_task_run(ThreadTask *task) {
    if(task == NULL) {
        return NULL;
    }

    if (task && task->handle) {
        return task->handle(task->arg);
    }
    return NULL;
}



/// @brief 线程池结构体
typedef struct ThreadPool{
    pthread_t *threads;          // Pointer to the array of threads
    pthread_mutex_t mutex;   // Mutex for thread synchronization
    pthread_cond_t wakeup_cond; // Condition variable for waking up threads
    Queue *queue;                // Pointer to the task queue
    int thread_count;            // Number of threads in the pool
    int queue_capacity;          // Maximum capacity of the task queue
    int shutdown;
} ThreadPool;

/**
 * @brief 线程函数，执行任务队列中的任务
 * @param pool 线程池指针
 * @return 返回 NULL
 */
static void *thread_function(ThreadPool *pool) {
    ThreadTask *task;
    while (1) {
        pthread_mutex_lock(&pool->mutex);
        while (queue_is_full(pool->queue) && !pool->shutdown) {
            pthread_cond_wait(&pool->wakeup_cond, &pool->mutex);
        }

        // 已经关闭直接推出
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        queue_dequeue(pool->queue, (void *)&task);
        if(task != NULL) {
           queue_task_run(task);
           free(task); // Free the task after execution
        }

        pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}

/// @brief 创建线程池结构体
/// @param thread_count 
/// @param queue_capacity 
/// @return 
ThreadPool *threadpool_create(int thread_count, int queue_capacity) {
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        return NULL; // Memory allocation failed
    }

    pool->threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
    if (pool->threads == NULL) {
        free(pool);
        return NULL; // Memory allocation failed
    }

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->wakeup_cond, NULL);

    pool->queue = queue_create(queue_capacity);
    pool->thread_count = thread_count;
    pool->queue_capacity = queue_capacity;

    for(int i = 0; i < pool->thread_count; i++) {
        pthread_create(&pool->threads[i], NULL, (void *(*)(void *))thread_function, pool);
    }

    return pool;
}

/// @brief  销毁线程池
/// @param pool 
void threadpool_destroy(ThreadPool *pool) {
    if (pool == NULL) return;
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1; // Set shutdown flag
    pthread_cond_broadcast(&pool->wakeup_cond); // Wake up all threads
    pthread_mutex_unlock(&pool->mutex);

    // 等待所有线程结束
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // Clean up resources
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->wakeup_cond);
    free(pool->threads);
    queue_destroy(pool->queue);
    free(pool);
}

/// @brief 添加任务到线程池
/// @param pool 
/// @param task 
/// @param arg 
int threadpool_add_task(ThreadPool *pool, void *(*task)(void *), void *arg) {
     if (pool == NULL || task == NULL) return ERR_NONE;

    if(pool->shutdown) {
        return ERR_THREADPOOL_SHUTTING_DOWN;
    }

    if(queue_is_full(pool->queue)) {
        return ERR_THREADPOOL_QUEUE_FULL;
    }

    ThreadTask *queue_task = queue_task_create(task, arg);
    if (queue_task == NULL) {
        return ERR_THREADPOOL_MALLOC_TASK_FAIL;
    };

    pthread_mutex_lock(&pool->mutex);
    if(queue_enqueue(pool->queue, queue_task) != 0){
        free(queue_task);
        pthread_cond_signal(&pool->wakeup_cond);
    }
    pthread_mutex_unlock(&pool->mutex);
    return SUCCESS;
}

/**
 * @brief 获取线程池的错误信息
 * @param errno 错误码
 * @return 错误信息字符串
 */
char *threadpool_error(int errno) {
    switch(errno) {
        case ERR_NONE:
            return "No error";
        case ERR_THREADPOOL_SHUTTING_DOWN:
            return "Thread pool is shutting down";
        case ERR_THREADPOOL_QUEUE_FULL:
            return "Thread pool queue is full";
        case ERR_THREADPOOL_MALLOC_TASK_FAIL:
            return "Thread pool failed to allocate memory for ThreadTask";
        default:
            return "Unknown error";
    }
}