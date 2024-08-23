#include "common.h"
#include "monitor.h"

void memset(u8int *ptr, int val, int num_bytes){
    for (int i = 0; i < num_bytes; i++)
    {
        *(ptr + i) = val;
    }

    return;
}

void PANIC(char *s){
    monitor_write(s);
    monitor_put('\n');

    while(1);
}