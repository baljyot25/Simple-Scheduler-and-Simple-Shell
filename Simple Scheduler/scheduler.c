#include "common.h"

//Initialising the file descriptor required for the history.txt
FILE *f1;

//Initialising the pid variables
pid_t parent_pid;
pid_t pid;

//Initialising the variable to store the details of commands in history.txt
char* line;

//Initialising the variables for storing ncpus and tslice
int ncpus = 0;
double tslice = 0.0;

//Initialising the timer varibale used in set_alarm
struct itimerval timer;

//Initialising the variable to store process details before enqueuing
Process* com_arr;

//Initialising the variable for shared memory
shm_t* shm;

//Initialising the variable to store the total number of processes in all queues
int count = 0;

//Initialising the variable to store the number of running processes in each tslice
int current_process_counter = 0;

// Initialising the variable the store the processes which are running in each tslice
Process** process_arr;

//Declaring the semaphore variable required later
sem_t sem;

// //Decalring the function syscall
// static void syscall_handler(int signum);

//Declaring the function round_robin()
void round_robin();

//Initialising the 4 priority queues
Queue* q1;
Queue* q2;
Queue* q3;
Queue* q4;

//Initialising the file descriptor for the shared memory
int fd;

// we are doing priority boosting after 5 timeslices
int priority_boosting_constant=3;
//counter for priority q boosting

int num_sigalrm=0;


Node* newnode ;

void print_q(Queue * q)
{

     printf("starting q printing.....\n");
    if (q==NULL)
    {
        printf("queue is null\n");
        return;
    }
    else if (q->front==NULL)
    {
        printf("q is empty\n");
        printf ("status of q->end %d\n",q->end==NULL);
        return ;
    }
    // printf("cleared empty area\n");
    Node * temp=q->front;
    Process *p;
    while(temp!=NULL)
    {
        p=temp->process_data;
        printf("f1  %d\n",p->f1);
        // printf("idhar toh aana chahiye\n");
        int j=0;
        printf("command : %d  ",p->com[0][0]);
        while((p->com)[j]!=NULL)
        {
            printf(" %s ",(p->com)[j++]);

        }
        printf("\n");
        temp=temp->next;
        // printf("queue->front==temp  %d\n", q->front==temp);
    }
    printf("ending q printing....\n");
}




//Function to return the queue pointer according to the queue_number passed as an argument
Queue* return_queue(int i){
    if(i==1) return q1;
    else if(i==2) return q2;
    else if(i==3) return q3;
    else if(i==4) return q4;
}

//Function to create all 4 priority queues
void create_queue(){
    //Malloc all 4 priority queues
    q1 = (Queue*)malloc(sizeof(Queue));
    q2 = (Queue*)malloc(sizeof(Queue));
    q3 = (Queue*)malloc(sizeof(Queue));
    q4 = (Queue*)malloc(sizeof(Queue));

    //Checks for malloc error
    if(!(q1) || !(q2) || !(q3) || !(q4)){
        printf("Memory allocation error for queue!");
        exit(7);
    }

    //Making the front and end pointers of each queue as NULL
    (q1)->front = (q1)->end = NULL;
    (q2)->front = (q2)->end = NULL;
    (q3)->front = (q3)->end = NULL;
    (q4)->front = (q4)->end = NULL;
}

//Function to enqueue a process, takes the process to be enqueued and the queue in which it is to be enqueued as arguments
void enqueue(Process* p, Queue* q){

    //Creating a new process node using malloc
    newnode = (Node*)malloc(sizeof(Node));

    //Checking for malloc error
    if(!newnode){
        printf("Memmory allocation error for new node!");
        exit(8);
    }

    //Setting the values of the new node created
    newnode->process_data = p;
    newnode->next = NULL;
    if(p->f1 == 0){
       
        
        //If the process is a new process ()(i.e., it has never been executed before) then start the timer for the response time
        if(clock_gettime(CLOCK_MONOTONIC, &p->start_time) == -1){
            printf("Error executing clock_gettime!");
            exit(1);
        }
    }

    //Checking to see if queue is empty
    if(!(q)->end){
        //Executes if queue is empty
        (q)->front = (q)->end = newnode;
        
        return;
    }

    //Executes if queue is not empty
    (q)->end->next = newnode;
    (q)->end = newnode;
    
}

