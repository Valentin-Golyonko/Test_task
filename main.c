
#include <sys/wait.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <pthread.h>

#include "shm_b.h"
#include "shm_c1.h"

#define BUF_SIZE 10
static int killProcesses = 0;

void shmC1(void) {

    key_t shmKeyC1;
    int shmIdC1;
    struct MemoryC1 *shmPtrC1;

    int shmC1pid = getpid();
    printf("   shmC1 pid: %d\n", shmC1pid);

    shmKeyC1 = ftok("shmKeyC1", 65);

    shmIdC1 = shmget(shmKeyC1, sizeof(struct MemoryC1), 0666);

    if (shmIdC1 == -1) {
        printf("   shmgetC1 error processC\n");
        exit(EXIT_FAILURE);
    } else {
        printf("   shmC1 has received a shared memory %d\n", shmIdC1);
    }

    shmPtrC1 = (struct MemoryC1 *) shmat(shmIdC1, NULL, 0);

    if ((long) shmPtrC1 == -1) {
        printf("   shmatC1 error processC\n");
        exit(EXIT_FAILURE);
    } else {
        printf("   shmC1 has attached the shared memory %p\n", shmPtrC1);
    }

    while (shmPtrC1->status != FILLED) { ;
    }
    printf("   shmC1 found the data is ready...\n");

    printf("   value = %li\n", shmPtrC1->data);

    shmPtrC1->status = TAKEN;
    printf("   shmC1 has informed processB data have been taken...\n");

    if (killProcesses == 1) {
        shmdt((void *) shmPtrC1);
        printf("   shmC1 has detached its shared memory...\n");

        if (kill(shmC1pid, SIGUSR1) == 0) {
            printf("shmC1pid %d off\n", shmC1pid);
        }
        exit(EXIT_SUCCESS);
    }
}

void *c1Pthread(void *c1) {

    int c1pid = getpid();
    printf("   C1 pid: %d\n", c1pid);
    printf("   C1 pthread id: %lu\n", pthread_self());

    shmC1();

    return NULL;
}

void wait_c2_thread(void) {
    time_t start_time = time(NULL);
    time_t current;
    const int period = 1;

    do {
        current = time(NULL);
    } while (difftime(current, start_time) < period);
}

void *c2Pthread(void *c2) {

    int c2pid = getpid();
    printf("   C2 pid: %d\n", c2pid);
    printf("   C2 pthread id: %lu\n", pthread_self());

    for (int i = 0; i < 3600; ++i) {        // c2_thread work time = 1 hour
        puts("   doing well\n");

        wait_c2_thread();
    }

    return NULL;
}

