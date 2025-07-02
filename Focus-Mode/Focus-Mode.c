#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void print(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

void print_menu() {
    print("\nSimulate a distraction:\n\
  1 = Email notification\n\
  2 = Reminder to pick up delivery\n\
  3 = Doorbell Ringing\n\
  q = Quit\n\
>> ");
}

void print_top(int round) {
    char buf[326];
    sprintf(buf, "══════════════════════════════════════════════\n\
                Focus Round %d                \n\
──────────────────────────────────────────────\n", round);

    print(buf);
}

void print_buttom() {

    print("──────────────────────────────────────────────\n\
             Back to Focus Mode.              \n\
══════════════════════════════════════════════\n");
}

void print_middle() {
    print("──────────────────────────────────────────────\n\
        Checking pending distractions...      \n\
──────────────────────────────────────────────\n");
}

/**
 * @brief gets the user input
 * @returns the input as Integer
 */
int read_user_input() {
    char c;
    read(STDIN_FILENO, &c, sizeof(char));

    if (c == '\n') {
        read(STDIN_FILENO, &c, sizeof(char)); 
    }

    if (c == 'q') {
        return 0;
    }
    return c - '0';
}

/**
 * @brief gets the intrupts from the user as input
 * @param duration number of interrupts
 */
void get_user_interruputs(int duration) {
    
    /* run for number of interrupts */
    while (duration--) {
        
        print_menu();
        
        /* read user input */
        switch (read_user_input()) {
            case 1:
                raise(SIGUSR1);
                break;
            case 2:
                raise(SIGUSR2);
                break;
            case 3:
                raise(SIGINT);
                break;
            case 0: /* q was pressed */
                return;
            default:
                break;
        }
    }
}

/**
 * @brief clears the panding signals
 * @param mask mask object
 */
void clear_pending(sigset_t * mask) {
    sigprocmask(SIG_UNBLOCK, mask, NULL); /* clear the pending signals */
}

/**
 * @brief handles the pending interrupts sent by the user.
 */
void handle_panding_interrupts() {

    const char *str;
    print_middle();
    sigset_t pending;
    sigpending(&pending);
    int distracted = 0;

    /* check interrupt type */
    if (sigismember(&pending, SIGUSR1)) {
        distracted = 1;
        print(" - Email notification is waiting.\n");
        print("[Outcome:] The TA announced: Everyone get 100 on the exercise!\n");
    }

    if (sigismember(&pending, SIGUSR2)) {
        distracted = 1;
        print(" - You have a reminder to pick up your delivery.\n");
        print("[Outcome:] You picked it up just in time.\n");
    }

    if (sigismember(&pending, SIGINT)) {
        distracted = 1;
        print(" - The doorbell is ringing.\n");
        print("[Outcome:] Food delivery is here.\n");

    }

    /* no interrupts was made */
    if (! distracted) {
        print("No distractions reached you this round.\n");
    }
    print_buttom();
}

/**
 * @brief main function
 * @param numOfRounds number of focus mode rounds
 * @param duration number of interrupts on each round
 */
void runFocusMode(int numOfRounds, int duration) {

    int round = 1;

    /* block the interrupts */
    struct sigaction sa;
    sigset_t mask, old;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);
    sa.sa_mask = mask;
    
    print("Entering Focus Mode. All distractions are blocked.\n");

    while (round <= numOfRounds) {
        sigprocmask(SIG_BLOCK, &mask, &old); /* block the signals */
        print_top(round++);
        get_user_interruputs(duration); /* get user input */
        handle_panding_interrupts();
        clear_pending(&mask);
    }

    print("\nFocus Mode complete. All distractions are now unblocked.");
}