
#include <sys/wait.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdbool.h>

#include "shm_b.h"
#include "shm_c1.h"

#define BUF_SIZE 10

static bool killProcesses = false;

void shmC1(void) {          // shared memory thread C1

    key_t shmKeyC1;
    int shmIdC1;
    struct MemoryC1 *shmPtrC1;

    printf("   shmC1 pid: %d\n", getpid());

    shmKeyC1 = ftok("shmKeyC1", 65);

    shmIdC1 = shmget(shmKeyC1, sizeof(struct MemoryC1), 0666);      // shmget returns an identifier in shmIdC1

    if (shmIdC1 == -1) {
        printf("   shmgetC1 error processC\n");
        exit(EXIT_FAILURE);
    }
    //printf("   shmC1 has received a shared memory %d\n", shmIdC1);

    shmPtrC1 = (struct MemoryC1 *) shmat(shmIdC1, NULL, 0);         // attache shared memory

    if ((long) shmPtrC1 == -1) {
        printf("   shmatC1 error processC\n");
        exit(EXIT_FAILURE);
    }
    //printf("   shmC1 has attached the shared memory %p\n", shmPtrC1);

    while (shmPtrC1->status != FILLED) { ;                          // wait until status is FILLED
    }
    //printf("   shmC1 found the data is ready...\n");

    printf("   value = %li\n", shmPtrC1->data);

    shmPtrC1->status = TAKEN;                                       // informed processB data have been taken
    //printf("   shmC1 has informed processB data have been taken...\n");

    shmdt((void *) shmPtrC1);                                       // detached shared memory
    printf("   shmC1 has detached its shared memory...\n");

    if (killProcesses) {
        //kill(getpid(), SIGUSR1);
        puts("   c1Pthread exit\n");
        pthread_exit(NULL);                 // terminate the calling thread
    }
}

void *c1Pthread(void *c1) {

    int c1pid = getpid();
    printf("   C1 pid: %d\n", c1pid);
    //printf("   C1 pthread id: %lu\n", pthread_self());

    shmC1();                                // go to shared memory thread C1

    return c1;
}

void wait_c2Pthread(void) {
    time_t start_time = time(NULL);
    time_t current;
    const int period = 1;                   // update every 1 sec

    do {
        current = time(NULL);
    } while (difftime(current, start_time) < period);
}

void *c2Pthread(void *c2) {

    printf("   C2 pid: %d\n", getpid());
    //printf("   C2 pthread id: %lu\n", pthread_self());

    if (killProcesses) {
        puts("   c2Pthread exit\n");
        pthread_exit(NULL);                 // terminate the calling thread
    }

    for (int i = 0; i < 3600; ++i) {        // c2Pthread work time = 1 hour
        puts("   doing well\n");

        wait_c2Pthread();                   // update output and load CPU
    }

    return c2;
}

