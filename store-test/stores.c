#include <stdio.h>
#include <pthread.h>
//#include <atomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
//#include <unitypes.h>


typedef uint64_t atomic_t;

void inline atomic_inc(atomic_t *p)
{
    __sync_fetch_and_add(p, 1);
}

void inline atomic_set(atomic_t *p)
{
}


uint64_t shared;
atomic_t nready;
volatile int start;


void *thr_writer(void *data)
{

    atomic_inc(&nready);
    
    while (!(start));

    while ((start)) {
        shared = (uint64_t)data;
    }

    pthread_exit(0);
}

void *thr_reader(void *data)
{
    uint64_t w1 = (0x1ll << 32) | 0x1ll;
    uint64_t w2 = (0x2ll << 32) | 0x2ll;

    atomic_inc(&nready);
    
    while (!(start));

    while ((start)) {
        uint64_t read = shared;
        if (read != w1 && read != w2) {
            printf("WRONG READ: %llu\n", read);
        }
    }

    pthread_exit(0);
}


int main()
{
    nready = 0;

    pthread_t w1, w2, r;
    shared = (0x1ll << 32) | 0x1ll;
    pthread_create(&w1, NULL, thr_writer, (void *)((0x1ll << 32) | 0x1ll));
    pthread_create(&w2, NULL, thr_writer, (void *)((0x2ll << 32) | 0x2ll));
    pthread_create(&r,  NULL, thr_reader, NULL);

    while (3 != nready);

    start = 1;

    sleep(100);

    start = 0;

    return 0;
}
