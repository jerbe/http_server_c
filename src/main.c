#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "http/http.h"
#include "http/route.h"
#include "util/util_string.h"
#include "util/map.h"

HttpServer *http_svr;

/**
 * @brief 初始化随机数种子
 * @details 该函数用于初始化随机数生成器的种子
 */
void init_rand(){
    time_t seed = time(NULL);
    srand(seed);
}

/**
 * @brief 初始化函数
 * @details 该函数用于初始化程序的全局状态，包括随机数种子等
 */
void init(){
    init_rand();

    http_svr = http_server_new();
    http_server_init(http_svr,"127.0.0.1", 8088);    

    // 其他初始化操作
    printf("Initialization complete.\n");
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

    printf("sizeof(char):%d\n",sizeof(char));
    printf("sizeof(char*):%d\n",sizeof(char*));
    
    printf("sizeof(int):%d\n",sizeof(int));
    printf("sizeof(int*):%d\n",sizeof(int*));

    printf("sizeof(long):%d\n",sizeof(long));
    printf("sizeof(long*):%d\n",sizeof(long*));

    printf("sizeof(float):%d\n",sizeof(float));
    printf("sizeof(float*):%d\n",sizeof(float*));
    
    printf("sizeof(double):%d\n",sizeof(double));
    printf("sizeof(double*):%d\n",sizeof(double*));

    char str[20];
    printf("sizeof(str):%d\n",sizeof(str));

    char *str1[20];
    printf("sizeof(str1):%d\n",sizeof(str1));

    // printf("sizeof(HttpServer):%d\n",sizeof(HttpServer));
    printf("sizeof(HttpServer*):%d\n",sizeof(HttpServer*));

    map_char_t map;

    map_init(&map);

    map_set(&map, "1","2");
    map_set(&map, "1","2");
    

    // char *new = str_append(dest, "jajsdjadadadad");
    // printf("dest:%s(%d,%p)  new:%s(%d,%p)\n",dest,dest,dest, new,new,new);




//   char *str1 = "Hello, ";
//   char str2[] = "world!";
//   size_t n = 5; // 只追加前5个字符

//   // 将 str2 的前5个字符追加到 str1
//   strncat(str1, str2, n);

//   printf("追加后的字符串: %s\n", str1); // 输出: Hello, world!




}

int main(){
    test();

    init(); // 调用初始化函数
    
    // run();

    // destroy();
    
    return 0;
}