#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_LINE_LENGTH 100000

FILE *f1;
int and_counter=1;
pid_t pid;
pid_t parent_pid;
char line[MAX_LINE_LENGTH] = "";

char timeofexec[50];
struct timespec start_time_of_exec;
struct timespec end_time_of_exec;
int rows;
int flag1 = 0;
int no_of_pipes=0;
int no_of_ands=0;

//Function to parse the command and transform it into a 2d array for easier use 
int split(char* command, char* com[10][MAX_INPUT_LENGTH]) {
    int checker=0;
    int flag=0;
    // printf("%d \n",command[0]);
    while(command[checker]!='\0')
    {
        if (command[checker]!=32)
        {
           
            flag=1;
            break;
        }
        checker++;
    }
    if (flag==0) 
    {
       
        return 0 ;
    }
    
    int i = 0,j=0;
    // printf("command %s\n", command);
    //Reading the first word of the given command and storing it in s1
    char* s1 = strtok(command, " ");
    while (s1 != NULL) {
        com[j][i]=s1;
        // printf("com[%d][%d]  : %s\n", j,i,com[j][i]);
        i++;
        
        
        // If s1 is a pipe, then making the last element of the current row as NULL and switching to the next row
        if(s1[0] == '|'){
            com[j][i] = NULL; 
            // printf("com[%d][%d]  : %s\n", j,i,com[j][i]);
            j++;
            i=0;
            no_of_pipes++;
        }
        if (s1[0]=='&')
        {
             com[j][i] = NULL; 
           // printf("com[%d][%d]  : %s\n", j,i,com[j][i]);
            j++;
            i=0;
            no_of_ands++;

        }
        
        s1 = strtok(NULL, " ");
        
    }
    //Setting the last element of the last row as NULL
   if (i==0)
   {
    rows =j;
   }
   else{
    rows=j+1;
   }
    com[j][i] = NULL;
    // printf("com[%d][%d]  : %s\n", j,i,com[j][i]);
    //Setting the variable "rows" which stores the number of rows in the 2d array.
    // rows = j+1;

    return 1;
}

//Function to calculate start time of execution of a command
void get_time() {
    //Setting the time zone
    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time_t current_time;
    //Getting the current time 
    time(&current_time);
    //Converting the time to the local time
    struct tm *timeinfo = localtime(&current_time);
    //Converting the format of the time we got to a readable and understandable format
    strftime(timeofexec, sizeof(timeofexec), "%d-%m-%Y %H:%M:%S", timeinfo);
}

//Function to get the history (all commands that have been issued so far)
void history() {
    //Puts the entire content that has been stored in line into the history file
    int r = fputs(line, f1);
    if(r == EOF){
        printf("Fputs error!");
        exit(1);
    }

    int c;
    //Sets the file pointer to the beginning of the history file
    rewind(f1); 
    //Prints all the content of the history file
    while ((c = fgetc(f1)) != EOF) {
        putchar(c);
    }
    //Empties the line variable and readies it for more commands to be added for storing in the history
    memset(line, '\0', sizeof(line));
}

