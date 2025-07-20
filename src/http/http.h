#ifndef HTTP_H_
#define HTTP_H_

// Description: Header file for http

#ifdef __cplusplus
extern "C" {
#endif


#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_POST "POST"

#define HTTP_STATUS_MSG_OK "OK"
#define HTTP_STATUS_MSG_NOT_FOUND "Not found"

// ====================================================================
// =========================== REQUEST ================================
// ====================================================================
/**
 * @brief 请求参数
 */
typedef struct HttpRequest HttpRequest;

/**
 * @brief 添加请求头
 */
char *http_request_get_header(HttpRequest *request, const char *key);

// ====================================================================
// =========================== RESPONSE ===============================
// ====================================================================

/**
 * @brief HTTP 返回数据
 */
typedef struct HttpResponse HttpResponse;

/**
 * @brief 写入返回数据
 * @return 成功写入的数量
 */
int http_response_write(HttpResponse *response, char *data);

/**
 * @brief 添加头
 */
void http_response_add_header(HttpResponse *response, char *key, char *value);

/**
 * @brief 设置
 */
void http_response_set_header(HttpResponse *response, char *key, char *value);

/**
 * @brief 获取头
 * @return 返回头内容
 */
char *http_response_get_header(HttpResponse *response, char *key);


// ====================================================================
// ============================= SERVER ===============================
// ====================================================================

/**
 * @brief http服务
 */
typedef struct HttpServer HttpServer;

/**
 * @brief HTTP 处理方法签名
 */
typedef void(*HttpHandler)(HttpRequest *, HttpResponse *);

/**
 * @brief 返回一个新的HTTP服务指针
 * @return HTTP 服务指针
 */
HttpServer *http_server_new();

/**
 * @brief 初始化HTTP服务器
 * @param server 指向HttpServer结构体的指针
 * @param host 绑定的主机地址
 * @param port 绑定的主机端口
 * @return 成功返回0，失败返回非0值
 */
int http_server_init(HttpServer *server, char *host, int port);

/**
 * @brief 启动HTTP服务器
 * @return 成功返回0,失败返回非0值
 */
int http_server_start(HttpServer *server);

/**
 * @brief 销毁HTTP服务
 * @return 成功返回0,失败返回非0值
 */
int http_server_destroy(HttpServer *server);

/**
 * @brief 添加HTTP路由
 * @return 添加成功返回0,失败返回-1
 */
int http_server_route_add(HttpServer *server, char *method , char *path, void (*handle)(HttpRequest *, HttpResponse *));

#ifdef __cplusplus
}
#endif

#endif /* HTTP_H_ */