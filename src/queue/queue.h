#ifndef QUEUE_H_
#define QUEUE_H_

// Description: Header file for queue

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 队列结构体，包含队列数据、前后指针、大小和容量
 */
typedef struct Queue Queue;

/**
 * @brief 创建一个队列
 * @param capacity 队列的最大容量
 * @return 返回一个指向新创建队列的指针，创建失败时返回
 */
Queue *queue_create(int capacity);

/**
 * @brief 销毁队列
 * @details 释放队列的内存，包括队列数据和队列本身
 * @param queue 需要销毁的队列
 */
void queue_destroy(Queue *queue);

/**
 * @brief 将一个任务入队，加入队列最后一个
 * @param queue 队列
 * @param task 任务
 * @return 是否成功入列 1：是 0：否
 */
int queue_enqueue(Queue *queue, void *task);

/**
 * @brief 将一个任务出队，取出丢列第一个任务
 * @param queue 需要被出队的队列
 * @param task 成功出队的任务
 * @return 是否成功出列 1：是 0：否
 */
int queue_dequeue(Queue *queue, void **task);

/**
 * @brief 获取队列的头部元素
 * @param queue 需要获取头部元素的队列
 * @param task 存储头部元素的指针
 * @return 如果队列不为空，返回1并将头部元素存储在task中；如果队列为空，返回0并将task设置为NULL
 */
int queue_peek(Queue *queue, void **task);

/**
 * @brief 检查队列是否已满
 * @param queue 需要检查的队列
 * @return 如果队列已满，返回1；否则返回0
 */
int queue_is_full(const Queue *queue);

/**
 * @brief 获取队列的当前大小
 * @param queue 需要获取大小的队列
 * @return 队列的当前大小
 */
int queue_size(const Queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H_ */ 