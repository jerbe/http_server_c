#include <stdlib.h>
#include <string.h>

#include "util_string.h"


/**
 * @brief 追加字符串
 */
char *str_append(const char *dest, const char *src){
    int len = strlen(dest) + strlen(src)+1;
    char *new = malloc(sizeof(char)*len);
    return strcat(strcpy(new,dest),src);
}