#ifndef THREADPOOL_H_
#define THREADPOOL_H_


#ifdef __cplusplus
extern "C" {
#endif

/// @brief 线程池结构体
typedef struct ThreadPool ThreadPool;

/**
 * @brief 线程池完成执行方法
 */
typedef void(*ThreadPoolTaskAfterRunHandle)(void *);

/// @brief 创建线程池结构体
/// @param thread_count 
/// @param queue_capacity 
/// @return 
ThreadPool *threadpool_new(int thread_count, int queue_capacity, ThreadPoolTaskAfterRunHandle free_handle);

/// @brief 销毁线程池
/// @param pool 
void threadpool_destroy(ThreadPool *pool);

/// @brief 添加任务到线程池
/// @param pool 
/// @param task_handle 任务执行方法
/// @param arg 
int threadpool_add_task(ThreadPool *pool, void *(*task_handle)(void *), void *arg);

/**
 * @brief 获取线程池的错误信息
 * @param errno 错误码
 * @return 错误信息字符串
 */
char *threadpool_error(int err_no);


#ifdef __cplusplus
}
#endif

#endif /* THREADPOOL_H_ */