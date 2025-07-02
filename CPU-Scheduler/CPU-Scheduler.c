#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef enum {
    ARRIVAL, SHORTEST_JOB, PRIORITY
} sort_by;

typedef struct process {
    char Name[51];
    char Desciption[101];
    int Arrival_Time;
    int Burst_Time;
    int Remaining_Time;
    int Priority;
    pid_t pid;
} P;

char buf[396];

void output_to_screen() {
    write(STDOUT_FILENO, buf, strlen(buf));
}

/**
 * @brief Forks a process
 * @return 0 uppon successful completion
 */
pid_t Fork(void) {
    pid_t pid;

    if ((pid = fork()) < 0)
    {
        perror("fork failed");
    }

    return pid;
}

FILE * get_csv_file(char * path) {

    FILE * file = fopen(path, "r+");  
    if (file == NULL) {
        perror("csv file not found!");
        exit(1);
    }
    return file;
}

int min (int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

int max (int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

/**
 * @brief fills the processes array data from the csv file
 * @param path path string
 * @param p_array processes array
 * @returns number of processes
 */
int fill_processes_array(char * path, P p_array[]) {

    FILE * csv = get_csv_file(path);
    int i = 0;
    char c;
    while (fscanf(csv, "%[^,]%*c%[^,]%*c%d,%d,%d", p_array[i].Name, p_array[i].Desciption,
        &p_array[i].Arrival_Time, &p_array[i].Burst_Time, &p_array[i].Priority) != EOF) {
            p_array[i].Remaining_Time = p_array[i].Burst_Time;
            i++;
            fscanf(csv, "%c", &c);
        }
    return i;
}

/**
 * @brief blocks all types of signals
 */
void block_all_signal() {
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, NULL);
}

/* handler for signals */
void handler(int sig, siginfo_t *si, void *ucontext) {}

/**
 * @brief sets the alaram handler for SIGALRM and SIGUSR1
 */
void set_alarm_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
}

int time = 0; // global time

/**
 * @brief simulates idle time of the cpu
 */
void idle_burst(int burst_time) {
    alarm(burst_time); /* set alarm */
    pause(); /* wait for the alarm */

    sprintf(buf, "%d → %d: Idle.\n", time, burst_time + time);
    output_to_screen();
}

/**
 * @brief simulates a cpu burst
 * @param pid child proccess id
 * @param burst_time cpu burst time
 */
void burst(pid_t pid, int burst_time) {
    kill (pid, SIGCONT); /* continue the child process*/
    alarm(burst_time); /* set alarm */
    pause(); /* wait for the alarm */
    kill (pid, SIGSTOP); /* stop the child process */
}

/**
 * @brief simulates a cpu burst
 * @param p process instance
 */
void simulate_cpu_burst(P * p) {

    int Burst_Time;
    Burst_Time = p->Burst_Time;
    p->Remaining_Time -= Burst_Time;

    burst(p->pid, Burst_Time);
    sprintf(buf, "%d → %d: %s Running %s.\n", time, Burst_Time + time, p->Name, p->Desciption);
    output_to_screen();

    if (p->Remaining_Time == 0) { /* process finished running */
        kill(p->pid, SIGKILL);
        waitpid(p->pid, NULL, 0);
    }
}

/**
 * @brief simulates the preemptive algorithms: RR
 * @param p_array processes array
 * @param len length of p_array
 */
void run_preemptive(P p_array[], int len) {

    int finished = 0, i = 0, j, arrived = 1;

    while (finished != len) { /* on non preemptive proccess must finish before the next runs */
        
        if ((p_array[i].Arrival_Time - time) > 0) { /* check of idle time */
            idle_burst(p_array[i].Arrival_Time - time);
            time = p_array[i].Arrival_Time;;
        }

        if (p_array[i].Remaining_Time) {

            p_array[i].Burst_Time = min(p_array[i].Burst_Time, p_array[i].Remaining_Time);
            simulate_cpu_burst(&p_array[i]); /* simulate a burst */
            time += p_array[i].Burst_Time;

            if (p_array[i].Remaining_Time == 0) {
                finished++;
                i++;
                continue;
            }
        }
  
        j = i + 1;
        while (j < len && p_array[j++].Arrival_Time <= time) arrived++; /* updating number of processes arrived */
       
        i = (i + 1) % arrived;
    }
}
/**
 * @brief simulates the non preemptive algorithms: FCFS, Priority and SJF
 * @param p_array processes array
 * @param len p_array len
 * @returns average waiting time
 */
float run_non_preemptive(P p_array[], int len) {

    float waiting_time = 0;

    for (int i = 0; i < len; i++) {

        if ((p_array[i].Arrival_Time - time) > 0) { /* check of idle time */
            idle_burst(p_array[i].Arrival_Time - time);
            time = p_array[i].Arrival_Time;;
        }

        waiting_time += time - p_array[i].Arrival_Time;
  
        simulate_cpu_burst(&p_array[i]);
        time += p_array[i].Burst_Time;
    }

    return waiting_time / len;
}

int cmp_arrival(P a1, P a2) {
    return (a1.Arrival_Time <= a2.Arrival_Time);
}

int cmp_sortest_job(P a1, P a2) {
    return (a1.Burst_Time <= a2.Burst_Time);
}

int cmp_priotiry(P a1, P a2) {
    return (a1.Priority <= a2.Priority);
}

