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

#define INPUT_MAX_SIZE 1000
#define OUTPUT_MAX_SIZE 1000
#define PIPE_WRITE 1
#define PIPE_READ 0
#define MINISAT_PATH "/usr/bin/minisat"

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    //Por stdin debe venir los path a los archivos minisat que le da master
    //Por stdout devuelve la info que consiguio de minisat
    char buff[INPUT_MAX_SIZE];
    //int fdPipe[2];
    //int forkId;
    int aux;
    
    if((aux = read(STDIN_FILENO, buff, INPUT_MAX_SIZE)) == -1) //Puede aux ser 0?
        perror("FALLO EL READ");

    if(aux > 0 && aux < INPUT_MAX_SIZE)
        buff[aux] = 0;
    
    // if(pipe(fdPipe)){
    //     perror("PIPE INPUT ERROR:");
    //     exit(EXIT_FAILURE);
    // }

    // if((forkId = fork()) == -1){
    //     perror("FORK ERROR");
    //     exit(EXIT_FAILURE);
    // }

    // if(!forkId){
    //     close(fdPipe[PIPE_READ]);

    //     dup2(fdPipe[PIPE_WRITE], STDOUT_FILENO);
        
    //     close(fdPipe[PIPE_WRITE]);

    //     execl(MINISAT_PATH, "minisat", buff, (char*)NULL);
    //     perror("EXEC OF CHILD FAILED");
    // }

    // close(fdPipe[PIPE_WRITE]);
    size_t outputLen = 0;
    char command[OUTPUT_MAX_SIZE];
    sprintf(command, "minisat %s | grep -o -e \"Number of .*[0-9]\\+\" -e \"CPU time.*\" -e \".*SATISFIABLE\"", buff);

    FILE* fp = popen(command, "r"); //Validar

    outputLen = fread(command, sizeof(char), OUTPUT_MAX_SIZE, fp); //Validar que sea EOF o error.
    command[outputLen] = 0;
    printf("%s", command);


    // if((aux = read(fdPipe[PIPE_READ], buff, BUFFER_SIZE)) == -1) //Puede aux ser 0?
    //     perror("FALLO EL READ");

    // if(aux > 0 && aux < BUFFER_SIZE)
    //     buff[aux] = 0;


    //write(STDOUT_FILENO, buff, MAX_INPUT);
    //printf("%s", buff);

    //wait(NULL); //Validar el wait

    return 0;
}