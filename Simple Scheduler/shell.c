#include "common.h"

//Initialising the variable for shared memory
shm_t *shm;

//Initialising the variables for storing ncpus and tslice
int ncpus = 0;
double tslice = 0.0;

//Initialising the file descriptor for the shared memory
int fd;

//Initialising the char array to store the normal commands given by the user to the shell for executing without the use of scheduler
char *normal_com[MAX_INPUT_LENGTH];

// Function to parse the command and transform it into a 2d array for easier use
int split(char *command)
{
    //Below portion of the code checks if the command entered by the user only has whitespaces
    int checker = 0;
    int flag = 0;
    while (command[checker] != '\0')
    {
        if (command[checker] != 32)
        {
            flag = 1;
            break;
        }
        checker++;
    }
    if (flag == 0)
        return 0;

    //This portion of the code is used to tokenise each word of the entered command for ease of execution
    int i = 0;
    // Reading the first word of the given command and storing it in s1
    char *s1 = strtok(command, " ");
    if (s1 != NULL && strcmp(s1, "submit") == 0){
        //This block executes if the command entered is supposed to be scheduled in the scheduler
        while (1){
            s1 = strtok(NULL, " ");
            if (s1 == NULL)
                break;
            int j = 0;
            while (s1[j] != 0){
                (shm->process_name)[shm->size][i][j] = s1[j];
                j++;
            }
            (shm->process_name)[shm->size][i][j] = '\0';
            i++;
        }
        (shm->size)++;
        (shm->n_process)++;
        return 1; //Returns 1 if the command is to be executed by the scheduler
    }

    //Comes to this portion of the code if the command entered is not supposed to be scheduled in the scheduler
    normal_com[i++] = s1;
    while (s1 != NULL)
    {
        s1 = strtok(NULL, " ");
        normal_com[i] = s1;
        i++;
    }
    normal_com[i] = NULL;
    return 2; //Returns 2 if the command is to be executed normally without the scheduler
}

// Function to run the command entered (for commands which are not to be scheduled)
int create_process_and_run1(){
    int status = fork();
    if (status < 0){
        printf("Process terminated successfully!");
        exit(1);
    }
    else if (status == 0){
        // Executes the command by using the inbuilt execvp function
        if (execvp(normal_com[0], normal_com) == -1){
            fprintf(stderr, "Error executing command.\n");
            exit(1);
        }
    }
    else waitpid(status, NULL, 0); //Waits for the child process to finish execution
}

// Handles the default and custom signals.
static void syscall_handler(int signum)
{
    //Handles the sys_call SIGTERM
    if (signum == SIGTERM){
        //Unmappping the shared memory region
        if (munmap(shm, sizeof(shm_t)) == -1){
            perror("munmap");
            close(fd);
            exit(1);
        }

        // Close the shared memory file descriptor
        if (close(fd) == -1){
            perror("close");
            exit(1);
        }

        // Unlink the shared memory object
        if (shm_unlink("/my_shared_memory") == -1){
            perror("shm_unlink");
            exit(1);
        }
        exit(0);
    }
    //Handles the sys_call SIGINT (which gets raised when the user does Ctrl-C)
    if (signum == SIGINT){
        shm->is_shell_exit=1;
        while (1){}
    }
}

// Loop for executing all the commands entered by the user at the terminal
void shell_loop()
{
    //Creating the shared memory region and checking for error
    fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
    if (fd == -1){
        printf("Error opening file descriptor for shared memory!\n");
        exit(1);
    }
    //Allocating size of the shared memory region and checking for error
    if (ftruncate(fd, sizeof(shm_t)) != 0){
        printf("Ftruncate Error\n!");
        exit(2);
    }
    //Mapping the shared memory region and checking for error
    shm = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED){
        printf("Mmap failure!\n");
        exit(3);
    }

    //Storing the values in the shared memory for the fields which are required by the scheduler
    shm->ncpus_shm = ncpus;
    shm->tslice_shm = tslice;
    shm->n_process = 0;
    shm->is_shell_exit=0;
    shm->size = 0;
    shm->shell_pid = getpid();

    // Setting the initial process array in the shared memory
    for (int i = 0; i < 256; i++){
        for (int j = 0; j < 64; j++){
            for (int k = 0; k < 64; k++){
                (shm->process_name)[i][j][k] = '\0';
            }
        }
    }

    // Initialisations for the sys_call handler function
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = syscall_handler;
    if (sigaction(SIGINT, &sig, NULL) != 0){
        perror("Unable to set up signal handler");
    }
    if (signal(SIGTERM, syscall_handler) == SIG_ERR){
        perror("Error setting up signal handler");
    }

    int status = 1;

    //Creating a new process
    int s1 = fork();
    if (s1 < 0){ //Handles the abrupt termination of the child process
        printf("Scheduler initialisation failed!\n");
        exit(3);
    }
    else if (s1 == 0){
        //Creating a grandchild process to launch the scheduler process in the background inside the previously created child process
        int s2 = fork();
        if (s2 < 0){ //Handles the abrupt termination of the grandchild process
            printf("Scheduler initialisation failed!\n");
            exit(2);
        }
        else if (s2 == 0){
            char *data[2] = {"./scheduler", NULL};
            shm->scheduler_pid = getpid();
            // Executes the scheduler process by using the inbuilt execvp function
            if (execvp(data[0], data) == -1){
                fprintf(stderr, "Error executing command.\n");
                exit(1);
            }
        }
        else{ //The chidl exits by transferring all its resources to the grandchild process, thus making the grandchild process a background (orpahned) process
            _exit(0);
        }
    }
    else{
        do{
            // Takes input from the user for the command to be executed
            printf("\niiitd@system:~$ ");
            // Initialisation of the variable in which the command entered is stored
            char *command = malloc(MAX_INPUT_LENGTH);
            if (command == NULL)
            {
                printf("Memory allocation error\n");
                exit(5);
            }
            // Stores the command entered by the user in a variable "command"
            if (fgets(command, MAX_INPUT_LENGTH, stdin) == NULL)
            {
                free(command);
                printf("Error reading input\n");
                exit(6);
            }
            // removes the \n character from the end of the string input and replaces it with the null terminator character '\0'
            command[strcspn(command, "\n")] = '\0';
            // Checks if the entered command is NULL;
            if (command == NULL)
                continue;

            // Tokenising the command entered
            int type = split(command);

            // Checking the type of the command
            if (type == 0) continue; // Continue to next iteration if the command only contains whitespaces
            else if (type == 2){
                // Run the command like a normal command without inserting into queue if the entered command does not start with "submit"
                create_process_and_run1();
            }
            //Freeing the command char array to store the next command being given to the shell by the user
            free(command);
        } while (status);
    }
}

int main(int argc, char const *argv[])
{
    //Accepts the ncups and tslice from the command line as input
    ncpus = atoi(argv[1]);
    tslice = atoi(argv[2]);

    // Starts the program execution and calls the shell_loop function which imitates the unix terminal
    shell_loop();
    return 0;
}