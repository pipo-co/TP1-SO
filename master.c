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

#define MAX_NUM_CHILD 5
#define PIPE_WRITE 1
#define PIPE_READ 0
#define CHILD_PATH "childCode.out"
#define BUFFER_SIZE 10000
#define MAX_INIT_ARGS 20

typedef struct childStruct{
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
    size_t initChildTaskCount; //Menos que MAX_INIT_ARGS
    size_t taskCounter = 1;
    char* initArgsArray[MAX_INIT_ARGS + 2];

    childCount = 1;
    initChildTaskCount = 3;

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

        childArray[i].tasksPending = initChildTaskCount;


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

        close(fdPipeInput[PIPE_READ]);
        close(fdPipeOutput[PIPE_WRITE]);

        // if(write(childArray[i].fdInput, argv[i + 1], strlen(argv[i + 1]) + 1) == -1 )
        //     perror("ERROR ON WRITE");
    }
    

    // fd_set fdSet;
    // char buff[BUFFER_SIZE];
    // size_t aux;
    // while(taskCounter < argc){
    //     FD_ZERO(&fdSet);










    //     FD_ZERO(&fdSet);
    //     for (size_t i = 0; i < childCount; i++)
    //         FD_SET(fdChildOutput[i], &fdSet);

    //     select(fdChildOutput[childCount - 1] + 1, &fdSet, NULL, NULL, NULL);

    //     for (size_t j = 0; j < childCount; j++){
    //         if(FD_ISSET(fdChildOutput[j], &fdSet)){
    //             if((aux = read(fdChildOutput[j], buff, BUFFER_SIZE)) == -1)
    //                 perror("ERROR AL LEER");
    //             if(aux){
    //                 if(aux > 0 && aux < BUFFER_SIZE)
    //                     buff[aux] = 0;

    //                 printf("%s", buff);
    //                 i++;
    //             }
    //         }
    //     } 
    // }
    

    for (size_t i = 0; i < childCount; i++){
        if(wait(NULL) == -1)
            perror("ERROR DE WAIT");
    }

    
}
