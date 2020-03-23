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
#define COMMAND_MAX_SIZE 1000
#define PIPE_WRITE 1
#define PIPE_READ 0

int processRequest(const char* input, char command[], size_t commandSize);

int main(int argc, char const *argv[]){

    //Por stdin debe venir los path a los archivos minisat que le da master
    //Por stdout devuelve la info que consiguio de minisat parseada

    setvbuf(stdout, NULL, _IONBF, 0);

    char input[INPUT_MAX_SIZE];
    char command[COMMAND_MAX_SIZE];

    // printf("%d\n", argc);
    // for (size_t i = 1; i < argc; i++)
    // {
    //     printf("%s\n", argv[i]);
    // }
    

    for (size_t i = 1; i < argc; i++)
    {
        processRequest(argv[i], command, COMMAND_MAX_SIZE);
        printf("File name:  %s\n%sID of slave who processed it:  %d\n", argv[i], command, getpid());
    }
    

    // if((read(STDIN_FILENO, input, INPUT_MAX_SIZE)) == -1) //Puede aux ser 0?
    //     perror("FALLO EL READ");
    
    // processRequest(input, command, COMMAND_MAX_SIZE);

    // printf("File name:  %s\n%sID of slave who processed it:  %d", input, command, getpid());

    return 0;
}

int processRequest(const char* input, char command[], size_t commandSize){
    size_t commandLen = 0;
    
    sprintf(command, "minisat %s | grep -o -e \"Number of .*[0-9]\\+\" -e \"CPU time.*\" -e \".*SATISFIABLE\"", input);

    FILE* fp = popen(command, "r"); //Validar

    commandLen = fread(command, sizeof(char), commandSize, fp); //Validar que sea EOF o error.
    command[commandLen] = 0;

    return pclose(fp); //Validar pclose
}