//Function to run the command entered
int create_process_and_run(char* com[][MAX_INPUT_LENGTH]) {
    int ret;
    //Stores the end time of the execution of the command
    if(clock_gettime(CLOCK_MONOTONIC, &start_time_of_exec) == -1){
        printf("Error executing clock_gettime!");
        exit(1);
    }
    int j = 0;
    //Checks if the command entered has any pipes or not
    // printf("rows %d no of ands %d\n",rows,no_of_ands);
    if( rows == 1){
        
        int cnt=0;
        // printf("%d",com[0][2][0]);
        while (com[0][cnt]!=NULL)
        {
            
            cnt++;
        }
        if (com[j][cnt-1][0]==38)// ascii of &
        {
            flag1 = 1;
            com[j][cnt-1]=NULL;
            //If the entered command doesn't have pipes, then this block is executed
            int status = fork(); //Creates a child process
            if (status < 0) { //Handles the case when the child process terminates abruptly
                printf("Process terminated abnormally!");
                return 0;
            } 
            else if (status == 0) { //Child process
                //Executes the command by using the inbuilt execvp function
                int status2=fork();
                if (status2<0){
                    printf("Process child terminated abnormally!");
                    return 0;
                } else if (status2==0){
                    sleep(2);              
                    if (execvp(com[0][0] ,com[j]) == -1) {
                        fprintf(stderr, "Error executing command.\n");
                        exit(1);
                    }
                }
                _exit(0);                              
            }
            else{ //Parent process
                //Waits for the child to complete execution and stores the process ID of the child process
                printf("[%d]%d",and_counter++,getpid());
                pid = wait(&ret);
                //Stores the end time of the execution of the child process
                if(clock_gettime(CLOCK_MONOTONIC, &end_time_of_exec) == -1){
                    printf("Error executing clock_gettime!");
                    exit(1);
                }
                return 1;
            }
        }
        else
        {

            int status = fork(); //Creates a child process
            if (status < 0) { //Handles the case when the child process terminates abruptly
                printf("Process terminated abnormally!");
                return 0;
            } else if (status == 0) { //Child process
                //Executes the command by using the inbuilt execvp function
            
                
                if (execvp(com[j][0], com[j]) == -1) {
                    fprintf(stderr, "Error executing command.\n");
                    exit(1);
                }
            }
            else{ //Parent process
                //Waits for the child to complete execution and stores the process ID of the child process
                pid = wait(&ret);
                //Stores the end time of the execution of the child process
                if(clock_gettime(CLOCK_MONOTONIC, &end_time_of_exec) == -1){
                    printf("Error executing clock_gettime!");
                    exit(1);
                }
                return 1;
             }

        }
    }   

    else{
        // printf("main idhar kyu hun\n");
         //If the entered command has pipes, then this block is executed

        if (no_of_pipes==0)
        {
            

            for (int k=0;k<rows;k++)
            {
                 int cnt=0;

                //  printf("yaar k : %d\n",k);
            // printf("%d",com[0][2][0]);
                while (com[k][cnt]!=NULL)
                {
                    
                    cnt++;
                }


                if (com[k][cnt-1][0]==38)// ascii of &
                {
                    // printf("with &\n");
                    flag1 = 1;
                    com[k][cnt-1]=NULL;
                    
                    int status = fork(); //Creates a child process
                    if (status < 0) { //Handles the case when the child process terminates abruptly
                        printf("Process terminated abnormally!");
                        return 0;
                    } 
                    else if (status == 0) { //Child process
                        //Executes the command by using the inbuilt execvp function
                        int status2=fork();
                        if (status2<0){
                            printf("Process child terminated abnormally!");
                            return 0;
                        } else if (status2==0){
                            sleep(2);              
                            if (execvp(com[k][0] ,com[k]) == -1) {
                                fprintf(stderr, "Error executing command.\n");
                                exit(1);
                            }
                        }
                        _exit(0);  
                    }
                     else{ //Parent process
                    //Waits for the child to complete execution and stores the process ID of the child process
                    printf("[%d]%d\n",and_counter++,getpid()+k);
                    pid = wait(&ret);
                    //Stores the end time of the execution of the child process
                    if(clock_gettime(CLOCK_MONOTONIC, &end_time_of_exec) == -1){
                        printf("Error executing clock_gettime!");
                        exit(1);
                    }
                    continue;
                         }
                    }
                else
                    {
                        // printf("without &\n");
                        int status = fork(); //Creates a child process
                        if (status < 0) { //Handles the case when the child process terminates abruptly
                            printf("Process terminated abnormally!");
                            return 0;
                        } else if (status == 0) { //Child process
                            //Executes the command by using the inbuilt execvp function
                        
                            
                            if (execvp(com[k][0], com[k]) == -1) {
                                fprintf(stderr, "Error executing command.\n");
                                exit(1);
                            }
                        }
                        else{ //Parent process
                            //Waits for the child to complete execution and stores the process ID of the child process
                            pid = wait(&ret);
                            //Stores the end time of the execution of the child process
                            if(clock_gettime(CLOCK_MONOTONIC, &end_time_of_exec) == -1){
                                printf("Error executing clock_gettime!");
                                exit(1);
                            }
                          
                        }

                    }
            }
            return 1;
        }
        
        
        else{
            int is_first_pipe=1;
            int is_prev_pipe=0;
            int fd[2][2];
            int pipe_counter=0;
             for (int k=0;k<rows;k++)
            {
                 int cnt=0;
                  while (com[k][cnt]!=NULL)
                {
                    
                    cnt++;
                }

                if (com[k][cnt-1][0]==38 )
                {
                     flag1 = 1;
                    com[k][cnt-1]=NULL;
                    
                    int status = fork(); //Creates a child process
                    if (status < 0) { //Handles the case when the child process terminates abruptly
                        printf("Process terminated abnormally!");
                        return 0;
                    } 
                    else if (status == 0) { //Child process
                        //Executes the command by using the inbuilt execvp function
                        int status2=fork();
                        if (status2<0){
                            printf("Process child terminated abnormally!");
                            return 0;
                        } else if (status2==0){
                            sleep(2);              
                            if (execvp(com[k][0] ,com[k]) == -1) {
                                fprintf(stderr, "Error executing command.\n");
                                exit(1);
                            }
                        }
                        _exit(0);  
                    }
                     else{ //Parent process
                    //Waits for the child to complete execution and stores the process ID of the child process
                    printf("[%d]%d\n",and_counter++,getpid()+k);
                    pid = wait(&ret);
                    //Stores the end time of the execution of the child process
                    if(clock_gettime(CLOCK_MONOTONIC, &end_time_of_exec) == -1){
                        printf("Error executing clock_gettime!");
                        exit(1);
                    }
                    continue;
                         }

                }
                else 
                {
                
                    com[k][cnt-1]=NULL;
                    if (k==1)
                    {
                      
                        // printf("k==1\n");
                        pipe(fd[0]);
                        int status = fork();
                        if (status == 0) {//child process
                         dup2(fd[0][1],STDOUT_FILENO); 
                       
                        //closing all the pipes that child has inherited from the parent
                       
                    //    close(fd[0][0]);
                       close(fd[0][1]);
                         execvp(com[1][0], com[1]) ;

                        //Executes the command by using the inbuilt execvp function
                    
                        } else {//parent process
                            pid = wait(&status); //Waits for the child to complete execution
                            close(fd[0][0]);
                       close(fd[0][1]);
                        //Closes the writing end of the pipe associated with this process
                           

                            
                        }


                  
                    
                    }
                    else{
                        // printf("k==2\n");
                         pipe(fd[1]);
                         int status = fork();
                         if (status == 0) {//child process
                         dup2(fd[0][0],STDIN_FILENO); 
                       
                        //closing all the pipes that child has inherited from the parent
                       
                       
                         execvp(com[2][0], com[2]) ;

                        //Executes the command by using the inbuilt execvp function
                    
                        } else {//parent process
                        
                            pid = wait(&status); //Waits for the child to complete execution
                        //Closes the writing end of the pipe associated with this process
                        // printf("parent of k=2\n");
                           

                            
                        }

                    }

                   
               

            }
        }
        
       
        //Stores the end time of the execution of the child process
        if(clock_gettime(CLOCK_MONOTONIC, &end_time_of_exec) == -1){
            printf("Error executing clock_gettime!");
            exit(1);
        }
        return 1; 
    }
}
}
//Handles the Crtl-C function by printing the history and then terminating the program
static void syscall_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n");
        printf("Ctrl-C pressed....\n");
        printf("----------------------------------------------------------------------------------------------\n");
        printf("Program History:\n");
        printf("\n");
        history();
        printf("\n");
        printf("----------------------------------------------------------------------------------------------\n");
        printf("Program terminated!\n");
        exit(0);
    }
}

