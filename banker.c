#include "campuscompute.h"
#include "input_reader.h"
#include <limits.h>
int banker_run(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file) { perror(path); return CC_ERROR; }
    long long n_raw, m_raw;
    long long available[CC_MAX_RESOURCES], work[CC_MAX_RESOURCES];
    long long allocation[CC_MAX_PROCESSES][CC_MAX_RESOURCES];
    long long maximum[CC_MAX_PROCESSES][CC_MAX_RESOURCES];
    long long need[CC_MAX_PROCESSES][CC_MAX_RESOURCES];
    if (input_integer(file, &n_raw) != 1 || input_integer(file, &m_raw) != 1 ||
        n_raw < 1 || n_raw > CC_MAX_PROCESSES ||
        m_raw < 1 || m_raw > CC_MAX_RESOURCES) goto invalid;
    size_t n = (size_t)n_raw, m = (size_t)m_raw;
    for (size_t r = 0; r < m; ++r) {
        if (input_integer(file, &available[r]) != 1 || available[r] < 0)
            goto invalid;
        work[r] = available[r];
    }
    for (size_t i = 0; i < n; ++i)
        for (size_t r = 0; r < m; ++r)
            if (input_integer(file, &allocation[i][r]) != 1 ||
                allocation[i][r] < 0) goto invalid;
    for (size_t i = 0; i < n; ++i)
        for (size_t r = 0; r < m; ++r) {
            if (input_integer(file, &maximum[i][r]) != 1 ||
                maximum[i][r] < allocation[i][r]) goto invalid;
            need[i][r] = maximum[i][r] - allocation[i][r];
        }
    if (!input_finished(file)) goto invalid;
    /* Bound Work before any simulated release can overflow. */
    for (size_t r = 0; r < m; ++r) {
        long long total = available[r];
        for (size_t i = 0; i < n; ++i) {
            if (allocation[i][r] > LLONG_MAX - total) goto invalid;
            total += allocation[i][r];
        }
    }
    if (fclose(file) != 0) { perror(path); return CC_ERROR; }
    printf("Banker safety check: %zu processes, %zu resource types\nAvailable:", n, m);
    for (size_t r = 0; r < m; ++r) printf(" %lld", available[r]);
    puts("\nProcess | Allocation | Maximum | Need");
    for (size_t i = 0; i < n; ++i) {
        printf("P%zu |", i + 1);
        for (size_t r = 0; r < m; ++r) printf(" %lld", allocation[i][r]);
        printf(" | ");
        for (size_t r = 0; r < m; ++r) printf(" %lld", maximum[i][r]);
        printf(" | ");
        for (size_t r = 0; r < m; ++r) printf(" %lld", need[i][r]);
        putchar('\n');
    }
    int finished[CC_MAX_PROCESSES] = {0};
    size_t sequence[CC_MAX_PROCESSES], count = 0;
    while (count < n) {
        int progress = 0;
        for (size_t i = 0; i < n; ++i) {
            if (finished[i]) continue;
            int runnable = 1;
            for (size_t r = 0; r < m; ++r)
                if (need[i][r] > work[r]) runnable = 0;
            if (!runnable) continue;
            /* Hypothetical completion returns its existing allocation. */
            for (size_t r = 0; r < m; ++r) work[r] += allocation[i][r];
            finished[i] = 1; sequence[count++] = i; progress = 1;
            printf("Assume P%zu completes; Work becomes:", i + 1);
            for (size_t r = 0; r < m; ++r) printf(" %lld", work[r]);
            putchar('\n');
        }
        if (!progress) break;
    }
    if (count == n) {
        printf("SAFE sequence:");
        for (size_t i = 0; i < n; ++i)
            printf("%sP%zu", i ? " -> " : " ", sequence[i] + 1);
        putchar('\n');
    } else {
        puts("UNSAFE: no safe sequence exists for the declared maximum claims.");
        printf("Unfinished:");
        for (size_t i = 0; i < n; ++i) if (!finished[i]) printf(" P%zu", i + 1);
        puts("\nUnsafe is not proof that a deadlock is already occurring.");
    }
    return CC_OK;
invalid:
    fprintf(stderr, "Error: invalid Banker file '%s'. Expected N M, Available, "
            "Allocation, Maximum; nonnegative integers, Maximum>=Allocation, "
            "valid dimensions, no overflow or missing/extra tokens.\n", path);
    fclose(file);
    return CC_USAGE;
}
