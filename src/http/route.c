#include "http.h"
#include "route.h"


void route_test(HttpRequest *request, HttpResponse *response){
    http_response_write(response, "你有新的消息，请注意查收!");
}