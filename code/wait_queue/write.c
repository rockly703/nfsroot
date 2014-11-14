#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int err;
    int buf;
    int fd = open("/dev/rocklee", O_RDWR);
    int len = sizeof(buf);
    
    if(fd < 0){
        printf("open failed!\n");
        return -1;
    }

    buf = strtoul(argv[1], NULL, 16);
    if(write(fd, (char *)&buf, len) < len) {
        printf("write error!\n");
        return -1;
    }

    return 0;
}

