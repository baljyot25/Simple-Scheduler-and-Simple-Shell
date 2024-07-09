## Scheduler.c

### Overview

- **Shared Memory Setup:** The scheduler initializes shared memory linked from the shell and retrieves variable values.
- **Signal Handling Setup:** Signals, file descriptors for `history.txt`, and a timer to simulate `tslice` are configured.

### Scheduler Operation

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

This Markdown structure breaks down the Scheduler.c functionality into sections for clarity and readability. Adjust formatting and headers as needed for your documentation.
