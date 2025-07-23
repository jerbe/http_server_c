#include <stdio.h>
#include <stdlib.h>

#include "array.h"


/**
 * @brief 自定义数组
 */
typedef struct Array{
    void *data;
    int elem_size;
    int size;
    int cap;
} Array;


Array *__array_new(int elem_size,int cap){
    Array *arr = malloc(sizeof(Array));
    arr->elem_size = elem_size;
    arr->cap = cap;
    return arr;
}