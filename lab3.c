/*
Caden Austin
Project 3
Worked with Sara Kostmayer
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


typedef struct{ //Helpful struct for working with pipes and processes
    int free;
    pid_t processID;
    int pipeFDS[2];
    char *fileName;
} PipeDictionary;

void print_usage(); //Prints usage to standard error
pid_t run_command_in_subprocess(char *file, char *command[], int fileIndex, int writeFD); //Running subprocess commands
int printout_terminated_subprocess(int status, PipeDictionary *dictionaryItem, int *errorInRun); //Printing terminated Processes
void print_header(char *name); //Prints the header
int get_free_pipe(PipeDictionary *pipeManagment, int size); //Gets a free pipe from the pipe management array
int all_pipes_accounted(PipeDictionary *pipeManagment, int size); //Checks if all proccesses have been accounted for
int match_pid_with_struct(PipeDictionary pipeManagment[], int size, pid_t searchPID); //Matches the pid of the PipeDictionary array with a search PID 


void print_usage(){
    fprintf(stderr, "Usage: runpar NUMCORES COMMAND... _files_ FILE...\n"); //Usage statement
    return;
}


pid_t run_command_in_subprocess(char *file, char *command[], int fileIndex, int writeFD){ //File Index is where to write the file in the command
    command[fileIndex] = file; //add file to command
    pid_t currentWorkingPID = fork(); //forking

    if (currentWorkingPID == -1){ //failed fork
        fprintf(stderr, "Problem with fork on file: %s\n", file);
        return 0; //error return
    } else if (currentWorkingPID == 0) { //child
        dup2(writeFD, 1); //Output set to pipe
        if ((execvp(command[0], command)) < 0){ // Execution and check
            fprintf(stderr, "Problem with exec on file: %s\n", file);
            return 0;
        }
    }
    return currentWorkingPID; //Return pid to keep in struct
    
}

int printout_terminated_subprocess(int status, PipeDictionary *dictionaryItem, int *errorInRun){ //Print terminated Process
    if (WEXITSTATUS(status) == EXIT_FAILURE){ //check for an exit failure
        (*errorInRun)++; //increment file error counter
        return -1; //no print
    }
   
    int readPipe = dictionaryItem->pipeFDS[0]; //read pipe form struct
    fcntl(readPipe, F_SETFL, fcntl(readPipe, F_GETFL) | O_NONBLOCK); //Set non-blocking

    char buffer[4092];
    print_header(dictionaryItem->fileName); //Print Header
    
    int bytesRead; //Count of bytes from read()
    int readState = 1; //Has anything been read yet

    while((bytesRead = read(readPipe, buffer, 4096)) > 0){ //read until nothing remains
        buffer[bytesRead] = '\0'; //sentinal
        printf("%s", buffer); //raw print with sentinal'd buffer
        readState = 0; //Something's been read
    }

    if (errno == EAGAIN && readState == 1){ //EAGAIN and no text read
        printf("Child has no output\n");
    } else if (readState == 1){ //Failed Read
        fprintf(stderr, "Error Reading from pipe for file: %s\n", (dictionaryItem->fileName));
        return -1;
    }

    dictionaryItem->free = 0; //Free that set of pipes
    
    //Close Pipes
    close(readPipe);
    close(dictionaryItem->pipeFDS[1]);
    return 0;
}

void print_header(char *name){ //Helper Header function
    printf("--------------------\n");
    printf("Output from: %s\n", name);
    printf("--------------------\n");
    return;
}

int get_free_pipe(PipeDictionary *pipeManagment, int size){ //Get first pipeset that is free to use
    for (int x = 0; x < size; x++){
        if (pipeManagment[x].free == 0){
            return x;
        }
    }
    return -1;
}

int all_pipes_accounted(PipeDictionary *pipeManagment, int size){ //Check if all processes have been closed from the pipeset
    for (int x = 0; x < size; x++){
        if (pipeManagment[x].free == 1){
            return -1; 
        }
    } 
    return 0;
}

int match_pid_with_struct(PipeDictionary pipeManagment[], int size, pid_t searchPID){ //Matches a PID with the struct array
    for (int x = 0; x < size; x++){
        if (pipeManagment[x].processID == searchPID){
            return x; //return index in struct array
        }
    } 
    return -1;
}

int main(int argc, char *argv[]){
    
    char *command[argc]; //Command Array for Exec
    int status, errorInRun = 0, commandIndex = 0, argIndex = 1, numberOfCores = 0;

    if (argc <= 4){ //Check for needed ammount of params
        print_usage();
        return EXIT_FAILURE;
    }
    numberOfCores = atoi(argv[1]); //Gets number of cores as an int greater than 0
    if (numberOfCores <= 0){
        print_usage();
        return EXIT_FAILURE;
    }

    while(strncmp(argv[++argIndex], "_files_", 7)){ //Adds command until the _files_ flag
        if (argc-1 <= argIndex){
            print_usage();
            return EXIT_FAILURE;
        }
        command[commandIndex] = argv[argIndex];
        commandIndex++;
    }
     
    command[commandIndex + 1] = NULL; //Add cap for execvp use
    
    PipeDictionary pipeManagment[numberOfCores]; //Number of cores worth of pipe sets
    for (int x = 0; x < numberOfCores; x++){
        pipeManagment[x].free = 0; //Initialize Pipesets to 0
    }


    for (int fileIndex = argIndex+1; fileIndex < argc; fileIndex++){ //Iterate through files
        int dictIndex = get_free_pipe(pipeManagment, numberOfCores); //first pipeset free or -1
    
        if (dictIndex == -1){ //No cores free so we wait
            pid_t finishedProcess = wait(&status);
            dictIndex = match_pid_with_struct(pipeManagment, numberOfCores, finishedProcess); //Pipeset associated with process
            printout_terminated_subprocess(status, &pipeManagment[dictIndex], &errorInRun); //Print terminated Process
        }
        
        pipe(pipeManagment[dictIndex].pipeFDS); //Generate pipe 
         
        pid_t currentWorkingPID = run_command_in_subprocess(argv[fileIndex], command, commandIndex, pipeManagment[dictIndex].pipeFDS[1]); //Run subprocess command
        if (currentWorkingPID == 0){
            errorInRun++; //could use exit but don't want to leave orphans
            close(pipeManagment[dictIndex].pipeFDS[0]);
            close(pipeManagment[dictIndex].pipeFDS[1]);
            continue;
        }
        pipeManagment[dictIndex].free = 1; //Unfree pipeset
        pipeManagment[dictIndex].processID = currentWorkingPID; //Add process ID to pipeset
        pipeManagment[dictIndex].fileName = argv[fileIndex]; //Add file name to pipeset for easy use
    }
    
    while (all_pipes_accounted(pipeManagment, numberOfCores) == -1){ //For still running processes
        pid_t finishedProcess = wait(&status);
        int dictIndex = match_pid_with_struct(pipeManagment, numberOfCores, finishedProcess);
        printout_terminated_subprocess(status, &pipeManagment[dictIndex], &errorInRun);
    }

    if (errorInRun != 0){ //if any file ran into an error
        return EXIT_FAILURE;
    } 

    return EXIT_SUCCESS;

}
