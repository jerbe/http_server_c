#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sys.h"


void print_memory_usage() {
    FILE* file = fopen("/proc/self/status", "r");
    if (!file) return;
    printf("--------------------------------------------------\n");
    printf("--------------------- MEMORY ---------------------\n");
    printf("--------------------------------------------------\n");

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // VmRSS 是实际占用的物理内存，VmSize 是虚拟内存
        if (strncmp(line, "VmRSS:", 6) == 0 || strncmp(line, "VmSize:", 7) == 0)
            printf("%s", line);
    }

    fclose(file);
}