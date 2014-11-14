#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static int count = 0;
int main(void) 
{
    int status,i;

    for (i = 0; i < 1000000; i++)
    {
        count++;
        printf("count is %d\n", count);

        status = fork();

        if (status == 0 || status == -1) break;//每次循环时，如果发现是子进程就直接从创建子进程的循环中跳出来，
         //不让你进入循环，这样就保证了每次只有父进程来做循环创建子进程的工作
    }

    if (status == -1)
    {
        printf("error!\n");
    }
    else if (status == 0) //每个子进程都会执行的代码
    {
        /*count++;*/
        /*printf("count is %d\n", count);*/
        /*printf("i am child!\n");*/
        ;
    }

    sleep(999999999);
    return 0;
} 
