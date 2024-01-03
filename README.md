# Project Title: Multi-Level Feedback Queue (MLFQ) Task Scheduler

Implemented a multi-threaded task scheduler simulating the MLFQ scheduling policy using C and POSIX threads.

Functionality:

Reads task information from a text file, enqueues tasks into priority queues based on their priority levels.
Employs multiple CPU threads to execute tasks, considering time quantum lengths, and handling I/O as required.
Implements MLFQ rules, including round-robin execution, priority reduction, and periodic boosting of lower-priority tasks.
Concurrency:

Utilizes POSIX threads and mutex locks to ensure thread safety in critical sections.
Performance Metrics:

Calculates and generates a comprehensive report with average turnaround and response times for different task types upon completion.
Time Management:

Tracks and records various time-related attributes for each task, including arrival, first run, completion, turnaround, and response times.
Technologies Used:

C, POSIX Threads
Outcome:

Successfully emulates a multi-level feedback queue scheduling policy, providing insights into task performance and adherence to MLFQ principles.
This project showcases proficiency in concurrent programming, system-level design, and simulation of complex scheduling algorithms.