//Function to dequeue a process from a specified queue which it takes as an argument and finally returns the dequeued process
Process* dequeue(Queue* q){

    //Checking if the queue passed is empty
    if(!q->front){
        printf("Scheduler Table is empty!");
        return NULL;
    }

    //Storing the node pointed to by the front of the queue into temp and updating the front of the queue
    Node* temp = q->front;
    Process* p = temp->process_data;
    q->front = temp->next;

    //Making the end of the queue NULL if the front of the queue is also NULL after dequeuing
    if(!q->front) q->end = NULL;

    //freeing the temp node
    free(temp);

    //Returning the aquired process node from the queue
    return p;
} 

//Function to check whether the queue (passed as an argument) is empty or not (by checking whether both front and the end of the passed queue are NULL)
int isEmpty(Queue* q){
    if(q->front == NULL && q->end == NULL) return 1;
    else return 0;
}

//Function to take the commands passed to the scheduler from the shell input via the shared memory, to create the process nodes and add them into the appropriate queue
void add_processes()
{
    //Iterating over the new commands added into the shared memory array
    for (int i = shm->size - shm->n_process; i < shm->size; i++)
    {
        //Allocating space for the com_arr variable and checking for any errors in malloc
        com_arr = (Process *)malloc(sizeof(Process));
        if (com_arr == NULL) {  
            perror("Error allocating memory for com_arr");
            exit(1);
        }

        //Allocating sspace for the com_arr field com_name which stores the name of the command given as input by the user to the shell and also checking for malloc errors
        com_arr->com_name = (char *)malloc(MAX_INPUT_LENGTH * sizeof(char));
        if (com_arr->com_name == NULL) {
            free(com_arr); // Free the previously allocated memory
            perror("Error allocating memory for com_arr->com_name");
            exit(1);
        }

        //Initialising the variable to iterate over every single word of the command selected
        int j = 0;

        //Running the while loop until the word of the selected command starts with the null character
        while ((shm->process_name)[i][j][0] != 0)
        {
            //Allocating space for the word to be stored in the com field of the com_arr and checking malloc error
            //(Note: Here, the com field of the com_arr stores the words of the command passed in a tokenised format) 
            com_arr->com[j] = (char *)malloc(MAX_INPUT_LENGTH * sizeof(char));
            if (com_arr->com[j] == NULL) {
                 perror("Error allocating memory for com_arr->com[j]");
                 exit(1);
            }

            //Copying the word from the shared memory array into the com field of com_arr
            strcpy(com_arr->com[j], (shm->process_name)[i][j]);
            //Concatenating the word of the shared memory array with the com_name field of the com_arr
            strcat(com_arr->com_name, (shm->process_name)[i][j]);
            strcat(com_arr->com_name, " ");

            //Increasing the word counter
            j++;
        }

        //Resetting the number of new processes to be added to 0;
        shm->n_process = 0;

        //Setting the last word of the com field of the com_arr to be NULL to pass the array into the execvp function without errors
        com_arr->com[j] = NULL;

        //Setting the flag of the newly added process to 0
        //(Note: Here, the flag can three values for each process which are as follows:
        //1) f1 = 0 implies newly added process, has not been executed even once
        //2) f1 = 1 implies an old process, has been executed for at least one tslice
        //3) f1 = 2 implies a finished process, which has completed its execution)
        com_arr->f1 = 0;

        //Calculating the priority number of the newly created process
        int x = com_arr->com[j - 1][0] - '0';

        //Checking whether the priority number passed is not <=4 and >=1
        if (!(x <= 4 && x >= 1))
        {
            //Setting the priority number of the newly created process as 1 if no priority number is passed and also if the priority number is <=0 or >4
            x = 1;
            com_arr->priority_no = x;
        }
        else
        {
            //Setting the priority number as passed by the user
            com_arr->priority_no = x;
            com_arr->com[j - 1] = NULL;
        }
         //Increasing the number of total processes
        count++;
        

        
        //Enqueuing the newly created process according to the new process' priority number
        if (x == 1)
            enqueue(com_arr, q1);
        else if (x == 2)
            enqueue(com_arr, q2);
        else if (x == 3)
            enqueue(com_arr, q3);
        else if (x == 4)
            enqueue(com_arr, q4);
    }
}

//Function to get the history (all commands that have been submitted into the scheduler)
void history() {
    int c;
    //Sets the file pointer to the beginning of the history file
    rewind(f1); 
    //Prints all the content of the history file
    while ((c = fgetc(f1)) != EOF) {
        putchar(c);
    }
}

