#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int err;
    int fd = open("/dev/rocklee", O_RDWR);
    int buf;
    
    if(fd < 0){
        printf("open failed!\n");
        return -1;
    }

    printf("open successfully\n");
    
    while(read(fd, &buf, sizeof(int)) == sizeof(int)) {
        printf("buf is 0x%x\n", buf);
    }

    return 0;
}

