#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct{
    int free;
    pid_t processID;
    int pipeFDS[2];
    char *fileName;
} PipeDictionary;

void print_usage(){
    fprintf(stderr, "Usage: runpar NUMCORES COMMAND... _files_ FILE...\n");
    return;
}


pid_t run_command_in_subprocess(char *file, char *command[], int fileIndex, int writeFD){
    pid_t currentWorkingPID = fork();
    command[fileIndex] = file;
    if (currentWorkingPID == -1){
        fprintf(stderr, "Problem with fork on file: %s\n", file);
        return 0;
    } else if (currentWorkingPID == 0) {
        dup2(writeFD, 1);
        if ((execvp(command[0], command)) < 0){
            fprintf(stderr, "Problem with exec on file: %s\n", file);
            return 0;
        }
    }
    return currentWorkingPID;
    
}

int printout_terminated_subprocess(int status, PipeDictionary *dictionaryItem, int *errorInRun){ 
    if (WEXITSTATUS(status) == EXIT_FAILURE){
        errorInRun++;
        return -1;
    }

   return 0;
}

void print_header(char *name){
    printf("____________________");
    printf("Output from: %s", name);
    printf("--------------------");
    return;
}

int get_free_pipe(PipeDictionary *pipeManagment, int size){
    for (int x = 0; x < size; x++){
        if (pipeManagment[x].free == 0){
            return x;
        }
    }
    return -1;
}

int all_pipes_accounted(PipeDictionary *pipeManagment, int size){
    for (int x = 0; x < size; x++){
        if (pipeManagment[x].free == 1){
            return -1; 
        }
    } 
    return 0;
}

int match_pid_with_struct(PipeDictionary *pipeManagment, int size, pid_t searchPID){
    for (int x = 0; x < size; x++){
        if (pipeManagment[x].processID == searchPID){
            return x; 
        }
    } 
    return -1;
}

int main(int argc, char *argv[]){
    char *command[argc];
    int status, errorInRun = 0, commandIndex = 0, argIndex = 1, numberOfCores = 0;

    if (argc <= 4){
        print_usage();
        exit(EXIT_FAILURE);
    }
    numberOfCores = atoi(argv[1]);
    if (numberOfCores <= 0){
        print_usage();
        exit(EXIT_FAILURE);
    }

    while(strncmp(argv[++argIndex], "_files_", 7)){
        if (argc-1 <= argIndex){
            print_usage();
            exit(EXIT_FAILURE);
        }
        command[commandIndex] = argv[argIndex];
        commandIndex++;
    }
     
    command[commandIndex + 1] = NULL;
    
    PipeDictionary pipeManagment[numberOfCores];
    for (int x = 0; x < numberOfCores; x++){
        pipeManagment[x].free = 0;
         
    }


    for (int fileIndex = argIndex+1; fileIndex < argc; fileIndex++){
        
        int dictIndex = get_free_pipe(pipeManagment, numberOfCores);
    
        if (dictIndex == -1){
            //Wait
        }
        PipeDictionary workingDict = pipeManagment[dictIndex];
        pipe(workingDict.pipeFDS);
        workingDict.free = 1;
         
        pid_t currentWorkingPID = run_command_in_subprocess(argv[fileIndex], command, commandIndex, workingDict.pipeFDS[1]);
        if (currentWorkingPID == 0){
            exit(EXIT_FAILURE);
        }
        workingDict.processID = currentWorkingPID; 
        workingDict.fileName = argv[fileIndex];
    }

    while (all_pipes_accounted(pipeManagment, numberOfCores) != -1){
        pid_t finishedProcess = wait(&status);
        int dictIndex = match_pid_with_struct(pipeManagment, numberOfCores, finishedProcess);
        printout_terminated_subprocess(status, &pipeManagment[dictIndex], &errorInRun);
        break;
    } 
    
}
