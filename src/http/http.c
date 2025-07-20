
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>     
#include <errno.h>
#include <string.h>

#include "http.h"
#include "../thread_pool/thread_pool.h"
#include "../util/map.h"
#include "../util/util_string.h"


#define BUFF_SIZE 4096

// ====================================================================
// ============================ COMMON ================================
// ====================================================================
/**
 * @brief 设置头部信息，直接覆盖原先数据
 */
static void _http_set_header(map_str_t *map,const char *key, char *value){
    map_set(map, key, value);
}

/**
 * @brief 添加头部信息
 */
static void _http_add_header(map_str_t *map,const char *key, char *value){
    char *old = (char *)map_get(map, key);
    if(old == NULL)
    {
        _http_set_header(map,key, value);
        return;
    }

    char *new = str_append(old, value);
    map_set(map, key, new);
    free(old);
}


/**
 * @brief 设置头部信息
 */
static char * _http_get_header(map_str_t *map,const char *key){
    char *val = (char *)map_get(map, key);
    if(val == NULL){
        return "";
    }
    return val;
}


// ====================================================================
// =========================== REQUEST ================================
// ====================================================================

typedef struct HttpRequest {
    int client_fd;

    int remote_port;
    char *remote_addr;
    char *remote_host;

    char method[16];
    char path[255];

    map_str_t header;

    char buf[BUFF_SIZE];
    
} HttpRequest;

/**
 * @brief 添加请求头
 */
static void _http_request_add_header(HttpRequest *request,const char *key, char *value){
    _http_add_header(&request->header,key,value);
}

/**
 * @brief 获取指定请求头
 */
char *http_request_get_header(HttpRequest *request,const char *key){
    return _http_get_header(&request->header,key);
}


// ====================================================================
// =========================== RESPONSE ===============================
// ====================================================================

typedef struct HttpResponse {
    int status;
    char *body;

    map_str_t header;
} HttpResponse;

/**
 * @brief 初始化返回数据
 */
static void http_response_init(HttpResponse *response){
    response->status = 200;
    map_init(&response->header);
}

/** 
 * @brief 返回一个新的response,在堆里
 */
static HttpResponse *http_response_new(){
    HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
    http_response_init(response);
    return response;
}

/**
 * @brief 销毁一个response
 * 
 * @param response 
 */
static void http_response_destroy(HttpResponse *response){
    if(response->body!= NULL){
        free(response->body);
    }
    response->body =NULL;

    map_deinit(&response->header);
}

/**
 * @brief 添加头
 */
void http_response_add_header(HttpResponse *response,const char *key, char *value){
    _http_add_header(&response->header,key,value);
}

/**
 * @brief 设置头部信息
 */
void http_response_set_header(HttpResponse *response,const char *key, char *value){
    _http_set_header(&response->header,key,value);
}

/**
 * @brief 获取头
 * @return 返回头内容
 */
char *http_response_get_header(HttpResponse *response,const char *key){
    return _http_get_header(&response->header,key);
}

/**
 * @brief 往response注入灵魂
 */
int http_response_write(HttpResponse *response, char *data){
    if(response == NULL) return -1;

    response->body = malloc(sizeof(char)*strlen(data));
    strcpy(response->body, data);
    return 0;
}


// ====================================================================
// ============================= SERVER ===============================
// ====================================================================

typedef struct HttpServer {
    char *host; // 绑定的主机地址       // 8
    int port; // 服务器端口            // 4
    int socket_fd; // 套接字文件描述符  // 4

    ThreadPool *thread_pool; // 线程池 // 8

    map_void_t routes;               // 8
} HttpServer;


typedef struct handle_struct{
    HttpServer *svr;
    int client_fd;
} handle_struct;

/**
 * @brief 客户端返回
 */
static void response_to_client(int client_fd, HttpRequest *request, HttpResponse *response){
    char buf[BUFF_SIZE];
    const char *status_msg;
    const char *body = response->body;

    if(body == NULL){
        body = "";
    }
    
    switch (response->status)
    {
        case 404:
            status_msg = HTTP_STATUS_MSG_NOT_FOUND;
            break;
        
        default:
            status_msg = HTTP_STATUS_MSG_OK;
            break;
    }

    char len_str[10];
    snprintf(len_str, sizeof(len_str), "%zu", strlen(body));
    http_response_set_header(response,"Content-Length", len_str);

    char *header_str = malloc(sizeof(char*)*255);
    map_iter_t header_iter = map_iter();
    const char *key;
    while ((key = map_next(&response->header, &header_iter)) != NULL)
    {  
       char *tmp = header_str;
       header_str = str_append(key,": ");
       header_str = str_append(header_str,_http_get_header(&response->header,key));
       header_str = str_append(header_str,"\r\n");
       free(tmp);
    }

    sprintf(buf,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "%s"
        "\r\n"
        "%s",
        response->status, status_msg, header_str, body);

    write(client_fd, buf, strlen(buf));
    free(header_str);
}

/**
 * @brief 处理客户端
 */
