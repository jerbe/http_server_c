#ifndef ROUTE_H_
#define ROUTE_H_

#include "http.h"

// Description: Header file for route

#ifdef __cplusplus
extern "C" {
#endif

void route_test(HttpRequest *request, HttpResponse *response);

#ifdef __cplusplus
}
#endif

#endif /* ROUTE_H_ */