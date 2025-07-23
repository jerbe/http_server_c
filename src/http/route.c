#include <stdio.h>
#include <stdlib.h>

#include "http.h"
#include "route.h"
#include "../util/map.h"

void route_test(HttpRequest *request, HttpResponse *response){
    map_iter_t itr = map_iter();

    char *key;
    while ((key = http_request_iter_header(request, &itr)) != NULL)
    {
        char *value = http_request_get_header(request, key);
        printf("%s:%s\n",key,value);
    }

    http_response_write(response, "你有新的消息，请注意查收!\r\n");
}