void processC(void) {

    pid_t cPid = -1;

    int pid = getpid();

    if (cPid != pid) {
        cPid = fork();                              // create Process C
    }
    printf("  C pid: %d, pid: %d\n", pid, cPid);

    pthread_t thread1, thread2;

    if (cPid == -1) {
        perror("  processC fork failed\n");
        exit(EXIT_FAILURE);
    } else if (cPid > 0) {                          // parent processC
        printf("  parent processC pid: %d\n", getpid());
    } else if (cPid == 0) {                         // child processC
        printf("  child processC pid: %d\n", getpid());

        /* Create independent threads each of which will execute function */
        if (pthread_create(&thread1, NULL, c1Pthread, NULL)) {  // create a new thread - C1
            printf("  threadC1 error\n");
            exit(EXIT_FAILURE);
        }
        //printf("threadC1 ID; %lu\n", thread1);    // return identifier of current thread

        if (pthread_create(&thread2, NULL, c2Pthread, NULL)) {  // create a new thread - C2
            printf("  threadC2 error\n");
            exit(EXIT_FAILURE);
        }
        //printf("threadC2 ID; %lu\n", thread2);    // return identifier of current thread

        if (pthread_join(thread1, NULL)) {          // wait for termination of another thread
            printf("  error join threadC1\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_join(thread2, NULL)) {          // wait for termination of another thread
            printf("  error join threadC2\n");
            exit(EXIT_FAILURE);
        }

        if (killProcesses) {
            kill(cPid, SIGUSR1);
            exit(EXIT_SUCCESS);
        }
    }
}

void shmB(long i) {

    key_t shmKeyB;
    int shmIdB;
    struct MemoryB *shmPtrB;

    shmKeyB = ftok("shmKeyB", 65);                          // generate unique key

    shmIdB = shmget(shmKeyB, sizeof(struct MemoryB), 0666 | IPC_CREAT); // shmget returns an identifier in shmIdB

    if (shmIdB == -1) {
        printf(" shmgetB error processB\n");
        exit(EXIT_FAILURE);
    }
    //printf(" processB has received a shared memory %d\n", shmIdB);

    shmPtrB = (struct MemoryB *) shmat(shmIdB, NULL, 0);    // shmat to attach to shared memory.

    if ((long) shmPtrB == -1) {
        printf(" shmatB error processB\n");
        exit(EXIT_FAILURE);
    }
    //printf(" processB has attached the shared memory %p\n", shmPtrB);

    shmPtrB->status = NOT_READY;
    shmPtrB->data = i;
    //printf(" processB has filled %li in shared memory...\n", shmPtrB->data);
    shmPtrB->status = FILLED;

    printf(" shmB pid: %d\n", getpid());

    processC();                                             // start C process

    while (shmPtrB->status != TAKEN) { // wait until status becomes TAKEN, meaning that the c1Pthread has taken the data
        sleep(1);
    }

    //printf(" processB has detected the completion of processC1...\n");

    shmdt((void *) shmPtrB);                                // detached shared memory
    //printf(" processB has detached its shared memory...\n");

    shmctl(shmIdB, IPC_RMID, NULL);                         // remove shared memory ID
    printf(" processB has removed its shared memory...\n");

    if (killProcesses) {
        kill(getpid(), SIGUSR1);
        exit(EXIT_SUCCESS);
    }
}

void processB(long inputLong) {

    printf(" processB pid: %d\n", getpid());

    long input_pow = inputLong * inputLong;                 // squaring
    //printf(" 'B' squaring result: %li\n", input_pow);

    if (input_pow == 100) {
        killProcesses = true;
        //printf("!-kill all Processes-!\n");
    }

    shmB(input_pow);
}

void aProcess() {

    // fd[0];   //-> for using read end
    // fd[1];   //-> for using write end
    int fd[2];
    pid_t aPid = -1, pid;
    int status;
    long input = 0;
    char in[BUF_SIZE];

    while (input != 10) {
        printf("inputScanPid: %d\n", getpid());

        int s = scanf("%s", in);        // user input
        assert(s == 1);     // only one 'line' allowed

        input = strtol(in, NULL, 10);  // convert input string to 'long' number
        printf("input: %li\n", input);

        if (pipe(fd) == -1) {
            perror("pipe fd failed\n");
            exit(EXIT_FAILURE);
        }

        pid = getpid();

        if (aPid != pid) {
            aPid = fork();                   // create process A
        }
        printf("aPid: %d, pid: %d\n", aPid, pid);

        if (aPid == -1) {
            perror("aPid fork failed\n");
            exit(EXIT_FAILURE);
        } else if (aPid > 0) {              // Parent process; return process ID of the child process to the parent
            close(fd[0]);                   // Close reading end of pipe

            write(fd[1], &input, sizeof(input));  // Write input string

            close(fd[1]);                   // close writing end of pipe

            printf("Parent pipe pid: %d\n", getpid());

            pid = wait(&status);
            printf("aProcess detects process %d was done.\n", pid);

            if (killProcesses) {
                wait(NULL);                 // Wait for child to send a string

                printf("aProcess process done.\n");

                kill(getpid(), SIGUSR1);

                exit(EXIT_SUCCESS);
            }
        } else if (aPid == 0) {             // child process == process B, with ID == aPid
            close(fd[1]);                   // Close writing end of pipe

            long pipeRead = 0;
            read(fd[0], &pipeRead, BUF_SIZE);   // Read a string using pipe

            close(fd[0]);                   // Close reading end

            printf("child pipe pid: %d\n", getpid());

            processB(pipeRead);             // run process B

            if (killProcesses) {
                kill(getpid(), SIGUSR1);
                exit(EXIT_SUCCESS);
            }
        }

        if (killProcesses) {
            kill(getpid(), SIGUSR1);
        }
    }
}

int main(int argc, char *argv[]) {

    aProcess();

    printf("--- main(), close all ---\n");

    exit(EXIT_SUCCESS);
}
