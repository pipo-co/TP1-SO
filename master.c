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
#define FILE_NAME "output.txt"
#define SHM_NAME "/shm"
#define SM_NAME "/sm_sem"
#define COMS_NAME "/coms_sem"
#define OUTPUT_MAX_SIZE 250
#define MAX_INIT_ARGS 20
#define INPUT_MAX_SIZE 200

#define HANDLE_ERROR(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

typedef struct childStruct{
    pid_t pid;
    int fdInput;
    int fdOutput;
    size_t tasksPending; 
}childStruct;

//Returns total tasks delivered to children on init.
size_t prepareChildren(childStruct childArray[], char const *argv[], size_t childCount, size_t initChildTaskCount, size_t* taskCounter);
void outputInfo(char output[], FILE * file, size_t * mapCounter, char * map, sem_t * sm_sem, sem_t * coms_sem);
void freeResources(sem_t* sm_sem, sem_t* coms_sem, char* map, size_t totalTasks);

int main(int argc, char const *argv[]){

    childStruct childArray[MAX_NUM_CHILD];

    size_t childCount;
    size_t initChildTaskCount;
    size_t childTasksPerCycle;
    size_t taskCounter = 1;
    size_t totalTasks = argc - 1;
    size_t generalTasksPending = 0;
    size_t mapCounter = 0;
    
    if(setvbuf(stdout, NULL, _IONBF, 0) != 0)
        HANDLE_ERROR("Master - Setvbuf");

    //Seting Configuration Variables and consecuent validation
        childCount = 5; //Menos que MAX_NUM_CHILD
        initChildTaskCount = 3; //Menos que MAX_INIT_ARGS. No puede ser 0.
        childTasksPerCycle = 5; //La consigna dice que tiene que ser 1. La variable existe para la abstraccion.

        if(childCount > MAX_NUM_CHILD)
            childCount = MAX_NUM_CHILD;

        if(initChildTaskCount > MAX_INIT_ARGS)
            initChildTaskCount = MAX_INIT_ARGS;

        if(childCount*initChildTaskCount >= totalTasks || initChildTaskCount < 0)
            initChildTaskCount = 1;
    
    //Shared Memory and Sempahores Initialization
        int shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRWXU);
        if(shm == -1)
            HANDLE_ERROR("Master - shm_open");
        
        if(ftruncate(shm, totalTasks * OUTPUT_MAX_SIZE) == -1)
            HANDLE_ERROR("Master - Ftruncate");
        
        char * map = mmap(NULL, totalTasks * OUTPUT_MAX_SIZE, PROT_WRITE, MAP_SHARED, shm, 0);
        if(map == MAP_FAILED)
            HANDLE_ERROR("Master - Mmap");
        
        if(close(shm) == -1)
            perror("Master - Close Shm");

        sem_t * sm_sem = sem_open(SM_NAME, O_CREAT, O_RDWR, 1);
        if(sm_sem == SEM_FAILED)
            HANDLE_ERROR("Master - Semaphore open sm");
        
        sem_t * coms_sem = sem_open(COMS_NAME, O_CREAT, O_RDWR, 0);
        if(coms_sem == SEM_FAILED)
            HANDLE_ERROR("Master - Semaphore open coms");
    
        FILE * fileOutput = fopen(FILE_NAME, "w");
        if(fileOutput == NULL)
            HANDLE_ERROR("Master - Opening output file");


    generalTasksPending += prepareChildren(childArray, argv, childCount, initChildTaskCount, &taskCounter);
    
    //Le pasamos a View la cantidad de files
    printf("%ld\n", totalTasks);
    
    fd_set fdSet;
    int readAux;
    char* auxAnsCounter;
    char output[OUTPUT_MAX_SIZE];
    int fdAvailable;
    char inputBuff[INPUT_MAX_SIZE];
    while(taskCounter <= totalTasks || generalTasksPending > 0){
        FD_ZERO(&fdSet); 

        for (size_t i = 0; i < childCount; i++)
            FD_SET(childArray[i].fdOutput, &fdSet);  

        fdAvailable = select(childArray[childCount - 1].fdOutput + 1, &fdSet, NULL, NULL, NULL); //Validar el select
        if(fdAvailable == -1)
            HANDLE_ERROR("Master - Select");


        for (size_t i = 0; fdAvailable > 0 && i < childCount && generalTasksPending > 0; i++){
            if(FD_ISSET(childArray[i].fdOutput, &fdSet)){
              
                if((readAux = read(childArray[i].fdOutput, output, OUTPUT_MAX_SIZE)) == -1)
                    HANDLE_ERROR("Master - Read Output from child");
                    
                
                if(readAux){ //A veces lee EOF como available
                    output[readAux] = 0;
                     
                    outputInfo(output, fileOutput, &mapCounter, map, sm_sem, coms_sem); //Imprimir en archivo y en memoria compartida
                          
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
                            
                            if(write(childArray[i].fdInput, inputBuff, strlen(inputBuff))== -1)
                                HANDLE_ERROR("Master - Write to send file path to child");

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
    
    //Collect children and Free Resources
        for (size_t i = 0; i < childCount; i++){
            if(close(childArray[i].fdInput) == -1)
                HANDLE_ERROR("Master - Closing childs' Input ");

            if(close(childArray[i].fdOutput) == -1)
                HANDLE_ERROR("Master - Closing childs' Output ");
        }

        for (size_t i = 0; i < childCount; i++){
            if(wait(NULL) == -1)
                HANDLE_ERROR("Master - Waiting for children");
        }

        if(sem_post(coms_sem)== -1)
                HANDLE_ERROR("Master - Post coms_sem");
        
        freeResources(sm_sem, coms_sem, map, totalTasks);      
}

size_t prepareChildren(childStruct childArray[], char const *argv[], size_t childCount, size_t initChildTaskCount, size_t* taskCounter){

    int fdPipeInput[2];
    int fdPipeOutput[2];
    int forkId;
    char* initArgsArray[MAX_INIT_ARGS + 2];
    int totalTasksDelivered = 0;

    for (size_t i = 0; i < childCount; i++){

        if(pipe(fdPipeInput))
            HANDLE_ERROR("Master - Creating Child Input Pipe");

        childArray[i].fdInput = fdPipeInput[PIPE_WRITE];

        if(pipe(fdPipeOutput))
            HANDLE_ERROR("Master - Creating Child Output Pipe");

        childArray[i].fdOutput = fdPipeOutput[PIPE_READ];

        if((forkId = fork()) == -1)
            HANDLE_ERROR("Master - Forking to Create Child");

        childArray[i].pid = forkId;

        if(!forkId){ //child
            if(close(fdPipeInput[PIPE_WRITE]) == -1)
                perror("Master - Closing Pipe Input");

            if(close(fdPipeOutput[PIPE_READ]) == -1)
                perror("Master - Closing Pipe Output");

            if(dup2(fdPipeInput[PIPE_READ], STDIN_FILENO) == -1)
                HANDLE_ERROR("Master - Dup2 Setting Child Stdin");

            if(dup2(fdPipeOutput[PIPE_WRITE], STDOUT_FILENO) == -1)
                HANDLE_ERROR("Master - Dup2 Setting Child Stdout");

            if(close(fdPipeInput[PIPE_READ]) == -1)
                perror("Master - Closing Child Pipe Input");
            
            if(close(fdPipeOutput[PIPE_WRITE]) == -1)
                perror("Master - Closing Child Pipe Output");

            initArgsArray[0] = CHILD_PATH;
            for(size_t i = 1; i <= initChildTaskCount; i++)
                initArgsArray[i] = argv[(*taskCounter)++];
            initArgsArray[initChildTaskCount + 1] = NULL;

            execv(CHILD_PATH, initArgsArray);
            HANDLE_ERROR("Master - Exec");
        }

        childArray[i].tasksPending = initChildTaskCount;
        totalTasksDelivered += initChildTaskCount;
        *taskCounter += initChildTaskCount;

        if(close(fdPipeInput[PIPE_READ]) == -1)
            perror("Master - Closing Parent Pipe Input");

        if(close(fdPipeOutput[PIPE_WRITE]) == -1)
            perror("Master - Closing Parent Pipe Output");
    }
    return totalTasksDelivered;
}

void outputInfo(char output[], FILE* file, size_t* mapCounter, char* map, sem_t* sm_sem, sem_t* coms_sem){

    int sval;

    if(fwrite(output, strlen(output), sizeof(output[0]), file) == 0)
        HANDLE_ERROR("Master - Fwrite to output file");

    if(sem_wait(sm_sem) == -1)
        HANDLE_ERROR("Master - Wait Semaphore sm_sem");

    for(size_t i = 0; output[i] != 0; i++)
        map[(*mapCounter)++] = output[i];
    map[(*mapCounter)] = 0;

    if(sem_post(sm_sem) == -1)
        HANDLE_ERROR("Master - Post Semaphore sm_sem");

    if(sem_getvalue(coms_sem, &sval) == -1)
        HANDLE_ERROR("Master - Get Value Semaphore coms_sem");

    if(sval < 1)
        if(sem_post(coms_sem) == -1)
            HANDLE_ERROR("Master - Post Semaphore coms_sem");
}

void freeResources(sem_t* sm_sem, sem_t* coms_sem, char* map, size_t totalTasks){
    if(sem_close(sm_sem) == -1)
            perror("Master - Close Semaphore sm_sem");

        if(sem_unlink(SM_NAME) == -1)
            if(errno != ENOENT)
                perror("Master - Unlink Semaphore sm_sem");

        if(sem_close(coms_sem) == -1)
            perror("Master - Close Semaphore coms_sem");

        if(sem_unlink(COMS_NAME) == -1)
            if(errno != ENOENT)
                perror("Master - Unlink Semaphore coms_sem");

        if(munmap(map, totalTasks * OUTPUT_MAX_SIZE) == -1)
           perror("Master - Munmap");

        if(shm_unlink(SHM_NAME) == -1)
            if(errno != ENOENT)
                perror("Master - Unlink Shared Memory");
}
