#ifndef THREADPOOL_H_
#define THREADPOOL_H_


#ifdef __cplusplus
extern "C" {
#endif

/// @brief 线程池结构体
typedef struct ThreadPool ThreadPool;

/// @brief 创建线程池结构体
/// @param thread_count 
/// @param queue_capacity 
/// @return 
ThreadPool *threadpool_create(int thread_count, int queue_capacity);

/// @brief 销毁线程池
/// @param pool 
void threadpool_destroy(ThreadPool *pool);

/// @brief 添加任务到线程池
/// @param pool 
/// @param task 
/// @param arg 
int threadpool_add_task(ThreadPool *pool, void *(*task)(void *), void *arg);

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