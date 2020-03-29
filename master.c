#define _POSIX_C_SOURCE 200112L


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
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>


#define MAX_NUM_CHILD 5
#define PIPE_WRITE 1
#define PIPE_READ 0
#define CHILD_PATH "childCode.out"
#define FILE_NAME "answer.txt"
#define SHM_NAME "/shm"
#define SEM_NAME "/sem"
#define OUTPUT_SIZE 10000
#define MAX_INIT_ARGS 20
#define INPUT_MAX_SIZE 1000

typedef struct childStruct{
    pid_t pid;
    int fdInput;
    int fdOutput;
    size_t tasksPending; 
}childStruct;

//Returns total tasks delivered to children on init.
size_t prepareChildren(childStruct childArray[], char const *argv[], size_t childCount, size_t initChildTaskCount, size_t* taskCounter);
void outputInfo(char output[], FILE * file, size_t * mapCounter, char * map, sem_t *sem);

int main(int argc, char const *argv[]){

    childStruct childArray[MAX_NUM_CHILD];

    size_t childCount;
    size_t initChildTaskCount;
    size_t childTasksPerCycle;
    size_t taskCounter = 1;
    size_t totalTasks = argc - 1;
    size_t generalTasksPending = 0;
    size_t mapCounter=0;
    
    setvbuf(stdout, NULL, _IONBF, 0);

    childCount = 10; //Menos que MAX_NUM_CHILD
    initChildTaskCount = 3; //Menos que MAX_INIT_ARGS
    childTasksPerCycle = 5; //La consigna dice que tiene que ser 1. La variable existe para la abstraccion.

    if(childCount > MAX_NUM_CHILD)
        childCount = MAX_NUM_CHILD;

    if(initChildTaskCount > MAX_INIT_ARGS)
        initChildTaskCount = MAX_INIT_ARGS;

    if(childCount*initChildTaskCount >= totalTasks)
        initChildTaskCount = 1; //childCount*initChildTaskCount < totalTasks!!!

    
    FILE * vista = fopen(FILE_NAME, "w");
    if(vista == NULL){
        perror("Error openning vista");
        exit(EXIT_FAILURE);
    }


    generalTasksPending += prepareChildren(childArray, argv, childCount, initChildTaskCount, &taskCounter);
    
    int shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRWXU);
    if(shm == -1){
        perror("Shared memory");
        exit(EXIT_FAILURE);
    }
    if(ftruncate(shm, totalTasks*(100)) == -1){
        perror("Ftruncate");
        exit(EXIT_FAILURE);
    }
    char * map = mmap(NULL,totalTasks*(100), PROT_WRITE, MAP_SHARED, shm, 0);
    if(map== MAP_FAILED){
        perror("Mmap");
        exit(EXIT_FAILURE);
    }

   close(shm);
   
    sem_t * sem = sem_open(SEM_NAME, O_CREAT, O_RDWR, 1);
    if(sem == SEM_FAILED){
        perror("Semaphore");
        exit(EXIT_FAILURE);
    }
    

    printf("%ld\n", totalTasks);
    
    fd_set fdSet;
    int readAux;
    char* auxAnsCounter;
    char output[OUTPUT_SIZE];
    int fdAvailable;
    char inputBuff[INPUT_MAX_SIZE];
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

                    outputInfo(output, vista, &mapCounter, map, sem); //Imprimir en archivo y en memoria compartida

                    auxAnsCounter = output;
                    while((auxAnsCounter = strchr(auxAnsCounter, '\n')) != NULL){ //Hubo tantas respuestas como \n
                        childArray[i].tasksPending--;
                        generalTasksPending--;
                        auxAnsCounter++;
                    }

                    if(childArray[i].tasksPending <= 0){
                        //Le mando tareas al escalvo correspondiente

                        for (size_t j = 0; j < childTasksPerCycle && taskCounter <= totalTasks; j++){
                            sprintf(inputBuff, "%s\n", argv[taskCounter]);
                            write(childArray[i].fdInput, inputBuff, strlen(inputBuff)); //Validar write

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
        if(close(childArray[i].fdInput) == -1)
            perror("CLOSING INPUT");

        if(close(childArray[i].fdOutput) == -1)
            perror("CLOSING OUTPUT");
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

void outputInfo(char output[], FILE * file, size_t * mapCounter, char * map, sem_t * sem){

    fwrite(output, strlen(output), sizeof(output[0]), file );
   //sem_wait(sem);
    for (size_t i = 0; output[i]!= '\0'; i++){
        map[*(mapCounter)++]=output[i];
    }
    map[*(mapCounter)++]=0;
    //sem_post(sem);
    printf("%s", output); //Imprimir en archivo y en memoria compartida
}
