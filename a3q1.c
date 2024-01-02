
//-----------------------------------------
// NAME: Devam Patel
// STUDENT NUMBER: 7859163
// COURSE: COMP 3430
// INSTRUCTOR: Franklin Bristow 
// Assignment # 3
// 
// REMARKS: This programs runs all the task in to cpu threads and mimics the mlfq scheduling policy
//-----------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#define MAX_TASKS 1000000
#define MAX_TK_LINE 4
#define QUANT_LENGTH 50
#define TIME_ALLOTMENT 200

#define NANOS_PER_USEC 1000
#define USEC_PER_SEC   1000000

//Global Variables initialized
int numOfTasksInHand = 0;
int numOfCPU = 0;
int VAL_S = 0;
char* fName = "";
int timePeriod_S = 0;
bool readingDone = false;

//Struct Task: stores all the required information for each task
// such as: name, type, lenght, oddsIO, 
// time taken in cpu, time remaining, time allotmentUse, and priority level of itself
typedef struct TASK {
    char task_name[100];
    int task_type ,task_length, oddsIO, timeTaken, timeRemain, allotmentUsed, priority;
    bool firstInstance;                   
    //int tkFirstRun , tkCompletion, tkArrival;
    struct timespec tkFirstRun, tkCompletion, tkArrival;
    struct timespec tkTurnaroundTime, tkResponseTime;
    
} Task;

//Struct node that stores Task and next node refrence
typedef struct NODE {
    Task* tk;
    struct NODE* next;
}Node ; 

//struct Queue that stores the front pointer and rearPointer of the queue as well the size of the queue
typedef struct QUEUE {
    Node *frontPt;
    Node *rearPt;
    int size;

} Queue ;

//initilizing mutex locks
pthread_mutex_t cpuLOCKS;
pthread_mutex_t PriorityLOCKS;
pthread_mutex_t doneLOCK;
pthread_mutex_t rescheduleLOCK;
pthread_mutex_t S_Lock;

//Global queues
Queue *Qreschedule;
Queue *Qpriority1;
Queue *Qpriority2;  
Queue *Qpriority3;
Queue *QDone;

//Functions
void queueInit(Queue* que);
void enqueue(Task *theTask, Queue *que);
Task* dequeue(Queue *que);
void* fileRead();
void boost(Queue* que);
void* schedulingPolicy();
void* cpu();
Task* getTaskDisptach();
static void microsleep(unsigned int usecs);
struct timespec diff(struct timespec start,struct timespec end);
void createReport();


//------------------------------------------------------
// main
// PURPOSE: main function initializes queues and mutexlocks for appropriate threads
//          create 2 threads for reaching and scheduling policy and n number of threads for cpu form user input
//          creates the reporst and destorys the locks at the end.
// INPUT PARAMETERS:
//      argc : number of arguements
//      argc[]: arguements in which the arry are stored in for acces
// OUTPUT PARAMETERS:
//      returns: exit Sucess
//------------------------------------------------------
int main(int argc, char* argv[]){

    (void) argc;
    Qreschedule = malloc(sizeof(Queue));
    Qpriority1 = malloc(sizeof(Queue));
    Qpriority2 = malloc(sizeof(Queue));
    Qpriority3 = malloc(sizeof(Queue));
    QDone = malloc(sizeof(Queue));
    
    queueInit(Qreschedule);
    queueInit(Qpriority1);
    queueInit(Qpriority2);
    queueInit(Qpriority3);
    queueInit(QDone);
 
    numOfCPU = atoi(argv[1]);
    VAL_S = atoi(argv[2]);
    fName = argv[3];

    printf("| CPU: %d |\t | S: %d | \t | FileName: %s |\n",numOfCPU,VAL_S,fName);

    pthread_mutex_init(&cpuLOCKS,NULL);
    pthread_mutex_init(&PriorityLOCKS,NULL);
    pthread_mutex_init(&doneLOCK,NULL);
    pthread_mutex_init(&rescheduleLOCK,NULL);
    pthread_mutex_init(&S_Lock,NULL);


    pthread_t thread[2];
    pthread_t cpu_TH[numOfCPU];

    if( pthread_create(&thread[0],NULL,&fileRead,NULL) != 0 ){
        perror("Error creating FileRead Thread.\n");
    }

    //sleep(2);
    usleep(750000);
    if( pthread_create(&thread[1],NULL,&schedulingPolicy,NULL) != 0 ){
        perror("Error creating Scheduler Thread.\n");
    }
    //numOfCPU = 1;
    for(int i = 0; i < numOfCPU; i++){
       if( pthread_create(&cpu_TH[i],NULL,&cpu,NULL) != 0 ){
            printf("Error creating CPU Thread %d.\n", i);
        } 
    }

    pthread_join(thread[0],NULL);
    pthread_join(thread[1],NULL);

    for(int i = 0; i < numOfCPU; i++){
        pthread_join(cpu_TH[i],NULL);
    }
    
    createReport();

    pthread_mutex_destroy(&cpuLOCKS);
    pthread_mutex_destroy(&PriorityLOCKS);
    pthread_mutex_destroy(&doneLOCK);
    pthread_mutex_destroy(&rescheduleLOCK);
    pthread_mutex_destroy(&S_Lock);
    printf("\n****End of Processing****\n");
    return EXIT_SUCCESS;
}