//Function to imitate the system call handler for the syscall "SIGCHLD" which will be generated whenever one of the currently running processes completes its execution
void sigchld_handler(int signum, siginfo_t *info, void *context){
    // printf("sigchild called for %d\n", info->si_pid);

    //Iterating over all the processes stored in the process_arr to identify which process has terminated
    for(int i = 0;i<current_process_counter;i++){
        //Comparing each process' pid stored in the process_arr with the pid of the process that has sent the SIGCHLD signal
        if(process_arr[i]!=NULL &&   process_arr[i]->pid == info->si_pid){
            //Setting the terminated process' flag to 2 to indicate that the process has finished its execution
            process_arr[i]->f1=2;

             //Decreasing the number of total processes
            count--;
           
            //Adding the tslice to the execution time of the process.
            process_arr[i]->exec_time += tslice;

            //Using sem_wait and sem_post for updating the history.txt to avoid race condition if too many processes are ending at almost the same time
            sem_wait(&sem);
            //Allocating memory for the variable line to store the data of the terminated process in history.txt and handling the malloc error
            line = (char*)malloc(MAX_INPUT_LENGTH*sizeof(char));
            if (line == NULL) {
                printf("Error allocating memory for 'line'");
                exit(1);
            }

            //Defining variable s1 to concatenate double or int values to line variable by converting them into string
            char s1[50];

            //Adding the details of the terminated process to the line char array
            strcat(line, "Command: ");
            strcat(line, process_arr[i]->com_name);
            strcat(line, "\t PID: ");
            sprintf(s1, "%d", process_arr[i]->pid);
            strcat(line, s1);
            strcat(line, "\t Execution Duration: ");
            sprintf(s1, "%f", process_arr[i]->exec_time);
            strcat(line, s1);
            strcat(line, " milliseconds ");
            strcat(line,"\t Wait Time: ");
            sprintf(s1, "%f", process_arr[i]->waiting_time);
            strcat(line, s1);
            strcat(line, " milliseconds ");
            strcat(line,"\t Turnaround Time: ");
            sprintf(s1, "%f", process_arr[i]->waiting_time + process_arr[i]->exec_time);
            strcat(line, s1);
            strcat(line, " milliseconds ");
            strcat(line, "\t Response Time: ");    
            sprintf(s1, "%f", (double)((process_arr[i]->end_time.tv_sec - process_arr[i]->start_time.tv_sec) * 1000.0) + ((process_arr[i]->end_time.tv_nsec - process_arr[i]->start_time.tv_nsec) / 1000000.0));
            strcat(line, s1);
            strcat(line, " milliseconds ");
            strcat(line, "\n\0");

            //Putting the contents of the lien char array into history.txt
            int r = fputs(line, f1);
            fflush(f1);
            if(r == EOF){
                printf("Fputs error!");
                exit(1);
            }
            //Empties the line variable and readies it for more commands to be added for storing in the history
            memset(line,'\0',sizeof(line));

            //Decreasing the number of total processes
            

            sem_post(&sem);
            return;
        }
    }
}

void set_waiting_time(Queue* q){
    Node* temp = q->front;
    while(temp!=NULL){
        temp->process_data->waiting_time += tslice;
        temp = temp->next;
    }
}