static void *handle_client(void *arg) {
    handle_struct hs = *(handle_struct *)arg;
    int client_fd = hs.client_fd;

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(client_fd, (struct sockaddr *)&addr, &addr_len) == -1) {
       return NULL;
    }

    HttpRequest request;
    request.client_fd  = client_fd;

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
    int port = ntohs(addr.sin_port);
    int host_len = snprintf(NULL,0,"%s:%d",ip_str,port);

    request.remote_addr = ip_str;
    request.remote_port = port;
    request.remote_host = malloc(host_len+1);
    sprintf(request.remote_host, "%s:%d",ip_str,port);

    int bytes = read(client_fd, request.buf, sizeof(request.buf)-1);
    if(bytes <= 0){
        printf("链接已关闭; IP:%s Port:%d\n",request.remote_addr, request.remote_port);
        return NULL;
    }

    printf("接收到客户端请求; IP:%s Port:%d\n",request.remote_addr, request.remote_port);

    // 读取buf
    sscanf(request.buf, "%15s %255s", request.method, request.path);

    char *routeTmp = "%s %s";
    int l = snprintf(NULL, 0,routeTmp, request.method, request.path);
    char *route = malloc(sizeof(char)*l+1);
    sprintf(route,routeTmp, request.method, request.path);

    void *m_val = map_get(&hs.svr->routes, route);

    // 默认状态 200
    HttpResponse *response = http_response_new();

    http_response_init(response);
    if(m_val != NULL){
        HttpHandler handle = *(HttpHandler*)m_val;
        handle(&request, response);
        response_to_client(client_fd, &request, response);
    }else{
        response->status = 404;
        response_to_client(client_fd,&request, response);
    }
    http_response_destroy(response);
    response = NULL;
    close(client_fd); // 处理完毕后关闭客户端连接
    return NULL;
}

/**
 * @brief 返回一个新的HTTP服务指针
 * @return HTTP 服务指针
 */
HttpServer *http_server_new(){
    // printf("sizeof(HttpServer):%d\n",sizeof(HttpServer));
    // printf("sizeof(HttpServer*):%d\n",sizeof(HttpServer*));
    HttpServer *svr = malloc(sizeof(HttpServer));
    map_init(&svr->routes);
    return svr;
}

/**
 * @brief 初始化HTTP服务器
 * @param server 指向HttpServer结构体的指针
 * @param host 绑定的主机地址
 * @param port 绑定的主机端口
 * @return 成功返回0，失败返回非0值
 */
int http_server_init(HttpServer *server, char *host, int port) {
    if (host == NULL) {
       host = "0.0.0.0";
    }

    server->host = host; // 设置主机地址
    if (port <= 0 || port > 65535) {
        return -1; // 端口号无效
    }

    server->port = port;
    return 0; // 成功
}

/**
 * @brief 启动HTTP服务器
 * @return 成功返回0,失败返回非0值
 */
int http_server_start(HttpServer *server){
// 打开套接字
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 初始化服务
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(server->port), // 将端口号转换为网络字节序
        .sin_addr.s_addr = inet_addr(server->host) // 将主机地址转换为网络字节序
    };

    // 绑定 socket 到本地地址和端口，此时还未监听
    int brs = bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(brs == -1){
        printf("绑定失败: 原因:%s",strerror(errno));
        return brs;
    }

    // 开始监听连接，最大连接等待队列长度为 1000
    int lrs = listen(socket_fd, 1000); // 监听队列长度为1000
    if(lrs == -1){
        printf("监听失败: 原因:%s",strerror(errno));
        return lrs;
    }

    server->thread_pool = threadpool_create(10, 1000); // 创建线程池，10个线程，最大任务数1000
    if (server->thread_pool == NULL) {
        close(socket_fd); // 线程池创建失败，关闭套接字
        return -1;
    }

    while (1)
    {
        int clinet_fd = accept(socket_fd, NULL, NULL); // 接受连接请求
        if(clinet_fd > 0){
            int rs = threadpool_add_task(server->thread_pool, handle_client, &(handle_struct){server, clinet_fd});
            if(rs != 0){
                printf("无法加入进程,原因:%s\n",threadpool_error(rs));
                close(clinet_fd);
            }
        }
    }

    threadpool_destroy(server->thread_pool);
    return 0;
}

/**
 * @brief 销毁HTTP服务
 * @return 成功返回0,失败返回非0值
 */
int http_server_destroy(HttpServer *server){
    close(server->socket_fd);
    server->socket_fd=0;
    server->thread_pool = NULL;
    map_deinit(&server->routes);
    return 0;
}

/**
 * @brief 添加HTTP路由
 * @return 添加成功返回0,失败返回-1
 */
int http_server_route_add(HttpServer *server, char *method , char *path, void (*handle)(HttpRequest *, HttpResponse *)){
    int key_len = snprintf(NULL,0, "%s %s",method, path);
    char *key = malloc(sizeof(char)*key_len);
    sprintf(key,"%s %s",method, path);

    if(map_get(&server->routes, key)){
        return -1;
    }

    map_set(&server->routes, key, handle);
    return 0;
}

