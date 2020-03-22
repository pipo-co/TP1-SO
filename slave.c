#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int childProcess(int argc, char const *argv[])
{
    //argv[1] = Path a file de minisat
    int forkId = fork();
    if(forkId = -1){
        printf("Error en el fork del slave"); //Cambiar por perror
    }


}