//------------------------------------------------------
// FileRead
// PURPOSE: fileRead function reads the txt file line by line and stores each word in a line accordingly
//          sends all the reading tasks to highest priority queue at first. Delays the reading process if 
//          the command is found in the txt file else runs the thread until end of file is reach for txt file
//          
// INPUT PARAMETERS:
// OUTPUT PARAMETERS:
//      returns: void
//------------------------------------------------------
void* fileRead(){
    
    //printf("File Name: %s has been passed in reading thread\n", fName);
    int delay = 0;
    FILE *fp = fopen(fName,"r");
    char fLine[150];
    //reads the txt file line by line 
    while(fgets(fLine,sizeof(fLine),fp) != NULL) {
        int i = 0;
        char* fLineCopy = strdup(fLine);
        char *tok = strtok(fLineCopy, " ");
        char *items[MAX_TK_LINE];
        
        //tokenizing each item in a line into array items
        while(tok!=NULL)
        {
            items[i]=tok;
            tok = strtok(NULL," ");
            i ++;
        }
        
        if(strcmp(items[0],"DELAY") != 0){ //if not delay than put everything in Qpriority3 priority
        
            Task *tk = (Task*)malloc(sizeof(Task));
            strcpy(tk->task_name,items[0]);
            //conversion from str to int.
            tk->task_type = atoi(items[1]);
            tk->task_length = atoi(items[2]);
            tk->oddsIO = atoi(items[3]);
            
            tk->priority = 3;                   
            tk->timeTaken = 0;                  
            tk->timeRemain = tk->task_length;   
            tk->allotmentUsed = 0;
            tk->firstInstance = true;           

            // tk->tkFirstRun = 0;
            // tk->tkCompletion = 0;             
            // tk->tkArrival = 0;
            numOfTasksInHand++;
            clock_gettime(CLOCK_REALTIME,&tk->tkArrival);
            //RULE 3: when a job enters the system, it is placed at the highest priority(Top Most Queue)
            pthread_mutex_lock(&PriorityLOCKS);
            enqueue(tk,Qpriority3);    
            pthread_mutex_unlock(&PriorityLOCKS);

        } else { //Delay

            delay = atoi(items[1]);
            microsleep(delay);
        }
    }
    readingDone =  true;
    fclose(fp);
    return NULL;
}

//microsleep function provided from assignment description
static void microsleep(unsigned int usecs){
    long seconds = usecs / USEC_PER_SEC;
    long nanos   = (usecs % USEC_PER_SEC) * NANOS_PER_USEC;
    struct timespec t = { .tv_sec = seconds, .tv_nsec = nanos };
    int ret;
    do
    {
        ret = nanosleep( &t, &t );
        // need to loop, `nanosleep` might return before sleeping
        // for the complete time (see `man nanosleep` for details)
    } while (ret == -1 && (t.tv_sec || t.tv_nsec));
}

