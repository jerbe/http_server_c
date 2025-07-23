#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <locale.h>
#include <pthread.h>
#include <unistd.h>

#include "util/sys.h"
#include "http/http.h"
#include "http/route.h"
#include "util/util_string.h"
#include "util/map.h"
#include "util/array.h"

HttpServer *http_svr;

/**
 * @brief 初始化随机数种子
 * @details 该函数用于初始化随机数生成器的种子
 */
void rand_init(){
    time_t seed = time(NULL);
    srand(seed);
}


void *print_system(){
    for(;;){
        print_memory_usage();
        sleep(5);
    }
}

/**
 * @brief 初始化打印系统数据
 */
void print_system_init(){
    pthread_t pt;

    pthread_create(&pt,NULL, print_system,NULL);
}

/**
 * @brief 初始化函数
 * @details 该函数用于初始化程序的全局状态，包括随机数种子等
 */
void init(){
    rand_init();

    http_svr = http_server_new();
    http_server_init(http_svr,"127.0.0.1", 8088);    

    print_system_init();

    // 其他初始化操作
    printf("初始化完成.\n");
}

/**
 * @brief 运行
 */
void run(){
    http_server_route_add(http_svr,HTTP_METHOD_GET, "/test", route_test);
    http_server_start(http_svr);
}

/**
 * @brief 销毁
 */
void destroy(){
    http_server_destroy(http_svr);
}


void test(){
    // char *dest = "hello";

    // while (*dest)
    // {
    //     dest++;
    //     printf("%s %d %p\n",dest, dest, dest);
    // }
    
    // char *src = "world";
    // while (*dest++ = *src++)
    // {
    //     printf("%s %d %p\n",dest, dest, dest);
    // }

    printf("sizeof(char):%ld\n",sizeof(char));
    printf("sizeof(char*):%ld\n",sizeof(char*));
    
    printf("sizeof(int):%ld\n",sizeof(int));
    printf("sizeof(int*):%ld\n",sizeof(int*));

    printf("sizeof(long):%ld\n",sizeof(long));
    printf("sizeof(long*):%ld\n",sizeof(long*));

    printf("sizeof(float):%ld\n",sizeof(float));
    printf("sizeof(float*):%ld\n",sizeof(float*));
    
    printf("sizeof(double):%ld\n",sizeof(double));
    printf("sizeof(double*):%ld\n",sizeof(double*));

    // char str[20];
    // printf("sizeof(str):%d\n",sizeof(str));

    // char *str1[20];
    // printf("sizeof(str1):%d\n",sizeof(str1));

    // // printf("sizeof(HttpServer):%d\n",sizeof(HttpServer));
    // printf("sizeof(HttpServer*):%d\n",sizeof(HttpServer*));

    // map_char_t map;

    // map_init(&map);

    // map_set(&map, "1","2");
    // map_set(&map, "1","2");
    
    // char *new = str_append(dest, "jajsdjadadadad");
    // printf("dest:%s(%d,%p)  new:%s(%d,%p)\n",dest,dest,dest, new,new,new);
    //   char *str1 = "Hello, ";
    //   char str2[] = "world!";
    //   size_t n = 5; // 只追加前5个字符

    //   // 将 str2 的前5个字符追加到 str1
    //   strncat(str1, str2, n);

    //   printf("追加后的字符串: %s\n", str1); // 输出: Hello, world!
}


void repeat_string(char *dest, const char *src, size_t n) {
    size_t len = strlen(src);
    for (size_t i = 0; i < n; i++) {
        strncat(dest, src, len);
    }
}


int main(){
    // setlocale(LC_ALL, ""); // 设置为当前环境的locale，通常会自动处理UTF-8编码问题
    test();

    Array *arr = array_new(char, 12);
    
    
    // init(); // 调用初始化函数
    
    // run();

    // destroy();
    
    return 0;
}