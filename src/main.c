#include "campuscompute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

static void usage(FILE *out)
{
    fputs("CampusCompute - OS Mini Project\n"
          "  campuscompute help\n"
          "  campuscompute schedule fcfs|priority FILE\n"
          "  campuscompute schedule rr FILE QUANTUM\n"
          "  campuscompute compare FILE  (RR quantum=2)\n"
          "  campuscompute logger|ipc|limit|demo\n"
          "  campuscompute banker FILE\n"
          "  campuscompute worker normal|fail\n", out);
}

static int quantum_parse(const char *s, long long *value)
{
    char *end;
    errno = 0;
    *value = strtoll(s, &end, 10);
    return errno == 0 && end != s && *end == '\0' && *value > 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "help") == 0) {
        usage(stdout); return CC_OK;
    }
    if (argc >= 2 && strcmp(argv[1], "schedule") == 0) {
        SchedulePolicy policy;
        long long quantum = 0;
        Job jobs[CC_MAX_JOBS];
        size_t count = 0;
        if (argc == 4 && strcmp(argv[2], "fcfs") == 0)
            policy = POLICY_FCFS;
        else if (argc == 4 && strcmp(argv[2], "priority") == 0)
            policy = POLICY_PRIORITY;
        else if (argc == 5 && strcmp(argv[2], "rr") == 0 &&
                 quantum_parse(argv[4], &quantum))
            policy = POLICY_RR;
        else goto bad_usage;
        int rc = jobs_load(argv[3], jobs, &count);
        return rc == CC_OK ? schedule_run(policy, jobs, count, quantum) : rc;
    }
    if (argc == 3 && strcmp(argv[1], "compare") == 0) {
        Job jobs[CC_MAX_JOBS]; size_t count = 0;
        int rc = jobs_load(argv[2], jobs, &count);
        return rc == CC_OK ? schedule_compare(jobs, count, 2) : rc;
    }
    if (argc == 3 && strcmp(argv[1], "banker") == 0)
        return banker_run(argv[2]);
    if (argc == 3 && strcmp(argv[1], "worker") == 0) {
        if (strcmp(argv[2], "normal") == 0) return worker_demo(0);
        if (strcmp(argv[2], "fail") == 0) return worker_demo(1);
        goto bad_usage;
    }
    if (argc == 2) {
        if (strcmp(argv[1], "logger") == 0) return logger_demo();
        if (strcmp(argv[1], "ipc") == 0) return ipc_demo();
        if (strcmp(argv[1], "limit") == 0) return limit_demo();
        if (strcmp(argv[1], "demo") == 0) return integrated_demo();
    }
bad_usage:
    fputs("Error: invalid command, arguments, or time quantum.\n", stderr);
    usage(stderr);
    return CC_USAGE;
}
