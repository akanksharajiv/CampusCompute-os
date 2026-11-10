#include "campuscompute.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_for_child(pid_t pid, int *status)
{
    pid_t waited;
    do {
        waited = waitpid(pid, status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        perror("waitpid");
        return CC_ERROR;
    }
    return CC_OK;
}

int worker_demo(int simulate_failure)
{
    if (simulate_failure != 0 && simulate_failure != 1) {
        fprintf(stderr, "Worker: invalid simulation mode.\n");
        return CC_USAGE;
    }

    const int expected = simulate_failure ? 42 : 0;

    printf("WORKER: starting %s worker demonstration.\n",
           simulate_failure ? "simulated-failure" : "normal");
    fflush(stdout);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Worker fork");
        return CC_ERROR;
    }

    if (pid == 0) {
        printf("WORKER CHILD [%ld]: running.\n", (long)getpid());

        if (simulate_failure) {
            puts("WORKER CHILD: simulating a failure (exit code 42).");
        } else {
            puts("WORKER CHILD: task completed successfully.");
        }

        fflush(stdout);
        _exit(expected);
    }

    int status;
    if (wait_for_child(pid, &status) != CC_OK) {
        return CC_ERROR;
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr,
                "WORKER PARENT: child did not exit normally.\n");
        return CC_ERROR;
    }

    int actual = WEXITSTATUS(status);

    printf("WORKER PARENT: waitpid reaped child %ld; exit code = %d.\n",
           (long)pid, actual);

    if (actual != expected) {
        fprintf(stderr, "WORKER: unexpected child exit status.\n");
        return CC_ERROR;
    }

    if (simulate_failure) {
        puts("WORKER PARENT: simulated failure detected correctly.");
    } else {
        puts("WORKER PARENT: successful completion confirmed.");
    }

    puts("WORKER: PASS (fork, waitpid, and exit-status verification).");
    return CC_OK;
}

int limit_demo(void)
{
    puts("LIMIT: starting resource-limit demonstration in a child process.");
    fflush(stdout);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Limit fork");
        return CC_ERROR;
    }

    if (pid == 0) {
        struct rlimit lim;

        if (getrlimit(RLIMIT_NOFILE, &lim) == -1) {
            perror("LIMIT child getrlimit");
            _exit(CC_ERROR);
        }

        rlim_t target = 16;

        if (lim.rlim_cur != RLIM_INFINITY &&
            lim.rlim_cur < target) {
            target = lim.rlim_cur;
        }

        if (target < 6) {
            fprintf(stderr,
                    "LIMIT child: inherited descriptor limit is too low.\n");
            _exit(CC_ERROR);
        }

        lim.rlim_cur = target;

        if (setrlimit(RLIMIT_NOFILE, &lim) == -1) {
            perror("LIMIT child setrlimit");
            _exit(CC_ERROR);
        }

        printf("LIMIT CHILD [%ld]: RLIMIT_NOFILE soft limit set to %lu.\n",
               (long)getpid(), (unsigned long)target);
        fflush(stdout);

        int handles[32];
        int used = 0;
        int hit_limit = 0;

        while (used < (int)(sizeof handles / sizeof handles[0])) {
            int fd = open("/dev/null", O_RDONLY);

            if (fd == -1) {
                if (errno == EMFILE) {
                    hit_limit = 1;
                    printf("LIMIT CHILD: open() returned EMFILE after %d extra descriptors.\n",
                           used);
                } else {
                    perror("LIMIT child open");
                }
                break;
            }

            handles[used++] = fd;
        }

        for (int i = 0; i < used; ++i) {
            close(handles[i]);
        }

        fflush(stdout);
        _exit(hit_limit ? CC_OK : CC_ERROR);
    }

    int status;
    if (wait_for_child(pid, &status) != CC_OK) {
        return CC_ERROR;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != CC_OK) {
        fprintf(stderr,
                "LIMIT PARENT: child failed to demonstrate the expected limit.\n");
        return CC_ERROR;
    }

    puts("LIMIT PARENT: resource limit was enforced; child exited and was reaped.");
    puts("LIMIT PARENT: parent process file-descriptor limit was not changed.");
    puts("LIMIT: PASS (setrlimit and observed EMFILE).");

    return CC_OK;
}