void processC(void) {

    pid_t cPid;
    cPid = fork();

    int cpid = getpid();
    printf("  C pid: %d\n", cpid);

    pthread_t thread1, thread2;
    //int iret1, iret2;

    if (cPid == -1) {
        perror("  processC fork failed\n");
        exit(EXIT_FAILURE);
    } else if (cPid > 0) {
        printf("  parent processC pid: %d\n", getpid());
    } else if (cPid == 0) {
        printf("  child processC pid: %d\n", getpid());

        if (pthread_create(&thread1, NULL, c1Pthread, NULL)) {
            printf("  threadC1 error\n");
            exit(EXIT_FAILURE);
        }
        if (pthread_create(&thread2, NULL, c2Pthread, NULL)) {
            printf("  threadC2 error\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_join(thread1, NULL)) {
            printf("  error join threadC1\n");
            exit(EXIT_FAILURE);
        }
        if (pthread_join(thread2, NULL)) {
            printf("  error join threadC2\n");
            exit(EXIT_FAILURE);
        }


        if (killProcesses == 1) {

            if (kill(cPid, SIGUSR1) == 0) {
                printf("cPid %d off\n", cPid);
            }
            exit(EXIT_SUCCESS);
        }
    }
}

void shmB(long i) {

    key_t shmKeyB;
    int shmIdB;
    struct MemoryB *shmPtrB;

    shmKeyB = ftok("shmKeyB", 65); // generate unique key

    // shmget returns an identifier in shmIdB
    shmIdB = shmget(shmKeyB, sizeof(struct MemoryB), 0666 | IPC_CREAT);
    if (shmIdB == -1) {
        printf(" shmget error processB\n");
        exit(EXIT_FAILURE);
    } else {
        printf(" processB has received a shared memory %d\n", shmIdB);
    }

    shmPtrB = (struct MemoryB *) shmat(shmIdB, NULL, 0); // shmat to attach to shared memory.
    if ((long) shmPtrB == -1) {
        printf(" shmat error processB\n");
        exit(EXIT_FAILURE);
    } else {
        printf(" processB has attached the shared memory %p\n", shmPtrB);
    }

    shmPtrB->status = NOT_READY;
    shmPtrB->data = i;
    printf(" processB has filled %li in shared memory...\n", shmPtrB->data);
    shmPtrB->status = FILLED;

    int shmBpid = getpid();
    printf(" shmB pid: %d\n", shmBpid);

    processC();                            // start C process

    while (shmPtrB->status != TAKEN) {
        sleep(1);
    }

    if (killProcesses == 1) {
        //printf(" processB has detected the completion of processC...\n");

        shmdt((void *) shmPtrB);
        printf(" processB has detached its shared memory...\n");

        shmctl(shmIdB, IPC_RMID, NULL);
        printf(" processB has removed its shared memory...\n");

        if (kill(shmBpid, SIGUSR1) == 0) {
            printf("shmBpid %d off\n", shmBpid);
        }
        exit(EXIT_SUCCESS);
    }
}

void processB(long inputLong) {

    int bpid = getpid();
    printf(" processB pid: %d\n", bpid);

    long input_pow = inputLong * inputLong; // squaring
    printf(" 'B' squaring result: %li\n", input_pow);

    if (input_pow == 100) {
        killProcesses = 1;
        printf("!kill all Processes!\n");
    }

    shmB(input_pow);
}

void aProcess(long in) {

    // fd[0];   //-> for using read end
    // fd[1];   //-> for using write end
    int fd[2];
    pid_t aPid, pid;
    int status;

    if (pipe(fd) == -1) {
        perror("pipe fd failed\n");
        exit(EXIT_FAILURE);
    }

    aPid = fork();

    if (aPid == -1) {
        perror("aPid fork failed\n");
        exit(EXIT_FAILURE);
    } else if (aPid > 0) {              // Parent process
        close(fd[0]);                   // Close reading end of pipe

        write(fd[1], &in, sizeof(in));  // Write input string

        close(fd[1]);                   // close writing end of pipe

        int ppipe_pid = getpid();
        printf("Parent pipe pid: %d\n", ppipe_pid);

        pid = wait(&status);
        printf("aProcess detects process %d was done.\n", pid);

        if (killProcesses == 1) {
            wait(NULL);                     // Wait for child to send a string

            printf("aProcess process done.\n");

            if (kill(ppipe_pid, SIGUSR1) == 0) {
                printf("ppipe_pid %d off\n", ppipe_pid);
            }
            exit(EXIT_SUCCESS);
        }
    } else if (aPid == 0) {             // child process
        close(fd[1]);                   // Close writing end of pipe

        long pipeRead = 0;
        read(fd[0], &pipeRead, BUF_SIZE);   // Read a string using pipe

        close(fd[0]);                   // Close reading end

        int cpipe_pid = getpid();
        printf("child pipe pid: %d\n", cpipe_pid);

        processB(pipeRead);             // run process B

        if (killProcesses == 1) {
            if (kill(cpipe_pid, SIGUSR1) == 0) {
                printf("cpipe_pid %d off\n", cpipe_pid);
            }
            exit(EXIT_SUCCESS);
        }
    }
}

void inputScan(void) {
    long a = 0;
    char in[BUF_SIZE];

    while (a != 10) {
        int inputScanPid = getpid();
        printf("inputScanPid: %d\n", inputScanPid);

        int s = scanf("%s", in);      // user input
        assert(s == 1);                 // only one 'line' allowed

        long input_convert = strtol(in, NULL, 10);  // convert input string to 'long' number
        printf("input_convert: %li, len: %zu\n", input_convert, sizeof(input_convert));

        a = input_convert;

        aProcess(a);                    // start PIPE method

        if (killProcesses == 1) {
            if (kill(inputScanPid, SIGUSR1) == 0) {
                printf("inputScanPid %d off\n", inputScanPid);
            }
        }
    }
}

int main(int argc, char *argv[]) {

    pid_t scanPid;
    scanPid = fork();
    printf("%d\n", getpid());

    if (scanPid == -1) {
        perror("scanPid fork failed\n");
        exit(EXIT_FAILURE);
    } else if (scanPid > 0) {
        inputScan();
    }

    printf("--- main(), close all ---\n");

    exit(EXIT_SUCCESS);
}
