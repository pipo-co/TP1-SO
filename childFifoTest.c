#define _DEFAULT_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <limits.h>
#include <fcntl.h> 

#define INPUT_MAX_SIZE 1200
#define COMMAND_MAX_SIZE 1000
#define PIPE_WRITE 1
#define PIPE_READ 0


int main(int argc, char const *argv[])
{
    
    int fd;
    size_t size =1200;
    char input[INPUT_MAX_SIZE];
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("openning");
    fd = open("./fifo1", O_RDONLY);
    if(fd==-1)
        perror("fallo open\n");
    else
    {
        if(read(fd,input, size)== -1)
            perror("fallo read");
        else
            printf("Proceso hijo pid:%ld recibio %s\n",(long int)getpid(), input);
    }
    //printf("Proceso hijo pid:%ld recibio :%s ", (long int)getpid(), argv[1]);
    while (1);    

}
