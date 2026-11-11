#include "campuscompute.h"
#include <stdio.h>
#include <string.h>

#define MAX_TIMELINE_SLICES 100000LL

static int earlier(const Job *a, const Job *b)
{
    return a->arrival < b->arrival ||
           (a->arrival == b->arrival && a->id < b->id);
}

static void sort_arrivals(Job *jobs, size_t n)
{
    /* Insertion sort: simple and sufficient for N <= 100. */
    for (size_t i = 1; i < n; ++i) {
        Job current = jobs[i];
        size_t j = i;
        while (j > 0 && earlier(&current, &jobs[j - 1])) {
            jobs[j] = jobs[j - 1]; --j;
        }
        jobs[j] = current;
    }
}

static int simulate(SchedulePolicy policy, const Job *input, size_t n,
                    long long quantum, long double *avg_tat,
                    long double *avg_wt)
{
    if (n == 0 || n > CC_MAX_JOBS ||
        (policy != POLICY_FCFS && policy != POLICY_RR && policy != POLICY_PRIORITY) ||
        (policy == POLICY_RR && quantum <= 0)) return CC_USAGE;
    Job jobs[CC_MAX_JOBS];
    memcpy(jobs, input, n * sizeof *jobs); /* Fresh state for every policy. */
    for (size_t i = 0; i < n; ++i) {
        jobs[i].remaining = jobs[i].burst;
        jobs[i].status = JOB_NEW;
        jobs[i].completion = jobs[i].turnaround = jobs[i].waiting = 0;
    }
    sort_arrivals(jobs, n);
    if (policy == POLICY_RR) {
        long long slices = 0;
        for (size_t i = 0; i < n; ++i) {
            long long needed = jobs[i].burst / quantum +
                               (jobs[i].burst % quantum != 0);
            if (needed > MAX_TIMELINE_SLICES - slices) {
                fputs("Error: RR timeline would exceed 100000 slices; "
                      "use smaller bursts or a larger quantum.\n", stderr);
                return CC_USAGE;
            }
            slices += needed;
        }
    }
    const char *name = policy == POLICY_FCFS ? "FCFS" :
                       policy == POLICY_RR ? "Round Robin" : "Priority (non-preemptive)";
    printf("\n=== %s ===\n", name);
    puts("Tie rule: arrival time, then job ID. Smaller priority number = higher priority.");
    if (policy == POLICY_RR)
        printf("Quantum = %lld; new arrivals through slice end precede requeue.\n", quantum);
    puts("Lifecycle: NEW -> READY on arrival; dispatch READY -> RUNNING.");
    puts("Timeline (time units; [start,end) intervals):");
    long long time = 0;
    size_t done = 0, next = 0;
    size_t queue[CC_MAX_JOBS], head = 0, used = 0;
    while (done < n) {
        while (next < n && jobs[next].arrival <= time) {
            jobs[next].status = JOB_READY;
            if (policy == POLICY_RR) {
                queue[(head + used) % CC_MAX_JOBS] = next;
                ++used;
            }
            ++next;
        }
        size_t chosen = n;
        if (policy == POLICY_RR && used > 0) {
            chosen = queue[head]; head = (head + 1) % CC_MAX_JOBS; --used;
        } else if (policy != POLICY_RR) {
            for (size_t i = 0; i < n; ++i) {
                if (jobs[i].status != JOB_READY) continue;
                if (chosen == n ||
                    (policy == POLICY_PRIORITY && jobs[i].priority < jobs[chosen].priority) ||
                    ((policy == POLICY_FCFS || jobs[i].priority == jobs[chosen].priority) &&
                     earlier(&jobs[i], &jobs[chosen]))) chosen = i;
            }
        }
        if (chosen == n) {
            printf("[%lld,%lld) IDLE\n", time, jobs[next].arrival);
            time = jobs[next].arrival;
            continue;
        }
        Job *job = &jobs[chosen];
        job->status = JOB_RUNNING;
        long long duration = job->remaining;
        if (policy == POLICY_RR && duration > quantum) duration = quantum;
        printf("[%lld,%lld) J%d READY -> RUNNING\n", time, time + duration, job->id);
        time += duration;
        job->remaining -= duration;
        /* Boundary arrivals precede the running job's RR requeue. */
        while (next < n && jobs[next].arrival <= time) {
            jobs[next].status = JOB_READY;
            if (policy == POLICY_RR) {
                queue[(head + used) % CC_MAX_JOBS] = next;
                ++used;
            }
            ++next;
        }
        if (job->remaining == 0) {
            job->status = JOB_COMPLETED;
            job->completion = time;
            job->turnaround = time - job->arrival;
            job->waiting = job->turnaround - job->burst;
            ++done;
            printf("  t=%lld J%d RUNNING -> COMPLETED\n", time, job->id);
        } else {
            job->status = JOB_READY;
            queue[(head + used) % CC_MAX_JOBS] = chosen; ++used;
            printf("  t=%lld J%d RUNNING -> READY (remaining=%lld)\n",
                   time, job->id, job->remaining);
        }
    }
    long double total_tat = 0, total_wt = 0;
    puts("ID ARR BUR PRI CT TAT WT STATUS (rows ordered by arrival, then ID)");
    for (size_t i = 0; i < n; ++i) {
        Job *j = &jobs[i];
        printf("%d %lld %lld %d %lld %lld %lld COMPLETED\n", j->id,
               j->arrival, j->burst, j->priority, j->completion,
               j->turnaround, j->waiting);
        total_tat += j->turnaround; total_wt += j->waiting;
    }
    *avg_tat = total_tat / n; *avg_wt = total_wt / n;
    printf("Average TAT = %.2Lf; Average WT = %.2Lf\n", *avg_tat, *avg_wt);
    return CC_OK;
}

int schedule_run(SchedulePolicy p, const Job *jobs, size_t n, long long q)
{
    long double tat, wt;
    return simulate(p, jobs, n, q, &tat, &wt);
}

int schedule_compare(const Job *jobs, size_t n, long long q)
{
    long double tat[3], wt[3];
    SchedulePolicy policies[] = {POLICY_FCFS, POLICY_RR, POLICY_PRIORITY};
    const char *names[] = {"FCFS", "Round Robin", "Priority"};
    for (size_t i = 0; i < 3; ++i) {
        int rc = simulate(policies[i], jobs, n, q, &tat[i], &wt[i]);
        if (rc != CC_OK) return rc;
    }
    printf("\nComparison: same original job set; RR quantum=%lld\n", q);
    puts("Policy         Average TAT  Average WT");
    for (size_t i = 0; i < 3; ++i)
        printf("%-14s %11.2Lf %11.2Lf\n", names[i], tat[i], wt[i]);
    return CC_OK;
}
