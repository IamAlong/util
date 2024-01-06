#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/errno.h>

typedef struct IrQueueNode
{
    void*               data;
    struct IrQueueNode* next;
} IrQueueNode_t;


typedef struct IrQueue
{
    pthread_mutex_t mtx;
    IrQueueNode_t*  head;
    IrQueueNode_t*  bottom;
} IrQueue_t;

IrQueue_t* create_ir_queue();
bool destory_ir_queue(IrQueue_t* ir_queue);
bool push_back(IrQueue_t* ir_queue, void* d);
bool front(IrQueue_t* ir_queue, void** d);
bool pop_front(IrQueue_t* ir_queue);
bool empty(IrQueue_t* ir_queue);

IrQueue_t* create_ir_queue()
{
    IrQueue_t* ir_queue = (IrQueue_t*)malloc(sizeof(IrQueue_t));
    if (ir_queue == NULL)
    {
        return NULL;
    }
    pthread_mutex_init(&ir_queue->mtx, NULL);
    ir_queue->head = NULL;
    ir_queue->bottom = NULL;
}

bool destory_ir_queue(IrQueue_t* ir_queue)
{
    if (ir_queue == NULL)
    {
        return false;
    }
    if (ir_queue->head == NULL)
    {
        return true;
    }
    while (ir_queue->head != NULL)
    {
        pop_front(ir_queue);
    }
    pthread_mutex_destroy(&ir_queue->mtx);
    free(ir_queue);
    return true;
}

bool push_back(IrQueue_t* ir_queue, void* d)
{
    if (ir_queue == NULL)
    {
        return false;
    }
    IrQueueNode_t* node = (IrQueueNode_t*)malloc(sizeof(IrQueueNode_t));
    if (node == NULL)
    {
        return false;
    }
    node->data = d;
    node->next = NULL;
    pthread_mutex_lock(&ir_queue->mtx);
    // empty queue
    if (ir_queue->head == NULL)
    {
        ir_queue->head = node;
        ir_queue->bottom = node;
    }
    else
    {
        ir_queue->bottom->next = node;
        ir_queue->bottom = node;
    }
    pthread_mutex_unlock(&ir_queue->mtx);
    return true;
}

bool front(IrQueue_t* ir_queue, void** d)
{
    if (ir_queue == NULL || d == NULL)
    {
        return false;
    }
    pthread_mutex_lock(&ir_queue->mtx);
    if (ir_queue->head == NULL)
    {
        pthread_mutex_unlock(&ir_queue->mtx);
        return false;
    }
    *d = ir_queue->head->data;
    pthread_mutex_unlock(&ir_queue->mtx);
    return true;
}

bool pop_front(IrQueue_t* ir_queue)
{
    if (ir_queue == NULL)
    {
        return false;
    }
    pthread_mutex_lock(&ir_queue->mtx);
    if (ir_queue->head != NULL)
    {
        IrQueueNode_t* node = ir_queue->head;
        if (ir_queue->head == ir_queue->bottom)
        {
            ir_queue->bottom = NULL;
        }
        ir_queue->head = ir_queue->head->next;
        free(node);
    }

    pthread_mutex_unlock(&ir_queue->mtx);
    return true;
}

bool empty(IrQueue_t* ir_queue)
{
    if (ir_queue == NULL)
    {
        return true;
    }
    pthread_mutex_lock(&ir_queue->mtx);
    if (ir_queue->head == NULL)
    {
        pthread_mutex_unlock(&ir_queue->mtx);
        return true;
    }
    pthread_mutex_unlock(&ir_queue->mtx);
    return false;
}

bool running = false;
pthread_mutex_t data_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* thread1(void* data)
{
    IrQueue_t* ir_queue = (IrQueue_t*)data;
    struct timespec tm;
    tm.tv_sec = 1;
    while(running)
    {
        pthread_mutex_lock(&data_mtx);
        if (empty(ir_queue))
        {
            if (ETIMEDOUT == pthread_cond_timedwait(&cond, &data_mtx, &tm))
            {
                pthread_mutex_unlock(&data_mtx);
                continue;
            }
        }
        int* d = NULL;
        bool ret = front(ir_queue, (void**)&d);
        if (ret)
        {
            printf("%d\n", *d);
            pop_front(ir_queue);
            free(d);
            d = NULL;
        }
        else
        {
            printf("@@@@@@\n");
        }
        
        pthread_mutex_unlock(&data_mtx);
    }
    return NULL;
}

void* thread2(void* data)
{
    IrQueue_t* ir_queue = (IrQueue_t*)data;
    int i = 0;
    for (; i < 10; i++)
    {
        int* d = (int*)malloc(sizeof(int));
        *d = i;
        pthread_mutex_lock(&data_mtx);
        push_back(ir_queue, d);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&data_mtx);
    }
    
    sleep(1);
    running = false;
    
    destory_ir_queue(ir_queue);
    return NULL;
}

int main()
{
    running = true;
    pthread_t t1, t2;
    IrQueue_t* ir_queue = create_ir_queue();
    pthread_create(&t2, NULL, thread2, ir_queue);
    pthread_create(&t1, NULL, thread1, ir_queue);
    pthread_detach(t1);
    pthread_detach(t2);
    sleep(1);
    pthread_mutex_destroy(&data_mtx);
    pthread_cond_destroy(&cond);
    return 0;
}