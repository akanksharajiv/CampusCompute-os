#include "campuscompute.h"
#include "input_reader.h"
#include <limits.h>
int jobs_load(const char *path, Job jobs[CC_MAX_JOBS], size_t *count)
{
    *count = 0; /* Never expose partial input as a valid set. */
    FILE *file = fopen(path, "r");
    if (!file) { perror(path); return CC_ERROR; }
    long long n, total_burst = 0, last_arrival = 0;
    if (input_integer(file, &n) != 1 || n < 1 || n > CC_MAX_JOBS)
        goto invalid;
    for (size_t i = 0; i < (size_t)n; ++i) {
        long long id, arrival, burst, priority;
        if (input_integer(file, &id) != 1 ||
            input_integer(file, &arrival) != 1 ||
            input_integer(file, &burst) != 1 ||
            input_integer(file, &priority) != 1)
            goto invalid;
        if (id < 1 || id > INT_MAX || arrival < 0 || burst < 1 ||
            priority < 0 || priority > INT_MAX)
            goto invalid;
        for (size_t j = 0; j < i; ++j)
            if (jobs[j].id == id) goto invalid;
        if (burst > LLONG_MAX - total_burst) goto invalid;
        total_burst += burst;
        if (arrival > last_arrival) last_arrival = arrival;
        jobs[i] = (Job){.id = (int)id, .arrival = arrival, .burst = burst,
                       .priority = (int)priority, .status = JOB_NEW,
                       .remaining = burst};
    }
    /* Conservative upper bound on the last completion time. */
    if (total_burst > LLONG_MAX - last_arrival || !input_finished(file))
        goto invalid;
    if (fclose(file) != 0) { perror(path); return CC_ERROR; }
    *count = (size_t)n;
    return CC_OK;
invalid:
    fprintf(stderr, "Error: invalid jobs file '%s'. Expected N (1..%d), then "
            "N rows: positive unique ID, arrival>=0, burst>0, priority>=0. "
            "No missing/extra tokens or time overflow.\n", path, CC_MAX_JOBS);
    fclose(file);
    return CC_USAGE;
}
