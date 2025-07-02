#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>

#define NUM_GLADIATORS 4

pid_t Fork(void) {
    pid_t pid;

    if ((pid = fork()) < 0) {
        perror("Fork failed");
        exit(1);
    }
    return pid;
}

void Execvp(char * args[]) {
    
    execvp(args[0], args);
    perror("failed to exec");
    exit(1);
}

int get_index_from_pid(pid_t pid, pid_t gladiators[]) {
    
    for (int i = 0; i < NUM_GLADIATORS; i++) {
        if (gladiators[i] == pid)
            return i;
    }
    return -1; //error
}

int get_winner_index(int winners[]) {

    for (int i = 0; i < NUM_GLADIATORS; i++) {
        if (winners[i]) {
            return i;
        }
    }
    return -1;
}

int main(int argc, char const *argv[])
{
    char* gladiator_names[NUM_GLADIATORS] = {"Maximus", "Lucius", "Commodus", "Spartacus"};
    char* gladiator_files[NUM_GLADIATORS] = {"G1", "G2", "G3", "G4"};

    pid_t gladiators[NUM_GLADIATORS];

    int finish_count = 0, retpid, status;
    int winners[] = {1, 1, 1 ,1};

    for (int i = 0; i < NUM_GLADIATORS; i++) {
        if ((gladiators[i] = Fork()) == 0) { /* child process */

            char *args[] = {"./gladiator" , gladiator_names[i], gladiator_files[i], NULL};
            execvp(args[0], args);
        }
    }

    while ((retpid = waitpid(-1, &status, 0)) > 0) {
        finish_count++;

        if (finish_count == NUM_GLADIATORS) {
            /* if all the proccess finished declare the winner */
            int winnerIndex = get_winner_index(winners);
            printf("The gods have spoken, the winner of the tournament is %s!", gladiator_names[winnerIndex]);
        } else {
            /* else update loosing gladiator */
            int index = get_index_from_pid(retpid, gladiators);
            winners[index] = 0;
        }
    }
    return 0;
}
