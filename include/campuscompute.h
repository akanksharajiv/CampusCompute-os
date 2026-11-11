#ifndef CAMPUSCOMPUTE_H
#define CAMPUSCOMPUTE_H

#include <stddef.h>

#define CC_MAX_JOBS 100
#define CC_MAX_PROCESSES 100
#define CC_MAX_RESOURCES 20
#define CC_EVENT_TEXT 256
#define CC_QUEUE_CAPACITY 8

/* Command returns: 0 expected demonstration completed; 1 runtime error;
 * 2 invalid input/usage; 3 Stage-1 feature not implemented.
 * A correctly detected unsafe state or simulated worker failure returns 0. */
enum { CC_OK = 0, CC_ERROR = 1, CC_USAGE = 2, CC_PENDING = 3 };
typedef enum { JOB_NEW, JOB_READY, JOB_RUNNING, JOB_COMPLETED } JobStatus;
typedef enum { POLICY_FCFS, POLICY_RR, POLICY_PRIORITY } SchedulePolicy;

typedef struct {
    int id;
    long long arrival, burst;
    int priority;
    JobStatus status;
    long long remaining, completion, turnaround, waiting;
} Job;

/* Copied by value into the bounded queue; no borrowed string pointers.
 * sequence is assigned under the queue mutex when an event is enqueued. */
typedef struct {
    unsigned long sequence;
    int producer_id;
    char text[CC_EVENT_TEXT];
} Event;

/* A: functions return CC_*; no module calls exit(). On load error, count=0.
 * Fixed arrays are caller-owned. Reject overflow rather than truncate. */
int jobs_load(const char *path, Job jobs[CC_MAX_JOBS], size_t *count);
int schedule_run(SchedulePolicy policy, const Job *jobs, size_t count,
                 long long quantum);
int schedule_compare(const Job *jobs, size_t count, long long quantum);

/* A: opaque logger; NULL on start failure. submit copies text and blocks if
 * full. stop closes submissions, drains, joins consumer, releases memory.
 * All producer threads must be joined before stop. Parent-process use only. */
typedef struct Logger Logger;
Logger *logger_start(const char *path);
int logger_submit(Logger *logger, int producer_id, const char *text);
int logger_stop(Logger *logger);
int logger_demo(void);

/* B: modules print their meaningful evidence and release owned resources.
 * No fork while logger threads are alive. Children never use Logger. */
int ipc_demo(void);
int banker_run(const char *path);
int worker_demo(int simulate_failure);
int limit_demo(void);

/* Shared CLI integration: A owns main.c; B reviews process sequencing. */
int integrated_demo(void);
#endif
