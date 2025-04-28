#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#include <pthread.h>

static pthread_mutex_t running_lock = PTHREAD_MUTEX_INITIALIZER;

int empty(struct queue_t * q) {
    if (q == NULL) return 1;
    return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
    /* TODO: put a new process to queue [q] */
    if (q->size < MAX_QUEUE_SIZE) {
        q->proc[q->size++] = proc;
    }
}

struct pcb_t * dequeue(struct queue_t * q) {
    /* TODO: return a pcb whose prioprity is the highest
     * in the queue [q] and remember to remove it from q
     * */
    if (empty(q)) return NULL;

    int max_idx = 0;
    for (int i = 1; i < q->size; i++) {
        if (q->proc[i]->priority < q->proc[max_idx]->priority) {
            max_idx = i;
        }
    }

    struct pcb_t *proc = q->proc[max_idx];
    for (int i = max_idx; i < q->size - 1; i++) {
        q->proc[i] = q->proc[i + 1];
    }
    q->proc[q->size - 1] = NULL;
    q->size--;

    return proc;
}

void remove_proc(struct queue_t * q, struct pcb_t * proc) {
    if (empty(q)) return;
    int idx = -1;

    for (int i = 0 ; i < q->size ; i ++) {
        if (q->proc[i]->pid == proc->pid) {
            idx = proc->pid;
            break;
        }
    }

    if (idx == -1) return;

    for (int i = idx ; i < q->size - 1 ; i ++) {
        q->proc[i] = q->proc[i + 1];
    }
    q->proc[q->size - 1] = NULL;
    q->size --;
}

void put_running_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&running_lock);
	enqueue(proc->running_list, proc);
	pthread_mutex_unlock(&running_lock);
}

void remove_running_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&running_lock);
	remove_proc(proc->running_list, proc);
	pthread_mutex_unlock(&running_lock);
}