#define _POSIX_C_SOURCE 2

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
#include <signal.h>


#define MAX_NUM_CHILD 5
#define PIPE_WRITE 1
#define PIPE_READ 0
#define CHILD_PATH "childCode.out"
#define OUTPUT_SIZE 10000
#define MAX_INIT_ARGS 20

typedef struct childStruct{
    pid_t pid;
    int fdInput;
    int fdOutput;
    size_t tasksPending; 
}childStruct;

int main(int argc, char const *argv[]){
    int forkId;
    childStruct childArray[MAX_NUM_CHILD];
    int fdPipeInput[2];
    int fdPipeOutput[2];
    size_t childCount;
    size_t initChildTaskCount;
    size_t childTasksPerCycle;
    size_t taskCounter = 1;
    char* initArgsArray[MAX_INIT_ARGS + 2];
    int totalTasks = argc - 1;
    int generalTasksPending = 0;

    setvbuf(stdout, NULL, _IONBF, 0);

    childCount = 1; //Menos que MAX_NUM_CHILD
    initChildTaskCount = 3; //Menos que MAX_INIT_ARGS
    childTasksPerCycle = 1; //La consigna dice que tiene que ser 1. La variable existe para la abstraccion.

    if(childCount > MAX_NUM_CHILD)
        childCount = MAX_NUM_CHILD;

    if(initChildTaskCount > MAX_INIT_ARGS)
        initChildTaskCount = MAX_INIT_ARGS;

    for (size_t i = 0; i < childCount; i++){

        if(pipe(fdPipeInput)){
            perror("PIPE INPUT ERROR:");
            exit(EXIT_FAILURE);
        }

        childArray[i].fdInput = fdPipeInput[PIPE_WRITE];

        if(pipe(fdPipeOutput)){
            perror("PIPE OUTPUT ERROR:");
            exit(EXIT_FAILURE);
        }

        childArray[i].fdOutput = fdPipeOutput[PIPE_READ];

        if((forkId = fork()) == -1){
            perror("FORK ERROR");
            exit(EXIT_FAILURE);
        }

        childArray[i].pid = forkId;

        if(!forkId){ //child
            close(fdPipeInput[PIPE_WRITE]);
            close(fdPipeOutput[PIPE_READ]);

            dup2(fdPipeInput[PIPE_READ], STDIN_FILENO);
            dup2(fdPipeOutput[PIPE_WRITE], STDOUT_FILENO);

            close(fdPipeInput[PIPE_READ]);
            close(fdPipeOutput[PIPE_WRITE]);

            initArgsArray[0] = CHILD_PATH;
            for(size_t i = 1; i <= initChildTaskCount; i++)
                initArgsArray[i] = argv[taskCounter++];
            initArgsArray[initChildTaskCount + 1] = NULL;
            execv(CHILD_PATH, initArgsArray);
            perror("EXEC OF CHILD FAILED");
        }

        childArray[i].tasksPending = initChildTaskCount;
        generalTasksPending += initChildTaskCount;
        taskCounter += initChildTaskCount;

        close(fdPipeInput[PIPE_READ]);
        close(fdPipeOutput[PIPE_WRITE]);
    }
    

    fd_set fdSet;
    int readAux;
    char* auxAnsCounter;
    char buff[OUTPUT_SIZE];
    int fdAvailable;
    while(taskCounter <= totalTasks || generalTasksPending > 0){
        FD_ZERO(&fdSet); 

        for (size_t i = 0; i < childCount; i++)
            FD_SET(childArray[i].fdOutput, &fdSet);  

        fdAvailable = select(childArray[childCount - 1].fdOutput + 1, &fdSet, NULL, NULL, NULL); //Validar el select
        if(fdAvailable == -1)
            perror("SELECT");

        for (size_t i = 0; fdAvailable > 0 && i < childCount && generalTasksPending > 0; i++){
            if(FD_ISSET(childArray[i].fdOutput, &fdSet)){

                if((readAux = read(childArray[i].fdOutput, buff, OUTPUT_SIZE)) == -1)
                    perror("ERROR AL LEER");

                if(readAux){ //A veces lee EOF como available
                    buff[readAux] = 0;

                    printf("%s", buff); //Imprimir en archivo y en memoria compartida

                    auxAnsCounter = buff;
                    while((auxAnsCounter = strchr(auxAnsCounter, '\n')) != NULL){ //Hubo tantas respuestas como \n
                        childArray[i].tasksPending--;
                        generalTasksPending--;
                        auxAnsCounter++;
                    }

                    if(childArray[i].tasksPending <= 0){
                        //Le mando tareas al escalvo correspondiente

                        for (size_t j = 0; j < childTasksPerCycle && taskCounter <= totalTasks; j++){
                            write(childArray[i].fdInput, argv[taskCounter], strlen(argv[taskCounter]) + 1);
                            //Escribo un espacio para separar?
                            taskCounter++;
                            childArray[i].tasksPending++;
                            generalTasksPending++;
                        }
                    }
                }
                fdAvailable--;
            }
        } 
    }
    

    for (size_t i = 0; i < childCount; i++){
        if(kill(childArray[i].pid, SIGKILL) == -1)
            perror("KILL");
    }

    for (size_t i = 0; i < childCount; i++){
        if(wait(NULL) == -1)
            perror("WAIT");
    }
}