//------------------------------------------------------
// queueInit
// PURPOSE: Initilizes the queues front pointer, rear pointer and size to 0;
//          
// INPUT PARAMETERS: Queue object
// OUTPUT PARAMETERS: none
//      returns: void
//------------------------------------------------------
void queueInit(Queue* que){
    que->frontPt = NULL;
    que->rearPt = NULL;
    que->size = 0;
}


//------------------------------------------------------
// enqueue
// PURPOSE: adds the task type node into the queue linked list and points the front and rear of the queues accordingly
//          
// INPUT PARAMETERS: Task struct, Queue struct
// OUTPUT PARAMETERS: none
//      returns: void
//------------------------------------------------------
void enqueue(Task *theTask, Queue *que) {
    Node* newNode;
    newNode = malloc(sizeof(Node));
    if(newNode == NULL){
        perror("Enqueue: Malloc Failed\n");
    }
    newNode->tk = (Task*)malloc(sizeof(Task)) ;
    newNode->tk = theTask;
    newNode->next = NULL;

    if(que->rearPt != NULL){ // if rearPt than connect to rear
        que->rearPt->next = newNode;
        
    } 
    que->rearPt = newNode;
    if(que->frontPt == NULL){ // 
        que->frontPt = newNode; 
    }
    que->size++;

}

//------------------------------------------------------
// dequeue
// PURPOSE: removes the front task from the queue linked list and sets the front pointer to the next
// INPUT PARAMETERS:  Queue struct
// OUTPUT PARAMETERS: none
//      returns: Task struct
//------------------------------------------------------
Task *dequeue(Queue *que){
    Node *tmp = NULL;
    Task *theTask = NULL;
    if(que->frontPt == NULL){

        perror("Dequeue: Queue is Empty\n");

    }  else {
        
        tmp = que->frontPt;
        theTask = que->frontPt->tk;
        
        que->frontPt = que->frontPt->next;
        if(que->frontPt == NULL){
            que->rearPt = NULL;
        }
        
        que->size--;
        free(tmp);
    }
    return theTask;
}

//------------------------------------------------------
// Boost
// PURPOSE: boosts the lower level tasks  to priotrity3 Queues 
// INPUT PARAMETERS:  Queue struct
// OUTPUT PARAMETERS: none
//      returns: Task struct
//------------------------------------------------------
void boost(Queue* que){
    while (que->size > 0){
        Task *tk = dequeue(que);
        tk->priority = 3;
        enqueue(tk, Qpriority3);
    }
}

//------------------------------------------------------
// schedulingPolicy
// PURPOSE: Reads the reschedule queue and places those tasks accordingly to pririty queues, Checks the rule 4 and 2
//          of the mlfq policy (reduce priority if task uses all of time allotment) or (run in round robin fashion). It
//          also checks the time slice for how long all the cpus have run for if > VAL_S than boost all tasks in lower level priority queues
//
// INPUT PARAMETERS:  none
// OUTPUT PARAMETERS: none
//      returns: void
//------------------------------------------------------
void *schedulingPolicy(){
    // will read rescheduleQueueand place them accordinly in Priority Queues
    // mimic the rules set by the mlfq
    //run this loop until finish que != totalTasks
    //Boost task if TimePeriod_S => VAL_S
    //place task from reschedule Queue to the appropirate priority queues
    printf("*Scheduling started\n");
    usleep(300000);
    while(1){
        if(readingDone == true && QDone->size == numOfTasksInHand-1){
            break;
        } else {
            //constantly check for Qreschedule
            if(Qreschedule->size > 0){
                Task *tk = NULL;

                pthread_mutex_lock(&rescheduleLOCK);
                tk = dequeue(Qreschedule);
                pthread_mutex_unlock(&rescheduleLOCK);
                
                pthread_mutex_lock(&PriorityLOCKS);
                //Rule: 4 & 2 Task  run in round robin unless it uses up its time allotment than reduce priority
                if(tk->allotmentUsed < TIME_ALLOTMENT){
                    // round robin
                    if(tk->priority == 3){
                        enqueue(tk,Qpriority3);
                    } else if (tk->priority == 2) {
                        enqueue(tk, Qpriority2);
                    } else if ( tk->priority == 1) {
                        enqueue(tk, Qpriority1);
                    }
                    
                } else {
                    //reduce priority
                    tk->allotmentUsed = 0;
                    if(tk->priority == 3 ){
                        tk->priority = 2;
                        enqueue(tk,Qpriority2);
                    } else if(tk->priority == 2){
                        tk->priority = 1;
                        enqueue(tk,Qpriority1);
                    } else if(tk->priority == 1){
                        enqueue(tk, Qpriority1);
                    }
                }

                pthread_mutex_unlock(&PriorityLOCKS);
                //Rule 5: After some time period S move all jobs in the system to hightest Priority (priority3)
                pthread_mutex_lock(&PriorityLOCKS);
                //timePeriod_S is > S than boost
                if(timePeriod_S >= VAL_S){
                    
                    //set TimePeriod_S to 0;
                    timePeriod_S = 0;
                    boost(Qpriority1);
                    boost(Qpriority2);
                    
                }
                pthread_mutex_unlock(&PriorityLOCKS);
            }
        }
    }
    return NULL;
}

