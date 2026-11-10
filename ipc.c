#include "campuscompute.h"

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    char message[128];
    char reply[128];
    int counter;
} SharedData;

int ipc_demo(void)
{
    char shm_name[64], sem_name[64];
    int fd = -1, shm_created = 0, sem_created = 0;
    int result = CC_ERROR;
    SharedData *data = MAP_FAILED;
    sem_t *sem = SEM_FAILED;
    pid_t child = -1;
    int child_status = 0;

    if (snprintf(shm_name, sizeof shm_name,
                 "/campuscompute_shm_%ld",
                 (long)getpid()) >= (int)sizeof shm_name ||
        snprintf(sem_name, sizeof sem_name,
                 "/campuscompute_sem_%ld",
                 (long)getpid()) >= (int)sizeof sem_name) {
        fprintf(stderr, "IPC: resource name too long.\n");
        return CC_ERROR;
    }

    fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        perror("IPC shm_open");
        goto cleanup;
    }
    shm_created = 1;

    if (ftruncate(fd, (off_t)sizeof(SharedData)) == -1) {
        perror("IPC ftruncate");
        goto cleanup;
    }

    data = mmap(NULL, sizeof *data, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("IPC mmap");
        goto cleanup;
    }
    memset(data, 0, sizeof *data);

    sem = sem_open(sem_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem == SEM_FAILED) {
        perror("IPC sem_open");
        goto cleanup;
    }
    sem_created = 1;

    printf("IPC: shared memory created: %s\n", shm_name);
    printf("IPC: named semaphore created: %s (initial value 0)\n",
           sem_name);
    fflush(stdout);

    child = fork();
    if (child == -1) {
        perror("IPC fork");
        goto cleanup;
    }

    if (child == 0) {
        int waited;
        do {
            waited = sem_wait(sem);
        } while (waited == -1 && errno == EINTR);

        if (waited == -1) {
            perror("IPC child sem_wait");
            _exit(CC_ERROR);
        }

        printf("IPC CHILD [%ld]: received '%s' via shared memory.\n",
               (long)getpid(), data->message);

        data->counter += 1;
        snprintf(data->reply, sizeof data->reply,
                 "Acknowledged by child PID %ld", (long)getpid());

        printf("IPC CHILD: sent acknowledgement; counter updated.\n");
        fflush(stdout);

        sem_close(sem);
        munmap(data, sizeof *data);
        close(fd);
        _exit(CC_OK);
    }

    snprintf(data->message, sizeof data->message,
             "Hello from parent PID %ld", (long)getpid());
    data->counter = 41;

    printf("IPC PARENT [%ld]: wrote '%s' into shared memory.\n",
           (long)getpid(), data->message);

    if (sem_post(sem) == -1) {
        perror("IPC parent sem_post");
        goto cleanup;
    }

    printf("IPC PARENT: semaphore posted to release child.\n");
    fflush(stdout);

    pid_t waited;
    do {
        waited = waitpid(child, &child_status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        perror("IPC waitpid");
        goto cleanup;
    }
    child = -1;

    if (!WIFEXITED(child_status) ||
        WEXITSTATUS(child_status) != CC_OK) {
        fprintf(stderr, "IPC: child did not exit successfully.\n");
        goto cleanup;
    }

    printf("IPC PARENT: received reply '%s'.\n", data->reply);
    printf("IPC PARENT: counter = %d (expected 42).\n",
           data->counter);

    if (data->counter != 42 ||
        strncmp(data->reply, "Acknowledged by child PID ", 26) != 0) {
        fprintf(stderr, "IPC: shared-memory exchange verification failed.\n");
        goto cleanup;
    }

    printf("IPC: child exited successfully and was reaped using waitpid.\n");
    result = CC_OK;

cleanup:
    if (child > 0) {
        kill(child, SIGTERM);
        while (waitpid(child, &child_status, 0) == -1 &&
               errno == EINTR) { }
    }

    if (sem != SEM_FAILED && sem_close(sem) == -1) {
        perror("IPC sem_close");
        result = CC_ERROR;
    }

    if (sem_created) {
        if (sem_unlink(sem_name) == -1) {
            perror("IPC sem_unlink");
            result = CC_ERROR;
        } else {
            puts("IPC cleanup: named semaphore unlinked.");
        }
    }

    if (data != MAP_FAILED && munmap(data, sizeof *data) == -1) {
        perror("IPC munmap");
        result = CC_ERROR;
    }

    if (fd != -1 && close(fd) == -1) {
        perror("IPC close");
        result = CC_ERROR;
    }

    if (shm_created) {
        if (shm_unlink(shm_name) == -1) {
            perror("IPC shm_unlink");
            result = CC_ERROR;
        } else {
            puts("IPC cleanup: shared memory unlinked.");
        }
    }

    if (result == CC_OK) {
        puts("IPC: PASS (transfer, synchronization, and cleanup).");
    }

    return result;
}
