#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#define size 1024

int main(void)
{
    int fd[size];
    int i;

    memset(fd, 0, size * sizeof(int));

    fd[0] = open("/tmp/rocklee", O_RDWR);
#if 0
    for(i = 0; i < size; i++){
        fd[i] = open("/tmp/rocklee", O_RDWR);
        if(fd[i] < 0){
            printf("fd[%d] error!\n", i);
            goto close;
        }

        printf("fd[%d] is %d\n", i, fd[i]);
    }
        
close:
    for(--i; i >= 0; i--)
        close(fd[i]);

#endif
    printf("fd[0]is %d\n", fd[0]);
    close(fd[0]);
    return 0;
}

