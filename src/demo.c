#include "campuscompute.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    int status;
} DemoResult;

static int record(Logger *logger, const DemoResult *result)
{
    char message[CC_EVENT_TEXT];
    int written = snprintf(message, sizeof message, "%s: %s (return code %d)",
                           result->name,
                           result->status == CC_OK ? "PASS" : "FAIL",
                           result->status);
    if (written < 0 || (size_t)written >= sizeof message)
        return CC_ERROR;
    return logger_submit(logger, 0, message);
}

int integrated_demo(void)
{
    DemoResult results[8];
    size_t count = 0;
    Job jobs[CC_MAX_JOBS];
    size_t jobs_count = 0;
    int rc;

    puts("=== CampusCompute integrated demonstration ===");
    puts("Run from the project directory (where tests/ exists).");

    rc = jobs_load("tests/jobs.txt", jobs, &jobs_count);
    if (rc == CC_OK)
        rc = schedule_compare(jobs, jobs_count, 2);
    results[count++] = (DemoResult){"Scheduling comparison", rc};

    results[count++] = (DemoResult){"Banker safe state", banker_run("tests/safe.txt")};
    results[count++] = (DemoResult){"Banker unsafe state", banker_run("tests/unsafe.txt")};
    results[count++] = (DemoResult){"Shared-memory IPC", ipc_demo()};
    results[count++] = (DemoResult){"Normal worker", worker_demo(0)};
    results[count++] = (DemoResult){"Simulated worker failure detection", worker_demo(1)};
    results[count++] = (DemoResult){"Resource limit", limit_demo()};

    /* All fork-based demonstrations have finished before threads begin. */
    Logger *logger = logger_start("integrated.log");
    if (!logger) {
        fputs("Demo: could not start integrated logger.\n", stderr);
        return CC_ERROR;
    }
    int log_ok = CC_OK;
    for (size_t i = 0; i < count; ++i)
        if (record(logger, &results[i]) != CC_OK)
            log_ok = CC_ERROR;
    if (logger_stop(logger) != CC_OK)
        log_ok = CC_ERROR;
    results[count++] = (DemoResult){"Integrated event logging", log_ok};

    size_t passed = 0;
    puts("\n=== Integrated summary ===");
    for (size_t i = 0; i < count; ++i) {
        printf("%-38s %s\n", results[i].name,
               results[i].status == CC_OK ? "PASS" : "FAIL");
        if (results[i].status == CC_OK) ++passed;
    }
    printf("Summary: %zu/%zu checks passed.\n", passed, count);
    puts("Detailed integration events: integrated.log");
    return passed == count ? CC_OK : CC_ERROR;
}
