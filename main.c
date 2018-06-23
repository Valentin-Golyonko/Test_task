
#include <sys/wait.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>

#include "shm_b.h"
#include "shm_c1.h"

#define BUF_SIZE 10

void shmC1() {

    key_t shmKeyC1;
    int shmIdC1;
    struct MemoryC1 *shmPtrC1;

    shmKeyC1 = ftok("shmKeyC1", 65);

    shmIdC1 = shmget(shmKeyC1, sizeof(struct MemoryC1), 0666);

    if (shmIdC1 == -1) {
        printf("   shmgetC1 error processC\n");
        exit(EXIT_FAILURE);
    } else {
        printf("   c1Process has received a shared memory %d\n", shmIdC1);
    }

    shmPtrC1 = (struct MemoryC1 *) shmat(shmIdC1, NULL, 0);

    if ((long) shmPtrC1 == -1) {
        printf("   shmatC1 error processC\n");
        exit(EXIT_FAILURE);
    } else {
        printf("   c1Process has attached the shared memory %p\n", shmPtrC1);
    }

    while (shmPtrC1->status != FILLED) { ;
    }
    printf("   c1Process found the data is ready...\n");

    printf("   value = %li\n", shmPtrC1->data);

    printf("   shmC1 pid: %d\n", getpid());

    shmPtrC1->status = TAKEN;
    printf("   c1Process has informed processB data have been taken...\n");

    shmdt((void *) shmPtrC1);
    printf("   c1Process has detached its shared memory...\n");

    //exit(EXIT_SUCCESS);
}

void c2Process() {

    printf("    C2 doing well\n");
    printf("    C2 pid: %d\n", getpid());
}

void processC() {

    /*pid_t cPid;
    cPid = fork();

    if (cPid == -1) {
        perror("  processC fork failed\n");
        exit(EXIT_FAILURE);
    } else if (cPid == 0) {
        printf("  child processC pid: %d\n", getpid());
    }
    printf("  parent processC pid: %d\n", getpid());*/

    shmC1();

    c2Process();
}

void shmB(long i) {

    key_t shmKeyB;
    int shmIdB;
    struct MemoryB *shmPtrB;

    shmKeyB = ftok("shmKeyB", 65); // generate unique key

    shmIdB = shmget(shmKeyB, sizeof(struct MemoryB), 0666 | IPC_CREAT); // shmget returns an identifier in shmIdB
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

    printf(" shmB pid: %d\n", getpid());

    processC();                            // start C process

    while (shmPtrB->status != TAKEN) {
        sleep(1);
    }
    printf(" processB has detected the completion of processC...\n");

    shmdt((void *) shmPtrB);
    printf(" processB has detached its shared memory...\n");

    shmctl(shmIdB, IPC_RMID, NULL);
    printf(" processB has removed its shared memory...\n");

    //exit(EXIT_SUCCESS);
}

void processB(long inputLong) {

    printf(" processB pid: %d\n", getpid());

    long input_pow = inputLong * inputLong; // squaring
    printf(" 'B' squaring result: %li\n", input_pow);

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
    } else if (aPid == 0) {             // child process
        close(fd[1]);                   // Close writing end of pipe

        long pipeRead = 0;
        read(fd[0], &pipeRead, BUF_SIZE);     // Read a string using pipe
        //printf("pipeRead: %li\n", pipeRead);

        close(fd[0]);                   // Close reading end

        printf("child pipe pid: %d\n", getpid());

        processB(pipeRead);                   // run process B

        //exit(EXIT_SUCCESS);
    } else {                            // Parent process
        close(fd[0]);                   // Close reading end of pipe

        write(fd[1], &in, sizeof(in));  // Write input string

        close(fd[1]);                   // close writing end of pipe

        printf("Parent pipe pid: %d\n", getpid());

        pid = wait(&status);
        printf("aProcess detects process %d was done.\n", pid);
        wait(NULL);                     // Wait for child to send a string

        printf("aProcess process done.\n");
        exit(EXIT_SUCCESS);
    }
}

void inputScan() {
    long a = 0;
    char in[BUF_SIZE];

    while (a != 10) {
        printf("inPid: %d\n", getpid());

        int s = scanf("%s", in);      // user input
        assert(s == 1);                 // only one 'line' allowed

        long input_convert = strtol(in, NULL, 10);  // convert input string to 'long' number
        printf("input_convert: %li, len: %zu\n", input_convert, sizeof(input_convert));

        a = input_convert;

        aProcess(a);                    // start PIPE method
    }
}

int main(int argc, char *argv[]) {

    inputScan();

    printf("--- main(), close all ---\n");

    exit(EXIT_SUCCESS);
}


