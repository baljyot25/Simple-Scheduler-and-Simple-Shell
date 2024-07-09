# Index

1. [Scheduler.c Overview](#scheduler-c-overview)
2. [Scheduler Operation](#scheduler-operation)
3. [SimpleShell Overview](#simpleshell-overview)
    - 3.1 [Shell_Loop Function](#shell_loop-function)
    - 3.2 [Shell_loop2 Function](#shell_loop2-function)
    - 3.3 [History Function](#history-function)
    - 3.4 [Get_time Function](#get_time-function)
    - 3.5 [Sys_call Handler Function](#sys_call-handler-function)
    - 3.6 [Split Function](#split-function)
    - 3.7 [Create_process_and_run Function](#create_process_and_run-function)

---

# Scheduler.c

## Scheduler.c Overview

- **Shared Memory Setup:** The scheduler initializes shared memory linked from the shell and retrieves variable values.
- **Signal Handling Setup:** Signals, file descriptors for `history.txt`, and a timer to simulate `tslice` are configured.

## Scheduler Operation

1. **Waiting for `tslice`:**
   - Scheduler waits for `tslice` period.

2. **Checking for New Processes:**
   - If new processes are available, they are added to one of 4 priority queues based on priority number or default to queue 1.

3. **Process Handling:**
   - **Enqueuing New Processes:**
     - New processes are created from 2D arrays and enqueued based on specified or default priority.
   - **Round Robin Function:**
     - Determines `current_process_counter` as the minimum of total processes and available processors.
   - **Dequeuing from Queues:**
     - Processes are dequeued from priority queues to fill `current_process_counter`.

4. **Process Execution:**
   - Scheduler sets an alarm to simulate `tslice` and launches each process.
   - Depending on whether a process is new or existing, `create_process_and_run2` or `SIGCONT` is used to resume execution.

5. **End of `tslice`:**
   - Scheduler handles `SIGALRM`:
     - Processes not finished receive `SIGSTOP`.
     - Processes finishing trigger `SIGCHLD`, logging execution details to `history.txt`.
     - Remaining processes have waiting time incremented by `tslice`.

6. **Priority Adjustments:**
   - Every 5 `tslices`, priority levels are adjusted:
     - Processes in higher queues may be moved to lower queues if not finished.

7. **Cycle Repeats:**
   - After adjustments, the scheduler repeats the round robin function to continue the cycle.

---

# SimpleShell

## SimpleShell Overview

### Shell_Loop Function

- **Function Call:** Invoked by the main method to handle user commands via terminal interface.
- **Command Validation:** Checks if the command entered by the user is valid; terminates if input is invalid.
- **File Input Handling:** If command is "fileinput", redirects execution to `shell_loop2` to process commands from `commands.sh`.
- **Command Execution:** Executes valid commands using `create_process_and_run` function.
- **History Command:** If "history" command is entered, displays all previously executed commands and stores them in `history.txt`.
- **Termination Handling:** Allows termination via Ctrl-C, displaying command history before exiting.

### Shell_loop2 Function

- **Function Call:** Called by `shell_loop` if user commands "fileinput".
- **File Input Validation:** Checks if commands in `commands.sh` are readable; terminates if not.
- **Command Validation:** Validates and stores commands from `commands.sh` for processing.
- **History Command:** Displays and logs "history" command similar to `shell_loop`.
- **Command Execution:** Executes commands sequentially from `commands.sh` using `create_process_and_run`.
- **Termination Handling:** Allows termination via Ctrl-C, displaying command history before exiting.

### History Function

- **Function Purpose:** Logs command details from the `line` array into `history.txt`, displays content on terminal.
- **Reset:** Empties the `line` variable after storing commands for future entries.

### Get_time Function

- **Function Purpose:** Sets time-zone, captures local time in readable format, stores in global variable `timeofexec`.

### Sys_call Handler Function

- **Function Purpose:** Catches Ctrl-C interrupt, displays formatted command history.

### Split Function

- **Function Purpose:** Validates command input, stores commands in a 2D array for processing.
- **Piping Support:** Handles commands with pipes, prepares for execution using `execvp`.

### Create_process_and_run Function

- **Function Purpose:** Executes commands entered via `execvp`.
- **Execution Tracking:** Records start and end times of command execution.
- **Piping Support:** Manages command execution for piped commands.
- **Background Execution:** Adds support for running processes in the background using '&' symbol.

#### Details

- **Background Execution Enhancement:** 
  - Checks the second last string of each command line for the '&' symbol.
  - If found, creates a grandchild process inside the child process.
  - Immediately exits the child process, allowing the grandchild process to be managed by the system's init process in the background.
  - Enables the main shell process to continue accepting and executing subsequent commands without waiting for background processes to finish.