//------------------------------------------------------
// getTaskDispatch
// PURPOSE: returns the task from priority queues by hierarchy levels folliwng RUle 1 of mlfq
//
// INPUT PARAMETERS:  
// OUTPUT PARAMETERS: none
//      returns: Task*
//------------------------------------------------------
Task* getTaskDisptach(){
    
    //Rule 1: highest priority runs first 
    //printf("Tasks from dispatch\n");
    Task *tk;
    if(Qpriority3->size != 0) {
        tk = dequeue(Qpriority3);
    } else if(Qpriority2->size != 0) {
        tk = dequeue(Qpriority2);
    } else if( Qpriority1->size != 0) {
        tk = dequeue(Qpriority1);
    } else {
        tk = NULL;
        
        //printf("Tassks Completed\n");
        printf("TotalNUmOFTASK: %d, QDone->Size: %d\n", numOfTasksInHand, QDone->size);
    }
    //printf("Task sent from dispatch to CPU\n");
    return tk;
}

//------------------------------------------------------
// cpu
// PURPOSE: cpu method that has n number of threads runs until the task is either completed or till quantom length
//          also checks if IO is required for that specific task and checks if task is completed or its needs to be executed again
//
// INPUT PARAMETERS: none
// OUTPUT PARAMETERS: none
//      returns: Task*
//------------------------------------------------------
void *cpu(){

    //run until done Queue does not equal number of total task
    //printf("--CPU Thread: started\n");

    //check if Io is required for this task if so run the task asap
    //cpu put the task to sleep
    // check if time slice is complete
    // if task has time left to run send back to queue
    usleep(300000);
    while(1){
        pthread_mutex_lock(&PriorityLOCKS);
        Task* tk = getTaskDisptach();
        pthread_mutex_unlock(&PriorityLOCKS);
        
        //start Task
        if(tk == NULL){
            //assert(tk != NULL);
            break;
        }
        if(tk->firstInstance == true){
            //first Run
            tk->firstInstance = false;
            clock_gettime(CLOCK_REALTIME,&tk->tkFirstRun);   
        }

        int run = 0;
        int randNum = rand() % 100;

        if(randNum <= tk->oddsIO){
            //Do IO
            //run is a random number <= QUANT_LENGTH
            run = (rand() % QUANT_LENGTH);
        } else {
            // Don't do IO
            //Run for QuantLength
            run = QUANT_LENGTH;
        }

        //Update timePeriod_S
        pthread_mutex_lock(&S_Lock);
        timePeriod_S = timePeriod_S + run;  
        pthread_mutex_unlock(&S_Lock);

        tk->timeRemain = tk->timeRemain - run;
        tk->allotmentUsed = tk->allotmentUsed + run;
        microsleep(run);

        if(tk->timeRemain > 0){
            //Task not completed
            //assign into reschedule queue so scheduler can assign it back
            pthread_mutex_lock(&rescheduleLOCK);
            tk->timeTaken = tk->timeTaken + run;
            enqueue(tk, Qreschedule);
            pthread_mutex_unlock(&rescheduleLOCK);
        } else { 
            //Task Completed
            clock_gettime(CLOCK_REALTIME,&tk->tkCompletion);
            tk->tkTurnaroundTime = diff(tk->tkArrival , tk->tkCompletion);
            tk->tkResponseTime = diff(tk->tkArrival , tk->tkFirstRun);
            pthread_mutex_lock(&doneLOCK);
            enqueue(tk, QDone); 
            pthread_mutex_unlock(&doneLOCK);
        }
    }
    
    
    return NULL;
}

