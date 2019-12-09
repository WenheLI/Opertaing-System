#include "rpi.h"

#ifndef E
#	error "Client must define the Q datatype <E>"
#endif



typedef struct Queue{
    E* nodes[50];
    int cursor;
} Queue;

static void Q_append(Queue* q, E* e) {
    q -> nodes[q -> cursor] = e;
    q -> cursor ++;
}

static E* Q_pop(Queue* q) {
    if (q -> cursor <= 0) return 0;
    E* temp = q -> nodes[0];
    for (int i = 1; i < q -> cursor; i++) {
        q -> nodes[i-1] = q -> nodes[i];
    }
    q -> cursor -= 1;
    return temp;
}

static E* Q_peek(Queue* q) {
    return q -> nodes[1];
}

static E* Q_tail(Queue* q) {
    return q -> nodes[q -> cursor - 1];
}