void scheduler_syscall_handler(int signum){
    if(signum == SIGALRM){ //Catches SIGALRM signal and handles it accordingly
        num_sigalrm++;
        //Iterates over all the processes in the process_arr
        for (int i=0;i<current_process_counter;i++){
            int status;
            //Checks if the entry of the process_arr is not NULL and the process has not completed its execution
            if (process_arr[i]!=NULL && process_arr[i]->f1 != 2){
                //The scheduler sends a SIGSTOP signal to all the running processes in the process_arr to stop their execution temporarily
                if (kill(process_arr[i]->pid, SIGSTOP)!=0) printf("error in SIGSTOP\n");
                //Adding the tslice to the execution time of the process
                process_arr[i]->exec_time += tslice;   
            }
        }

        //Iterating over all the waiting processes in the queues and adding tslice to their waiting times
        for(int i = 1;i<=4;i++){
            set_waiting_time(return_queue(i));
        }
        

        if (num_sigalrm==priority_boosting_constant)
        {
            
            Queue *q;
            Process * p;

            for (int i=4;i>=2;i--)
            {
                q=return_queue(i);
                while (!isEmpty(q))
                {
                    p=dequeue(q);
                    p->priority_no=1;
                    enqueue(p,q1);
                    
                }
                
                }

            num_sigalrm=0;
            

        }

        
        
        
        //Enqueuing the new processes that were sent during the tslice
        add_processes();
        //Iterating over the processes which were running during the previous tslice
        for(int i = 0;i<current_process_counter;i++){
            //Checking if the process is an unfinished process
            if(process_arr[i]->f1 == 1) {
                if (num_sigalrm==priority_boosting_constant)
                {
                    process_arr[i]->priority_no=1;
                    enqueue(process_arr[i],q1);

                    continue;
                }
                //After every tslice, all processes are moved to the lower priority queue
                //Enqueuing the unfinished processes into the appropriate queues by updating their priority numbers and folowing the above rule
                if(process_arr[i]->priority_no == 4) enqueue(process_arr[i],return_queue(4));
                else{
                    process_arr[i]->priority_no += 1;
                    enqueue(process_arr[i],return_queue(process_arr[i]->priority_no));            
                }                
            }
        }
        

        //Reseting the number of running processes in the process_arr
        current_process_counter=0;

        //Calling round_robin function to simulate the round_robin algorithm of selecting and executing the processes
        round_robin();

        //Enters into the below if block if Ctrl-C has been pressed in shell
        if (shm->is_shell_exit==1){
            //Checks if all the queued processes have been completed and there are no more running processes
            if (count == 0 && isEmpty(q1)&&isEmpty(q2)&&isEmpty(q3)&&isEmpty(q4)){
                printf("Entered inside last block!\n");
                //Printing the details of all the commands that had been given to the scheduler for execution 
                printf("\n");
                printf("Ctrl-C pressed....\n");
                printf("----------------------------------------------------------------------------------------------\n");
                printf("Program History:\n");
                printf("\n");
                history();
                printf("\n");
                printf("----------------------------------------------------------------------------------------------\n");
                // Closing the history file
                fclose(f1);

                //Sending a signal to the shell that the scheduler process is over and that shell should terminate as well
                

                //Cleanup before exiting from the scheduler
                free(q1);
                free(q2);
                free(q3);
                free(q4);
                free(line);
                free(com_arr->com_name);
                for (int i=0;i<256;i++)
                {
                    free(com_arr->com[i]);
                }
                free(com_arr);
                free(process_arr);
                
        
                if (kill(shm->shell_pid,SIGTERM)!=0)
                {
                    perror("Error sending signal");

                }
                munmap(shm, sizeof(shm_t));
                if (munmap(shm, sizeof(shm_t)) == -1)
                {
                    perror("munmap");
                    close(fd);
                    exit(1);
                }
                if (close(fd) == -1)
                {
                    perror("close");
                    exit(1);
                }
                sem_destroy(&sem);
                exit(10);
            }
        }
    }
       
}

void set_alarm() {     
    // Start the timer or raise error if timer cannot be started
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("Error setting the timer");
        exit(1);
    }
}

int create_process_and_run2(Process* p, int i) {
    //Sets the flag of the about to be run process to running
    process_arr[i]->f1=1;
    int ret;
    int status = process_arr[i]->pid = fork(); //Creates a child process
    if(status < 0) { //Handles the case when the child process terminates abruptly
        printf("Process terminated abnormally!");
        exit(2);
    } else if(status == 0){ 
        //Executes the command stored in com field of the process passed to the function by using the inbuilt execvp function
        if (execvp(process_arr[i]->com[0], process_arr[i]->com) == -1) {
            fprintf(stderr, "Error executing command.\n");
            exit(1);
        }
    }
    return 1;
}