void merge(P arr[], int left, int mid, int right, int (*cmp)(P, P)) {

    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    P L[n1], R[n2];

    for (i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    i = 0; j = 0; k = left;

    while (i < n1 && j < n2) {

        if (cmp(L[i], R[j])) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1)
        arr[k++] = L[i++];
    while (j < n2)
        arr[k++] = R[j++];
}

void mergeSort(P arr[], int left, int right, int (*cmp)(P, P)) {

    if (left < right) {
        int mid = left + (right - left) / 2;

        // Sort first and second halves
        mergeSort(arr, left, mid, cmp);
        mergeSort(arr, mid + 1, right, cmp);

        // Merge the sorted halves
        merge(arr, left, mid, right, cmp);
    }
}

void sort(P p_array[], int len, sort_by sortby) {

    switch (sortby)
    {
    case ARRIVAL:
        mergeSort(p_array, 0, len - 1, cmp_arrival);
        break;
    case SHORTEST_JOB:
        mergeSort(p_array, 0, len - 1, cmp_sortest_job);
        break;
    case PRIORITY:
        mergeSort(p_array, 0, len - 1, cmp_priotiry);
        break;
    default:
        break;
    }
}

/**
 * @brief creates the child processes
 * @param p_array processes array
 * @param len p_array length
 */
void create_child_proccess(P p_array[], int len) {

    for (int i = 0; i < len; i++) {
        p_array[i].Remaining_Time = p_array[i].Burst_Time; /* set remaning time */

        pid_t pid = fork();

        if (pid != 0) {
            p_array[i].pid = pid;
            kill(pid, SIGSTOP); /* stop the child process */
        } else {
            while(1) {};
        }
    }
}

void print_openning(char * alg) {
    sprintf(buf, "══════════════════════════════════════════════\n\
>> Scheduler Mode : %s\n\
>> Engine Status  : Initialized\n\
──────────────────────────────────────────────\n\n", alg);
    output_to_screen();
}

void print_np_closing(double avg_WT) {

    sprintf(buf, "\n──────────────────────────────────────────────\n\
>> Engine Status  : Completed\n\
>> Summary        :\n\
   └─ Average Waiting Time : %.2f time units\n\
>> End of Report\n\
══════════════════════════════════════════════\n", avg_WT);
    output_to_screen();
}

void print_p_closing(int TAT) {

    sprintf(buf, "\n──────────────────────────────────────────────\n\
>> Engine Status  : Completed\n\
>> Summary        :\n\
   └─ Total Turnaround Time : %d time units\n\
\n\
>> End of Report\n\
══════════════════════════════════════════════\n", TAT);

    output_to_screen();
}

/**
 * @brief moving array item from one position to another, allowes procces that came first to run
 * @param p_array processes array
 * @param from from where to move
 * @param to where to move
 */
void bring_to_pos(P p_array[], int from, int to) {

    P temp;
    for (int i = from; i > to; i--) {
        temp = p_array[i];
        p_array[i] = p_array[i - 1];
        p_array[i - 1] = temp;
    }
}

/**
 * @brief sort the array by arrival time allowes procesess came first to run
 * @param p_array processes array
 * @param len p_array length
 */
void sort_by_prop(P p_array[], int len) {

    int time = 0, start, end, pos = 0;

    for (int i = 0; i < len; i++) {

        for (int j = i; j < len; j++) {
            if (p_array[j].Arrival_Time <= time) {
                time += p_array[j].Burst_Time;
                bring_to_pos(p_array, j, i);
                break;
            }
        }
    }
}

/**
 * @brief runs the FCFS
 */
void FCFS(P p_array[], int len) {

    print_openning("FCFS");
    /* sort the array by arrival time */
    sort(p_array, len, ARRIVAL);

    create_child_proccess(p_array, len);

    float avg_WT = run_non_preemptive(p_array, len);

    print_np_closing(avg_WT);
}

void SJF(P p_array[], int len) {
    
    time = 0;
    print_openning("SJF");

    /* sort the array */
    sort(p_array, len, SHORTEST_JOB);
    sort_by_prop(p_array, len);

    create_child_proccess(p_array, len);
    float avg_WT = run_non_preemptive(p_array, len);

    print_np_closing(avg_WT);
}

void PS(P p_array[], int len) {

    time = 0;
    print_openning("Priority");

    /* sort the array */
    sort(p_array, len, PRIORITY);
    sort_by_prop(p_array, len);

    create_child_proccess(p_array, len);
    float avg_WT = run_non_preemptive(p_array, len);

    print_np_closing(avg_WT);
}

void RR(P p_array[], int len, int time_quantum) {

    time = 0;
    sort(p_array, len, ARRIVAL);
    create_child_proccess(p_array, len);

    for (int i = 0; i < len; i++) {
        p_array[i].Burst_Time = time_quantum;
    }

    print_openning("Round Robin");
    run_preemptive(p_array, len);
    print_p_closing(time);
}

void copy_array(P p_array[], P copy[], int len) {
    for (int i = 0; i < len; i++) {
        copy[i] = p_array[i];
    }
}

/**
 * @brief the main function
 * @param processesCsvFilePath path to the csv file
 * @param time_quantum time unit for round robin
 */
void runCPUScheduler(char* processesCsvFilePath, int time_quantum) {

    P p_array[1001], copy[1001];
    int len;

    len = fill_processes_array(processesCsvFilePath, p_array);
    
    copy_array(p_array, copy, len);
    
    /* block all signals exept SIGALRM */
    block_all_signal();

    /* handle sig alarm */
    set_alarm_handler();
    
    FCFS(p_array, len);

    sprintf(buf, "\n");
    output_to_screen();
    
    SJF(p_array, len);
    sprintf(buf, "\n");
    output_to_screen();

    PS(p_array, len);
    sprintf(buf, "\n");
    output_to_screen();
    RR(copy, len, time_quantum);
}