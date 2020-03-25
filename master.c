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

//Returns total tasks delivered to children on init.
size_t prepareChildren(childStruct childArray[], char const *argv[], size_t childCount, size_t initChildTaskCount, size_t* taskCounter);
void outputInfo(char output[]);

int main(int argc, char const *argv[]){

    childStruct childArray[MAX_NUM_CHILD];

    size_t childCount;
    size_t initChildTaskCount;
    size_t childTasksPerCycle;
    size_t taskCounter = 1;
    size_t totalTasks = argc - 1;
    size_t generalTasksPending = 0;
    
    setvbuf(stdout, NULL, _IONBF, 0);

    childCount = 1; //Menos que MAX_NUM_CHILD
    initChildTaskCount = 3; //Menos que MAX_INIT_ARGS
    childTasksPerCycle = 1; //La consigna dice que tiene que ser 1. La variable existe para la abstraccion.

    if(childCount > MAX_NUM_CHILD)
        childCount = MAX_NUM_CHILD; //childCount*initChildTaskCount < totalTasks!!!

    if(initChildTaskCount > MAX_INIT_ARGS)
        initChildTaskCount = MAX_INIT_ARGS;

    generalTasksPending += prepareChildren(childArray, argv, childCount, initChildTaskCount, &taskCounter);
    

    fd_set fdSet;
    int readAux;
    char* auxAnsCounter;
    char output[OUTPUT_SIZE];
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

                if((readAux = read(childArray[i].fdOutput, output, OUTPUT_SIZE)) == -1)
                    perror("ERROR AL LEER");

                if(readAux){ //A veces lee EOF como available
                    output[readAux] = 0;

                    outputInfo(output); //Imprimir en archivo y en memoria compartida

                    auxAnsCounter = output;
                    while((auxAnsCounter = strchr(auxAnsCounter, '\n')) != NULL){ //Hubo tantas respuestas como \n
                        childArray[i].tasksPending--;
                        generalTasksPending--;
                        auxAnsCounter++;
                    }

                    if(childArray[i].tasksPending <= 0){
                        //Le mando tareas al escalvo correspondiente

                        for (size_t j = 0; j < childTasksPerCycle && taskCounter <= totalTasks; j++){
                            write(childArray[i].fdInput, argv[taskCounter], strlen(argv[taskCounter]) + 1); //Validar write
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
        if(kill(childArray[i].pid, SIGKILL) == -1) //Esta bien que sea por kill?
            perror("KILL");
    }

    for (size_t i = 0; i < childCount; i++){
        if(wait(NULL) == -1)
            perror("WAIT");
    }
}

size_t prepareChildren(childStruct childArray[], char const *argv[], size_t childCount, size_t initChildTaskCount, size_t* taskCounter){

    int fdPipeInput[2];
    int fdPipeOutput[2];
    int forkId;
    char* initArgsArray[MAX_INIT_ARGS + 2];
    int totalTasksDelivered = 0;

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

            dup2(fdPipeInput[PIPE_READ], STDIN_FILENO); //Puede fallar?
            dup2(fdPipeOutput[PIPE_WRITE], STDOUT_FILENO);

            close(fdPipeInput[PIPE_READ]);
            close(fdPipeOutput[PIPE_WRITE]);

            initArgsArray[0] = CHILD_PATH;
            for(size_t i = 1; i <= initChildTaskCount; i++)
                initArgsArray[i] = argv[(*taskCounter)++];
            initArgsArray[initChildTaskCount + 1] = NULL;
            execv(CHILD_PATH, initArgsArray);
            perror("EXEC OF CHILD FAILED");
            exit(EXIT_FAILURE);
        }

        childArray[i].tasksPending = initChildTaskCount;
        totalTasksDelivered += initChildTaskCount;
        *taskCounter += initChildTaskCount;

        close(fdPipeInput[PIPE_READ]);
        close(fdPipeOutput[PIPE_WRITE]);
    }

    return totalTasksDelivered;
}

void outputInfo(char output[]){
    printf("%s", output); //Imprimir en archivo y en memoria compartida
}