void round_robin(){
    
    //Initialising the p variable to store the process from the process_arr
    Process* p = NULL;

    //Checking whether all the 4 priority queues are empty or not
    if (isEmpty(q1) && isEmpty(q2) && isEmpty(q3) && isEmpty(q4))
    {
        //If all 4 queues are empty then again wait for user input
        set_alarm();
        return;
    }

    //Setting the number of processes to be run during this tslice according to number of processors and processes available 
    current_process_counter = (ncpus < count) ? ncpus : count;

    //initialising the q variable
    Queue* q;

    //Initialising the c and cur variables to keep track of total number of processes dequeued and total processes dequeued from a particluar queue respectively
    int c=0,cur=0;

    //Iterating over all 4 priority queues
    for(int i = 1;i<=4;i++){
        //setting the q to the pointer of the queue corresponding to the value of i
        q = return_queue(i);

        //Looping till the total processes dequeued is not equal to the number of processes that needs to be dequeued in this tslice
        while(c<current_process_counter){
            if(isEmpty(q)){ //Break if the current queue is empty
                break;
            }

            //Dequeuing from the current queue and storing in the process_arr
            process_arr[c] = dequeue(q);

            //Incrementing the total number of processes that have been dequeued so far
            c++;
        }

        //If the total processes dequeued has reached the number of processes that had to be dequeued then break from the for loop, irrespective of whether each queue has been visited or not
        if(c==current_process_counter){
            break;
        }   
    }

    //Starting the timer for the tslice
    set_alarm();

    //Iterating over each entry of the process_arr
    for (int i = 0;i < current_process_counter;i++){
        p = process_arr[i];
        if(p->f1 == 0){
            //End the timer of the response time
            if(clock_gettime(CLOCK_MONOTONIC, &p->end_time) == -1){
                printf("Error executing clock_gettime!");
                exit(1);
            }
           
            //If the process at the ith index of the process_arr has never been executed before, then create a new process and lauch the new process
            create_process_and_run2(p,i);
        }
        else if(p->f1 == 1){
            //If the process at the ith index of the process_arr has been executed before, then start the process from where it left off in the previous tslice
            if (kill(p->pid, SIGCONT)!=0) printf("error in SIGCONT");
        }
    }
}

int main()
{
    //Initialising the required signals and linking them to their respective handlers
    struct sigaction sig1;
    memset(&sig1, 0, sizeof(sig1));
    sig1.sa_handler = scheduler_syscall_handler;
    
    if (signal(SIGALRM, scheduler_syscall_handler) == SIG_ERR)
    {
        perror("Error setting up signal handler");
        // return 1;
        exit(5);
    }

   
   // this step is to ensure that when scheduler receives sigint from shell , it does not terminates its child.
    
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
    {
        perror("Error setting up signal handler");
        exit(5);
        // return 1;
    }


    struct sigaction sig2;
    memset(&sig2, 0, sizeof(sig2));
    sig2.sa_sigaction = sigchld_handler;
    sig2.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    sigemptyset(&sig2.sa_mask);
    
    if (sigaction(SIGCHLD, &sig2, NULL)!= 0)
    {
        perror("Unable to set up signal handler");
    }


    //Opens the history.txt file and checks if the file has been opened correctly or not
    f1 = fopen("history.txt", "w+");
    if (f1 == NULL) {
        printf("Error in opening history file!\n");
        exit(1);
    }

    //Create sthe link to the shared memory of the shell and links the scheduler variables with the shared memory
    fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        printf("Error opening file descriptor for shared memory!\n");
        exit(1);
    }
   
    if (ftruncate(fd, sizeof(shm_t)) != 0)
    {
        printf("Ftruncate Error\n!");
        exit(2);
    };
    shm =(shm_t*)mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED)
    {
        printf("Mmap failure!\n");
        exit(3);
    }
    ncpus=shm->ncpus_shm;
    tslice=shm->tslice_shm;
    shm->is_shell_exit=0;
    create_queue();
    

    //Initialisng the semaphore required in the sigchld_handler function
    sem_init(&sem,0,1);

    //Setting the timer to be used for simulating the tslice in the program
    if(tslice >= 10){
        timer.it_value.tv_sec = tslice / 1000;
        timer.it_value.tv_usec = ((int)tslice % 1000) * 1000;
        timer.it_interval.tv_sec = timer.it_interval.tv_usec = 0;
    }
    else{
        timer.it_value.tv_sec = 10 / 1000;
        timer.it_value.tv_usec = (10 % 1000) * 1000;
        timer.it_interval.tv_sec = timer.it_interval.tv_usec = 0;
    }

    //Initialises the process_arr variable used to store the processes in the running queue.
    process_arr=(Process**)malloc(ncpus*sizeof(Process));
    if (process_arr==NULL)
    {
        printf("Memory allocation error\n");
        exit(5);
    }
    for (int i = 0; i < ncpus; i++) {
        process_arr[i] = (Process*)malloc(sizeof(Process));
        if (process_arr[i]==NULL)
        {
            printf("Memory allocation error\n");
            exit(5);
        }

    }
    

    //Starting the first tslice
    set_alarm(); 
    while(1) usleep(1000*tslice);
}