//Loop for executing all the commands given in the file as input
void shell_loop2(){
    parent_pid = getppid();
    int status1;
    double duration1;
    char s2[50];
    //Opens the commands.sh file and checks if the file has been opened correctly or not
    FILE* f2 = fopen("command.sh","r");
    if(f2 == NULL){
        printf("Error reading file!");
        exit(1);
    }
    //Intialisation of the 2d array in which the command is to be stored for processing
    char* com1[10][MAX_INPUT_LENGTH];

    //Initialisation of the variable in which the command entered is stored
    char* command1 = malloc(MAX_INPUT_LENGTH);
    if(command1 == NULL){
        printf("Error in malloc!");
        exit(1);
    }
    int c1;
    //reads all the commands written in the commands.sh file
    while(1){
        //Stores the command enteres by the user in a variable "command1"
        if (fgets(command1, MAX_INPUT_LENGTH, f2) == NULL) {
            break;
        }
        //  puts(command1);
        //removes the \n character from the end of the string input and replaces it with the null terminator character '\0'
        command1[strcspn(command1, "\r")] = '\0';
        //  puts(command1);

        //Checks if the entered command is NULL;
        if (command1 == NULL) {
            continue;
        } 

        //Copies the entered command into a new variable for storing in history
        char* copiedCommand1 = malloc(strlen(command1) + 1); // +1 for the null terminator
        if (copiedCommand1 == NULL) {
            printf("Memory allocation error\n");
            free(copiedCommand1); // Free the original 'command' if allocation fails
            exit(3);
        }
        strcpy(copiedCommand1, command1);

        //Parses the entered commmand and stores in the format of a 2d array
        // if we find a line with only emptyspace or a null character we will ignore that line
        if (split(command1, com1)==0)
        {
            continue;
        }

        // printf("%d \n",com1[0][0][2]);
        // puts(com1[0][1]);
        //  if (strcmp(com1[0][0], "ls") == 0) printf("wow\n");
        //Checks if the command entered was history, if yes then prints the history (all commands that have been entered uptil now)
        if (strcmp(com1[0][0], "history") == 0) {
            //Stores the time at which the command execution began in the global variable timeofexec
            get_time();
            //Calls the history function to display the history
            history();
            //Calculates the exxecution duration of the command entered
            duration1 = (double)((end_time_of_exec.tv_sec - start_time_of_exec.tv_sec) * 1000.0) + ((end_time_of_exec.tv_nsec - start_time_of_exec.tv_nsec) / 1000000.0);
            //Adds all the details of the command execution to the line variable
            strcat(line, "Command: history");
            strcat(line, "\tPID: ");
            sprintf(s2, "%d", (int)parent_pid);
            strcat(line, s2);
            strcat(line, "\tExecution Start Time: ");
            strcat(line, timeofexec);
            strcat(line, "\tExecution Duration: ");
            sprintf(s2, "%f", duration1);
            strcat(line, s2);
            strcat(line, " milliseconds ");
            strcat(line, "\n");
            printf("8\n"); 
        } else {
            //Stores the time at which the command execution began in the global variable timeofexec
            get_time();
            //Executes the command
            status1 = create_process_and_run(com1);
            if(flag1 == 1){
                duration1 = 0.0;
                flag1 = 0;
            }
            else{
                //Calculates the exxecution duration of the command entered
                duration1 = (double)((end_time_of_exec.tv_sec - start_time_of_exec.tv_sec) * 1000.0) + ((end_time_of_exec.tv_nsec - start_time_of_exec.tv_nsec) / 1000000.0);
            }
            //Adds all the details of the command execution to the line variable
            strcat(line, "Command: ");
            strcat(line, copiedCommand1);
            strcat(line, "\tPID: ");
            sprintf(s2, "%d", (int)pid);
            strcat(line, s2);
            strcat(line, "\tExecution Start Time: ");
            strcat(line, timeofexec);
            strcat(line, "\tExecution Duration: ");
            sprintf(s2, "%f", duration1);
            strcat(line, s2);
            strcat(line, " milliseconds ");
            strcat(line, "\n"); 
        }
        free(copiedCommand1);
    }
    fclose(f2);
    free(command1);
}