// returns the difference from first input start - end.
// for the calculation of turnaround time and response time
struct timespec diff(struct timespec start,struct timespec end){
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

//------------------------------------------------------
// createReport
// PURPOSE: prints the repost by taking all the tasks that are in DoneQueue and adding them up
//          to calculate the mean response and arrival time.
// INPUT PARAMETERS:  
// OUTPUT PARAMETERS: none
//      returns: Task*
//------------------------------------------------------
void createReport(){

    // Turnaround time of types
    long sumTurnType0 = 0, sumTurnType1 = 0, sumTurnType2 = 0, sumTurnType3=0;
    // Response Time of types
    long sumResType0 = 0, sumResType1 = 0, sumResType2 = 0, sumResType3 = 0;
    //number of types
    int numofType0 = 0, numofType1 = 0, numofType2 = 0, numofType3 = 0;

    Task *tk = NULL;
    //generate turnaround and response
    for(int i = QDone->size; i > 0; i--){
        tk = dequeue(QDone);
        if(tk->task_type == 0) {
            numofType0++;
            sumTurnType0 += tk->tkTurnaroundTime.tv_nsec;
            sumResType0 += tk->tkResponseTime.tv_nsec;
            
        } else if(tk->task_type == 1) {
            numofType1++;
            sumTurnType1 += tk->tkTurnaroundTime.tv_nsec;
            sumResType1 += tk->tkResponseTime.tv_nsec;
            
        } else if(tk->task_type == 2) {       
            numofType2++;
            sumTurnType2 += tk->tkTurnaroundTime.tv_nsec;
            sumResType2 += tk->tkResponseTime.tv_nsec;
            
        } else if(tk->task_type == 3) {
            numofType3++;
            sumTurnType3 += tk->tkTurnaroundTime.tv_nsec;
            sumResType3 += tk->tkResponseTime.tv_nsec;
            
        } else {
            printf("CREATE REPORT: else case NOT VALID ERROR\n");
        }
    }

    // printf("\n**Average turnaround time per type:**\n");
    // printf("\tType 0: %ld usec\n",((sumTurnType0/numofType0)/NANOS_PER_USEC));
    // printf("\tType 1: %ld usec\n",((sumTurnType1/numofType1)/NANOS_PER_USEC));
    // printf("\tType 2: %ld usec\n",((sumTurnType2/numofType2)/NANOS_PER_USEC));
    // printf("\tType 3: %ld usec\n",((sumTurnType3/numofType3)/NANOS_PER_USEC));

    // printf("\n**Average response time per type:**\n");
    // printf("\tType 0: %ld usec\n",((sumResType0/numofType0)/NANOS_PER_USEC));
    // printf("\tType 1: %ld usec\n",((sumResType1/numofType1)/NANOS_PER_USEC));
    // printf("\tType 2: %ld usec\n",((sumResType2/numofType2)/NANOS_PER_USEC));
    // printf("\tType 3: %ld usec\n",((sumResType3/numofType3)/NANOS_PER_USEC));

    printf("\n---Turnaround Time--|--Response Time---\n");
    printf("Type 0: %ld usec | Type 0: %ld usec\n",((sumTurnType0/numofType0)/NANOS_PER_USEC),((sumResType0/numofType0)/NANOS_PER_USEC));
    printf("Type 1: %ld usec | Type 1: %ld usec\n",((sumTurnType1/numofType1)/NANOS_PER_USEC),((sumResType1/numofType1)/NANOS_PER_USEC));
    printf("Type 2: %ld usec | Type 2: %ld usec\n",((sumTurnType2/numofType2)/NANOS_PER_USEC),((sumResType2/numofType2)/NANOS_PER_USEC));
    printf("Type 3: %ld usec | Type 3: %ld usec\n",((sumTurnType3/numofType3)/NANOS_PER_USEC),((sumResType3/numofType3)/NANOS_PER_USEC));

}
