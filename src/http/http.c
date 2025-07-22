
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

#define MAX_LINE_SIZE 1024
#define MAX_HEADER_SIZE 4096
#define MAX_BODY_SIZE 1048576

// ====================================================================
// ============================ COMMON ================================
// ====================================================================
/**
 * @brief 设置头部信息，直接覆盖原先数据
 */
static void _http_set_header(map_str_t *map, const char *key, char *value){
    size_t len = sizeof(char) * strlen(value)+1;
    char *tmp = malloc(len);
    memset(tmp,0,len);
    map_set(map, key, memcpy(tmp, value, len));
}

/**
 * @brief 添加头部信息
 */
static void _http_add_header(map_str_t *map, const char *key, char *value){
    char **old = (char **)map_get(map, key);
    if(old == NULL)
    {
        _http_set_header(map, key, value);
        return;
    }
    char *old_val = *old;
    char *new = str_append(*old, value);
    free(old_val);
    old_val = new;
    
    new = str_append(new, ";");
    free(old_val);
    map_set(map, key, new);
    
}


/**
 * @brief 设置头部信息
 */
static char *_http_get_header(map_str_t *map, const char *key){
    char **val = (char **)map_get(map, key);
    if(val == NULL){
        return "";
    }
    return *val;
}

/**
 * @brief 清理头部信息
 * 
 * @param map 
 */
static void _http_clear_header(map_str_t *map){
    map_iter_t iter = map_iter();
    const char *key;
    while ((key = map_next(map, &iter)) != NULL)
    {
        char **val = (char **)map_get(map, key);
        if(val != NULL){
            free(*val);
            *val = NULL;
        }
    }
}


// ====================================================================
// =========================== REQUEST ================================
// ====================================================================

typedef struct HttpRequest {
    int client_fd;

    int remote_port;
    char *remote_addr;
    char *remote_host;

    char method[8];
    char path[255];

    map_str_t header;
    map_str_t get_data;

    // get 数据已经初始化过
    char get_data_init;


    char *body_data;
    
} HttpRequest;


static void _pull_end_flag(char *flag, size_t len, char c){
    if(len > 1){
        for(int i = 0; i < len-1;i++){
            flag[i]=flag[i+1];
        }
    }
    flag[len-1] = c;
}

/**
 * @brief 初始化请求
 */
static int http_request_init(HttpRequest *request, int client_fd){
    request->client_fd  = client_fd;
    map_init(&request->header);

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(client_fd, (struct sockaddr *)&addr, &addr_len) == -1) {
       return -1;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
    request->remote_addr = malloc(sizeof(char)*INET_ADDRSTRLEN+1);
    strcpy(request->remote_addr, ip_str);

    int port = ntohs(addr.sin_port);
    request->remote_port = port;

    int host_len = snprintf(NULL,0,"%s:%d",ip_str, port);
    request->remote_host = malloc(host_len+1);
    sprintf(request->remote_host, "%s:%d",ip_str, port);

    
    // 读取第一行 直到第一个 \r\n
    char line_data[MAX_LINE_SIZE];
    int line_read = 0;
    char line_end_flag[2] = {0};
    for(;line_read < MAX_LINE_SIZE-2;){
        int n = read(client_fd, line_data+line_read,1);
        if(n <= 0){
            return -1;
        }

        _pull_end_flag(line_end_flag, 2, line_data[line_read]);
        line_read += n;

        if(line_read >=2 && strncmp(line_end_flag,"\r\n",2) == 0){
            break;
        }
    }
    line_data[line_read] = '\0';
    if(!sscanf(line_data,"%7s %1024s ",request->method,request->path)){
        return -1;
    }

    // 1. 读取请求头（直到 \r\n\r\n）
    char header_data[MAX_HEADER_SIZE];
    int header_read = 0;
    char header_end_flag[4] = {0};
    int header_line_offset = 0;
    for(;header_read < MAX_HEADER_SIZE-4;){
        int n = read(client_fd, header_data + header_read,1);
        if(n <= 0){
            return -1;
        }
        _pull_end_flag(line_end_flag, 2, header_data[header_read]);
        _pull_end_flag(header_end_flag, 4, header_data[header_read]);

        header_read += n;
        // 行结束
        if(strncmp(line_end_flag,"\r\n",2) == 0){
            int l = header_read - header_line_offset-1;
            char *header_entry = malloc(sizeof(char)+l);
            strcpy(header_entry, header_data + header_line_offset);
            header_entry[l-1] = '\0';

            // 查找第一个冒号位置
            char *col = strchr(header_entry,':');
            if(col){
                *col = '\0';  // 将冒号填充成字符串结束
                char *key = header_entry;
                char *value = col+1;

                while (*value == ' '|| *value == '\r' || *value == '\n')// 如果是空白或者换行，则指针往前推
                {
                    value++;
                }

                _http_add_header(&request->header, key, value);
            }

            free(header_entry);
            header_entry = NULL;
            header_line_offset = header_read;
        }
        
        // strstr 找寻字符串第一次出现
        if(header_read >= 4 && strncmp(header_end_flag,"\r\n\r\n",4) == 0){
            break;
        }
    }
    header_data[header_read] = '\0';

    map_iter_t iter = map_iter();
    const *key = map_next(&request->header, &iter);
    // while ((key = map_next(&request->header,&iter) != NULL))
    // {
    //     printf("%s:%s\n",key,_http_get_header(&request->header,key));
    // }
    




    // 遍历 Content-Length 查找body长度
    char *content_length_str = _http_get_header(&request->header,"Content-Length");
    int content_length = 0;
    if(content_length_str){
        content_length = atoi(content_length_str);
        if(content_length < 0 || content_length > MAX_BODY_SIZE){
            return -1;
        }
    }

    // 读取body
    size_t content_length_t = sizeof(char)*content_length+1;
    char *body = malloc(content_length_t);
    memset(body, 0, content_length_t);
    if(body == NULL){
        return -1;
    }

    int body_read = 0;
    for(;body_read < content_length;){
        int n = read(client_fd, body+body_read, content_length-body_read);
        if(n <= 0){
            free(body);
            return -1;
        }
        body_read += n;
    }

    request->body_data = body;

    return 0;
}

