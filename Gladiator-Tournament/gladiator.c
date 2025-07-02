#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define NUM_GLADIATORS 4

typedef struct {
    int health;
    int id;
    int attack;
    FILE * file;
} opponents;

typedef struct {
    int health;
    int attack;
    char * name;
    opponents opponents[NUM_GLADIATORS - 1];
    FILE * file;
    FILE * logfile;
} player;

player pl;

FILE * Fopen(char * name, char * perm) {

    FILE * file = fopen(name, perm);  

    if (file == NULL) {
        perror("failed to open file");
        exit(1);
    }
    return file;
}

int exrtact_id(char * fname) {
    return (int)(fname[1] - '0');
}

void set_opponents_info() {

    char fname[] = "G0.txt";
    
    for (int i = 0; i < NUM_GLADIATORS - 1; i++) {

        fname[1] += pl.opponents[i].id;
        pl.opponents[i].file = Fopen(fname, "r+");
        fscanf(pl.opponents[i].file, "%d, %d", &pl.opponents[i].health, &pl.opponents[i].attack);
        fname[1] = '0';
    }
}

void log_first_line(int id) {
    fprintf(pl.logfile, "Gladiator process started. %d:\n", id);
}

void set_player_info(char * name, char * fname) {

    pl.name = name;
    char log_fname[11];

    strcpy(log_fname, fname);
    strcat(log_fname, "_log.txt");
    strcat(fname, ".txt");

    pl.file = Fopen(fname, "r+");
    pl.logfile = Fopen(log_fname, "w+");

    fscanf(pl.file, "%d, %d, %d, %d, %d", &pl.health, &pl.attack,
        &pl.opponents[0].id, &pl.opponents[1].id, &pl.opponents[2].id);

    set_opponents_info();

    log_first_line(exrtact_id(fname));
}

void close_files() {
    
    for (int i = 0; i < NUM_GLADIATORS - 1; i++) {
        fclose(pl.opponents[i].file);
    }
    fclose(pl.file);
}

int main(int argc, char *argv[])
{
    set_player_info(argv[1], argv[2]); 

    while (pl.health > 0) {
        
        for (int i = 0; i < 3; i++) {
            
            int opponent_attack = pl.opponents[i].attack;
            int oppponent_id = pl.opponents[i].id;

            fprintf(pl.logfile, "Facing opponent %d... Taking %d damage\n", oppponent_id, opponent_attack);
            pl.health -= opponent_attack;

            if (pl.health > 0) {
                fprintf(pl.logfile, "Are you not entertained? Remaining health: %d\n", pl.health);
            } else {
                fprintf(pl.logfile, "The gladiator has fallen... Final health: %d\n", pl.health);
                close_files();
                exit(0);
            }
        }
    }
    return 0;
}

