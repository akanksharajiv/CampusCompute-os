#include "campuscompute.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Logger {
    Event queue[CC_QUEUE_CAPACITY];
    size_t head, used;
    unsigned long next_sequence;
    int closed, error;
    FILE *file;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty, not_full;
    pthread_t consumer;
};

static void *consume(void *arg)
{
    Logger *l = arg;
    for (;;) {
        pthread_mutex_lock(&l->mutex);
        while (l->used == 0 && !l->closed)
            pthread_cond_wait(&l->not_empty, &l->mutex);
        if (l->used == 0 && l->closed) {
            pthread_mutex_unlock(&l->mutex);
            break;
        }
        Event e = l->queue[l->head];
        l->head = (l->head + 1) % CC_QUEUE_CAPACITY;
        --l->used;
        pthread_cond_signal(&l->not_full);
        pthread_mutex_unlock(&l->mutex);
        if (fprintf(l->file, "Event %lu | Producer %d | %s\n",
                    e.sequence, e.producer_id, e.text) < 0) l->error = 1;
        printf("Consumed event %lu from producer %d\n", e.sequence, e.producer_id);
    }
    if (fflush(l->file) != 0) l->error = 1;
    puts("Consumer terminated cleanly");
    return NULL;
}

Logger *logger_start(const char *path)
{
    if (!path) return NULL;
    Logger *l = calloc(1, sizeof(*l));
    if (!l) return NULL;
    l->file = fopen(path, "w");
    if (!l->file) { perror("fopen"); free(l); return NULL; }
    if (pthread_mutex_init(&l->mutex, NULL) != 0) goto fail_file;
    if (pthread_cond_init(&l->not_empty, NULL) != 0) goto fail_mutex;
    if (pthread_cond_init(&l->not_full, NULL) != 0) goto fail_empty;
    if (pthread_create(&l->consumer, NULL, consume, l) != 0) goto fail_full;
    return l;
fail_full:
    pthread_cond_destroy(&l->not_full);
fail_empty:
    pthread_cond_destroy(&l->not_empty);
fail_mutex:
    pthread_mutex_destroy(&l->mutex);
fail_file:
    fclose(l->file);
    free(l);
    return NULL;
}

int logger_submit(Logger *l, int producer_id, const char *text)
{
    if (!l || !text || strlen(text) >= CC_EVENT_TEXT) return CC_USAGE;
    pthread_mutex_lock(&l->mutex);
    while (l->used == CC_QUEUE_CAPACITY && !l->closed)
        pthread_cond_wait(&l->not_full, &l->mutex);
    if (l->closed) { pthread_mutex_unlock(&l->mutex); return CC_ERROR; }
    size_t tail = (l->head + l->used) % CC_QUEUE_CAPACITY;
    Event *e = &l->queue[tail];
    e->sequence = ++l->next_sequence;
    e->producer_id = producer_id;
    strcpy(e->text, text);
    ++l->used;
    printf("Produced event %lu from producer %d\n", e->sequence, producer_id);
    pthread_cond_signal(&l->not_empty);
    pthread_mutex_unlock(&l->mutex);
    return CC_OK;
}

int logger_stop(Logger *l)
{
    if (!l) return CC_USAGE;
    pthread_mutex_lock(&l->mutex);
    l->closed = 1;
    pthread_cond_broadcast(&l->not_empty);
    pthread_cond_broadcast(&l->not_full);
    pthread_mutex_unlock(&l->mutex);
    int join_rc = pthread_join(l->consumer, NULL);
    int file_rc = fclose(l->file);
    int result = (join_rc || file_rc || l->error) ? CC_ERROR : CC_OK;
    pthread_cond_destroy(&l->not_empty);
    pthread_cond_destroy(&l->not_full);
    pthread_mutex_destroy(&l->mutex);
    free(l);
    return result;
}

typedef struct { Logger *logger; int id; int result; } ProducerArgs;
static void *produce(void *arg)
{
    ProducerArgs *a = arg;
    a->result = CC_OK;
    for (int i = 1; i <= 6; ++i) {
        char msg[CC_EVENT_TEXT];
        snprintf(msg, sizeof(msg), "Job activity %d", i);
        if (logger_submit(a->logger, a->id, msg) != CC_OK) {
            a->result = CC_ERROR;
            break;
        }
    }
    return NULL;
}

int logger_demo(void)
{
    Logger *l = logger_start("activity.log");
    if (!l) return CC_ERROR;
    pthread_t threads[2];
    ProducerArgs args[2] = {{l, 1, CC_OK}, {l, 2, CC_OK}};
    int created = 0, rc = CC_OK;
    for (int i = 0; i < 2; ++i) {
        if (pthread_create(&threads[i], NULL, produce, &args[i]) != 0) {
            fputs("Error creating producer thread\n", stderr);
            rc = CC_ERROR;
            break;
        }
        ++created;
    }
    for (int i = 0; i < created; ++i) pthread_join(threads[i], NULL);
    for (int i = 0; i < created; ++i)
        if (args[i].result != CC_OK) rc = CC_ERROR;
    if (logger_stop(l) != CC_OK) rc = CC_ERROR;
    printf("Logger: %d producer threads joined; log saved to activity.log\n", created);
    return rc;
}