/**
 * @brief 销毁请求头
 */
static void http_request_destroy(HttpRequest *request){
    request->client_fd=0;
    request->remote_port=0;

    if(request->remote_addr != NULL){
        free(request->remote_addr);
        request->remote_addr = NULL;
    }

    if(request->remote_host != NULL){
        free(request->remote_host);
        request->remote_host = NULL;
    }

    _http_clear_header(&request->header);
    map_deinit(&request->header);
    request = NULL;
}

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

/**
 * @brief 迭代请求头
 * @return 返回key，可能NULL
 */
const char *http_request_iter_header(HttpRequest *request, map_iter_t *iter_t){
    return map_next(&request->header, iter_t);
}

/**
 * @brief 获取指定的请求参数
 */
char *http_request_get(HttpRequest *request, const char *key){
    // 取得值
    char **value = map_get(&request->get_data,key);
    if(value != NULL){
        return *value;
    }

    // 判断是否已经有系列化过
    if(value == NULL && request->get_data_init){
        return NULL;
    }

    // TODO 需要初始化
    return NULL;
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
    response->body = NULL;
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
    response->status = 0;

    _http_clear_header(&response->header);
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
    char buf[MAX_HEADER_SIZE];
    char *status_msg;
    char *body = response->body;

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

    char *header_str = malloc(sizeof(char)*256);
    map_iter_t header_iter = map_iter();
    const char *key;
    while ((key = map_next(&response->header, &header_iter)) != NULL)
    {  
        char *tmp = header_str;
        header_str = str_append(key,": ");
        free(tmp);
        tmp = header_str;
        header_str = str_append(header_str,_http_get_header(&response->header,key));
        free(tmp);
        tmp = header_str;
        header_str = str_append(header_str,"\r\n");
        free(tmp);  
    }
    key = NULL;

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
    header_str = NULL;
    status_msg = NULL;
    body = NULL;
}

/**
 * @brief 处理客户端
 */
static void *run_client_handle(void *arg) {
    handle_struct *hs = (handle_struct *)arg;
    int client_fd = hs->client_fd;

    HttpRequest request;
    http_request_init(&request,client_fd);
    
    char *routeTmp = "%s %s";
    int l = snprintf(NULL, 0, routeTmp, request.method, request.path);
    char route[l+1];
    sprintf(route, routeTmp, request.method, request.path);

    void *m_val = map_get(&hs->svr->routes, route);
    // // 默认状态 200
    HttpResponse response;
    http_response_init(&response);

    if(m_val != NULL ){
        HttpHandler handle = *(HttpHandler*)m_val;
        handle(&request, &response);
        response_to_client(client_fd, &request, &response);
    }else{
        response.status = 404;
        response_to_client(client_fd,&request, &response);
    }
    http_request_destroy(&request);
    http_response_destroy(&response);

    close(client_fd); // 处理完毕后关闭客户端连接
    return NULL;
}

/**
 * @brief 
 */
static void client_handle_done(void *arg){
    handle_struct *s = (handle_struct *)arg;
    s->client_fd =0;
    s->svr = NULL;
    free(s);
    s = NULL;
}

/**
 * @brief 返回一个新的HTTP服务指针
 * @return HTTP 服务指针
 */
HttpServer *http_server_new(){
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

    int yes = 1;
    int rs = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (rs < 0) {
        printf("设置socket失败: 原因:%s",strerror(errno));
        return rs;
    }

    // 绑定 socket 到本地地址和端口，此时还未监听
    rs = bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(rs < 0){
        printf("绑定失败: 原因:%s",strerror(errno));
        return rs;
    }

    // 开始监听连接，最大连接等待队列长度为 1000
    rs = listen(socket_fd, 1000); // 监听队列长度为1000
    if(rs < 0){
        printf("监听失败: 原因:%s",strerror(errno));
        return rs;
    }

    server->thread_pool = threadpool_new(10, 2, client_handle_done); // 创建线程池，10个线程，最大任务数1000
    if (server->thread_pool == NULL) {
        close(socket_fd); // 线程池创建失败，关闭套接字
        return -1;
    }

    while (1)
    {
        int clinet_fd = accept(socket_fd, NULL, NULL); // 接受连接请求
        if(clinet_fd > 0){
            handle_struct *handle_arg = malloc(sizeof(handle_struct));
            handle_arg->client_fd = clinet_fd;
            handle_arg->svr = server;
            int rs = threadpool_add_task(server->thread_pool, run_client_handle, handle_arg);
            if(rs != 0){
                // printf("无法加入进程,原因:%s\n",threadpool_error(rs));
                client_handle_done(handle_arg);
                close(clinet_fd);
            }
            handle_arg = NULL;
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

