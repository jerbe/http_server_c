#ifndef ARRAY_H_
#define ARRAY_H_

// Description: Header file for array

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 自定义数组
 */
typedef struct Array Array;

#define array_new(T,cap) __array_new(sizeof(T),cap)

// static Array *__array_new(int elem_size,int cap);

 
 

#ifdef __cplusplus
}
#endif

#endif /* ARRAY_H_ */