//
// Created by zhangfuwen on 2022/4/1.
//

#include <iostream>
#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

int main() {
    int fd = open("/sys/kernel/debug/kgsl/kgsl-3d0/profiling/pipe", O_RDONLY);
    if(fd < 0) {
        printf("failed to open pipe file %d, %s", fd, strerror(errno));
        return errno;
    }

    char buff[1024*1000];
    while(true) {
        auto ret = read(fd, buff, sizeof(buff));
        if(ret < 0) {
            printf("failed  to read from file, %d, %s",ret, strerror(errno));
            close(fd);
            return ret;
        }

        if(ret > 0) {
            printf("%s\n", buff);
        }
    }

    close(fd);
    return 0;
}