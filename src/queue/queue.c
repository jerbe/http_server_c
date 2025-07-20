#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"
// =========================================================================
// ==============================  QUEUE  ==================================
// =========================================================================

/**
 * @brief 队列结构体，包含队列数据、前后指针、大小和容量
 */
typedef struct Queue {
    void **data;        // Pointer to the queue data
    int front;       // Index of the front element
    int rear;        // Index of the rear element
    int size;        // Current size of the queue
    int capacity;    // Maximum capacity of the queue
    pthread_mutex_t mutex; // Mutex for thread safety
    int destroyed; // Flag to indicate if the queue is destroyed
} Queue;

/// @brief 创建一个队列
/// @param capacity 
/// @return 
Queue *queue_create(int capacity) {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (queue == NULL) {
        return NULL; // Memory allocation failed
    }

    queue->data = (void *)malloc(capacity * sizeof(void *));
    if (queue->data == NULL) {
        return NULL; // Memory allocation failed
    }

    pthread_mutex_init(&queue->mutex, NULL); // Initialize the mutex

    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
    queue->capacity = capacity;
    queue->destroyed = 0; // Initialize the destroyed flag to 0
    

    return queue; // Return the pointer to the newly created queue
}

/// @brief 销毁一个队列
/// @param queue 
void queue_destroy(Queue *queue) {
    if (queue == NULL || queue->destroyed) {
        return; // If the queue is NULL or already destroyed, do nothing
    }

    queue->destroyed = 1; // Set the destroyed flag to 1

    if (queue->data != NULL) {
        free(queue->data);
        queue->data = NULL;
    }
    pthread_mutex_destroy(&queue->mutex); // Destroy the mutex

    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
    queue->capacity = 0;
    free(queue);
}

/// @brief 将一个任务入队，加入队列最后一个
/// @param queue 队列
/// @param task 任务
/// @return 是否成功入列 1：是 0：否
int queue_enqueue(Queue *queue, void *task) {
    if (queue_is_full(queue)) {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex); // Lock the mutex for thread safety
    if (queue->destroyed) {
        pthread_mutex_unlock(&queue->mutex); // Unlock the mutex before returning
        return -1; // If the queue is destroyed, do not enqueue
    }
    queue->data[queue->rear] = task;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;
    pthread_mutex_unlock(&queue->mutex); // Unlock the mutex
    return 0;
}

/// @brief 将一个任务出队，取出丢列第一个任务
/// @param queue 需要被出队的队列
/// @param task 成功
/// @return 是否成功出列 1：是 0：否
int queue_dequeue(Queue *queue, void **task) {
    if (queue_size(queue) == 0) {
        *task = NULL;
        return -1;
    }

    pthread_mutex_lock(&queue->mutex); // Lock the mutex for thread safety
    if (queue->destroyed) {
        pthread_mutex_unlock(&queue->mutex); // Unlock the mutex before returning
        free(task);
        *task = NULL; // If the queue is destroyed, set task to NULL
        return -1; // If the queue is destroyed, do not dequeue
    }
    *task = queue->data[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    pthread_mutex_unlock(&queue->mutex); // Unlock the mutex
    return 0;
}

/**
 * @brief 获取队列的头部元素
 * @param queue 需要获取头部元素的队列
 * @param task 存储头部元素的指针
 * @return 如果队列不为空，返回1并将头部元素存储在task中；如果队列为空，返回0并将task设置为NULL
 */
int queue_peek(Queue *queue, void **task) {
    if (queue == NULL || queue->destroyed) {
        *task = NULL; // If the queue is NULL or destroyed, set task to NULL
        return -1; 
    }

    if (queue->size == 0) {
        *task = NULL;
        return -1; 
    }
    pthread_mutex_lock(&queue->mutex); // Lock the mutex for thread safety
    if (queue->destroyed) {
        pthread_mutex_unlock(&queue->mutex); // Unlock the mutex before returning
        free(task);
        *task = NULL; // If the queue is destroyed, set task to NULL
        return -1; // If the queue is destroyed, do not peek
    }
    *task = queue->data[queue->front]; // Get the front element
    pthread_mutex_unlock(&queue->mutex); // Unlock the mutex
    return 0; // Successfully retrieved the front element   
}       

/// @brief 判断队列是否已经满了
/// @param queue 需要判断的队列
/// @return >=1:满 0:未满
int queue_is_full(const Queue *queue) {
    int r = queue->size == queue->capacity;
    return r; 
}


/**
 * @brief 获取队列的当前大小
 * @param queue 需要获取大小的队列
 * @return 队列的当前大小
 */
int queue_size(const Queue *queue){
    if (queue == NULL || queue->destroyed) {
        return 0; // If the queue is NULL or destroyed, return 0
    }
    return queue->size; // Return the current size of the queue
}

