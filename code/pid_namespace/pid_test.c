#define _GNU_SOURCE
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)
#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */

static int grandFunc(void *arg)
{
    printf("grand child func pid:  %d\n", getpid());

    /* Keep the namespace open for a while, by sleeping.
       This allows some experimentation--for example, another
       process might join the namespace. */

    sleep(3);
    return 0;           /* Child terminates now */
}


static int childFunc(void *arg)
{
    printf("child func pid:  %d\n", getpid());

    char *stack;                    /* Start of stack buffer */
    char *stackTop;                 /* End of stack buffer */
    pid_t pid;

    /* Allocate stack for child */
    stack = malloc(STACK_SIZE);
    if (stack == NULL)
        errExit("malloc");
    stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */
    /* Create child that has its own UTS namespace;
       child commences execution in childFunc() */
    pid = clone(grandFunc, stackTop, CLONE_NEWPID, NULL);
    if (pid == -1)
        errExit("clone");

    /* Keep the namespace open for a while, by sleeping.
       This allows some experimentation--for example, another
       process might join the namespace. */
    printf("child func grand child pid %ld\n", (long) pid);

    sleep(2);

    wait(NULL);
    sleep(3);
    return 0;           /* Child terminates now */
}


int main(void)
{
    char *stack;                    /* Start of stack buffer */
    char *stackTop;                 /* End of stack buffer */
    pid_t pid;

    /* Allocate stack for child */
    stack = malloc(STACK_SIZE);
    if (stack == NULL)
        errExit("malloc");
    stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */
    /* Create child that has its own UTS namespace;
       child commences execution in childFunc() */
    pid = clone(childFunc, stackTop, CLONE_NEWPID, NULL);
    if (pid == -1)
        errExit("clone");
    printf("father func child pid %ld\n", (long) pid);

    /* Parent falls through to here */
    sleep(2);           /* Give child time to change its hostname */
    /* Display hostname in parent's UTS namespace. This will be
       different from hostname in child's UTS namespace. */

    if (waitpid(pid, NULL, 0) == -1)    /* Wait for child */
        errExit("waitpid");

    printf("child has terminated\n");
    exit(EXIT_SUCCESS);

}
