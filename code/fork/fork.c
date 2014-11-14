#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>

int main(void)
{
    pid_t pid;

    pid = fork();
    if(pid < 0)
        perror("fork");
    else if(pid == 0) {
        printf("child pid is %d\n", getpid());

        _exit(0);
    }

    waitpid(pid, NULL, 0);
    return 0;
}