//Loop for executing all the commands entered by the user at the terminal
void shell_loop() {
    //Initialisations for the Crtl-C handler function
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = syscall_handler;
    sigaction(SIGINT, &sig, NULL);

    //Intialisation of the 2d array in which the command is to be stored for processing
    char* com[10][MAX_INPUT_LENGTH];
    parent_pid = getppid();
    int status = 1;
    //Opens the history.txt file and checks if the file has been opened correctly or not
    f1 = fopen("history.txt", "w+");
    if (f1 == NULL) {
        printf("Error in opening file!\n");
        exit(1);
    }
    double duration;
    
    char s1[50];
    do {
        //Takes input from the user for the command to be executed
        printf("\niiitd@system:~$ ");
        //Initialisation of the variable in which the command entered is stored
        char* command = malloc(MAX_INPUT_LENGTH);
        if (command == NULL) {
            printf("Memory allocation error\n");
            exit(3);
        }

        //Stores the command enteres by the user in a variable "command"
        if (fgets(command, MAX_INPUT_LENGTH, stdin) == NULL) {
            free(command);
            printf("Error reading input\n");
            exit(4);
        }
        //removes the \n character from the end of the string input and replaces it with the null terminator character '\0'
        command[strcspn(command, "\n")] = '\0';
        //Checks if the entered command is NULL;
        if (command == NULL) {
            continue;
        }
        //Checks if the command entered was "fileinput", if yes then redirects to shell_loop2 and executes all the commands written in the commands.sh file
        if(strcmp(command,"fileinput") == 0){
            shell_loop2();
            
            continue;

        }
        
        //Copies the entered command into a new variable for storing in history

        char* copiedCommand = malloc(strlen(command) + 1); // +1 for the null terminator
        if (copiedCommand == NULL) {
            printf("Memory allocation error\n");
            free(command); // Free the original 'command' if allocation fails
            exit(3);
        }
        strcpy(copiedCommand, command);

        //Parses the entered commmand and stores in the format of a 2d array
        //split function returns 0 if the given iput cannot pe parsed into a 2d array
        if (split(command, com)==0)
        {
            printf("Not a valid command\n");
            continue;
        }

        //Checks if the command entered was history, if yes then prints the history (all commands that have been entered uptil now)
        if (strcmp(com[0][0], "history") == 0) {
            //Stores the time at which the command execution began in the global variable timeofexec
            get_time();
            //Calls the history function to display the history
            history();
            //Calculates the exxecution duration of the command entered
            duration = (double)((end_time_of_exec.tv_sec - start_time_of_exec.tv_sec) * 1000.0) + ((end_time_of_exec.tv_nsec - start_time_of_exec.tv_nsec) / 1000000.0);
            //Adds all the details of the command execution to the line variable
            strcat(line, "Command: history");
            strcat(line, "\tPID: ");
            sprintf(s1, "%d", (int)parent_pid);
            strcat(line, s1);
            strcat(line, "\tExecution Start Time: ");
            strcat(line, timeofexec);
            strcat(line, "\tExecution Duration: ");
            sprintf(s1, "%f", duration);
            strcat(line, s1);
            strcat(line, " milliseconds ");
            strcat(line, "\n"); 
        } else {
            //Stores the time at which the command execution began in the global variable timeofexec
            get_time();
            //Executes the command
            status = create_process_and_run(com);
            if(flag1 == 1){
                duration = 0.0;
                flag1 = 0;
            }
            else{
                //Calculates the exxecution duration of the command entered
                duration = (double)((end_time_of_exec.tv_sec - start_time_of_exec.tv_sec) * 1000.0) + ((end_time_of_exec.tv_nsec - start_time_of_exec.tv_nsec) / 1000000.0);
            }
            //Adds all the details of the command execution to the line variable
            strcat(line, "Command: ");
            strcat(line, copiedCommand);
            strcat(line, "\tPID: ");
            sprintf(s1, "%d", pid);
            strcat(line, s1);
            strcat(line, "\tExecution Start Time: ");
            strcat(line, timeofexec);
            strcat(line, "\tExecution Duration: ");
            sprintf(s1, "%f", duration);
            strcat(line, s1);
            strcat(line, " milliseconds ");
            strcat(line, "\n"); 
        }
         free(command);
        free(copiedCommand);
    } while (status);
    fclose(f1);
}

int main(int argc, char const* argv[]) {
    //Starts the program execution and calls the shell_loop function which imitates the unix terminal
    shell_loop();
    return 0;
}