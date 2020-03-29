#define _DEFAULT_SOURCE
#define _GNU_SOURCE

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


#define INPUT_MAX_SIZE 1000
#define COMMAND_MAX_SIZE 1000
#define PIPE_WRITE 1
#define PIPE_READ 0
#define REQUEST_DELIM "\n"

int processRequest(const char* input, char command[], size_t commandSize);

int main(int argc, char const *argv[]){

    //Por stdin debe venir los path a los archivos minisat que le da master
    //Por stdout devuelve la info que consiguio de minisat parseada

    setvbuf(stdout, NULL, _IONBF, 0);

    char input[INPUT_MAX_SIZE];
    char command[COMMAND_MAX_SIZE];

    for (size_t i = 1; i < argc; i++){
        processRequest(argv[i], command, COMMAND_MAX_SIZE);
        printf("%d\t%s\t%s\n", getpid(), basename(argv[i]), command);
    }

    int readAux;
    char* request;
    while((readAux = read(STDIN_FILENO, input, INPUT_MAX_SIZE)) != 0){ //Leer hasta EOF
        if(readAux  == -1){
            perror("FALLO EL READ");
            exit(EXIT_FAILURE);
        }
        input[readAux] = 0; //No viene con 0 al final

        request = strtok(input, REQUEST_DELIM);

        //Procesa todas las request en input
        while(request != NULL){
            processRequest(request, command, COMMAND_MAX_SIZE);
            printf("%d\t%s\t%s\n", getpid(), basename(request), command);

            request = strtok(NULL, REQUEST_DELIM);
        }
    }

    return 0;
}

int processRequest(const char* input, char command[], size_t commandSize){
    size_t commandLen = 0;
    char* aux;
    
    sprintf(command, "minisat %s | grep -o -e \"Number of .*[0-9]\\+\" -e \"CPU time.*\" -e \".*SATISFIABLE\" | grep -o -e \"[0-9]\\+.[0-9]\\+\" -e \"[0-9]\\+\" -e \".*SATISFIABLE\"", input);

    FILE* fp = popen(command, "r"); //Validar

    commandLen = fread(command, sizeof(char), commandSize, fp); //Validar que sea EOF o error.
    command[commandLen] = 0;

    while((aux = strchr(command, '\n')) != NULL) //Replace all \n with \t
        *aux = '\t';

    return pclose(fp); //Validar pclose
}