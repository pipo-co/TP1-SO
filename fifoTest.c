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
#include <fcntl.h>           /* Definition of AT_* constants */
    
int main(int argc, char const *argv[])
{
    int  fd;
    char *vuffer[] = {"string numero 1\n", "string numero 2\n"};
    if(mkfifo("fifo1",(mode_t)0666)==-1)
        perror("fallo makefifo");
    
    for (size_t i = 0; i < 2; i++)
    {
    
    switch (fork())
    {
    case -1:
        perror("error while forking\n");
        break;
    case 0:
        execl("childFifoTest.out", "childFifoTest.out", "Holis", (char*)NULL );
        perror("fallo exec\n");
        break;
    }
    }
    /*FD_ZERO(&wfds);
    FD_SET()
    select()*/
    
    while((fd =open("./fifo1", O_WRONLY | O_NONBLOCK ))==-1)
        perror("fallo open\n");
    for (size_t i = 0; i < 2; i++)
    {
     
    
        sleep(2);
        write(fd, vuffer[i], strlen(vuffer[i]));
    }
    return 0;
}
