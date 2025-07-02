#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

enum insturction_type {
    READ, WRITE, QUIT, INVALID
};

typedef struct {
    char type;
    int start_offset;
    int end_offset;
    char * text;
} instruction;

typedef struct {
    FILE * file;
    int fd;
    int file_size;
} file;

file dataf, resf, reqf;

FILE * Fopen(char * name, char * perm, char * err) {

    FILE * file = fopen(name, perm);  
    if (file == NULL) {
        perror(err);
        exit(1);
    }
    return file;
}

/**
 * @brief computes the file size in bytes
 * @return number of bytes.
 */
int get_file_size(int fd) {

    /* move to the end of file lseek returns the offset */
    int size = lseek(fd, 0, SEEK_END);

    /* go back to the beginning of the file */
    int p = lseek(fd, 0, SEEK_SET);
    return size;
}

/**
 * @brief assigns the text to a string array
 * @param int the instruction instance to assign the string to
 */
void assign_text_string(instruction * ins) {

    char c;
    int len = strlen(ins->text);
    int size = 0;
    
    fread(&c, sizeof(char), 1, reqf.file); /* read the first character */
    while (c != '\n') {

        size += 1;
        if (size < len) { /* allocate space only if needed */
            ins->text[size - 1] = c;
        } else {

            ins->text = (char *)realloc(ins->text, sizeof(char) * (size + 1));
            ins->text[size - 1] = c;
        }
        fread(&c, sizeof(char), 1, reqf.file); /* can the next character */
    }
    ins->text[size] = '\0';
}

/**
 * @brief skip the string on invalid instruction
 */
void skip_invalid_string() {
    char c;
    fread(&c, sizeof(char), 1, reqf.file);
    while (c != '\n') {
        fread(&c, sizeof(char), 1, reqf.file);
    }
}

/**
 * @brief validate the instruction
 * @param ins the instruction instance
 */
int is_instruction_valid(instruction ins) {

    if (ins.start_offset < 0) {
        return 0;
    }

    if (ins.type == READ) {
        if (dataf.file_size <= ins.end_offset) { /* read end offset is larger then file size */
            return 0;
        }
    } else {
        if (dataf.file_size < ins.start_offset) { /* read start offset is larger then file size */
            return 0;
        }
    }
    return 1;
}

/**
 * @brief get the next instruction
 * @param int instruction instance to fill with data 
 */
void fetch_next_instruction(instruction * ins) {
    
    /* get the type of the instruction */
    char type;
    fscanf(reqf.file, "%c ", &type);

    switch (type) {
        case 'R':
            /* decode */
            ins->type = READ;
            fscanf(reqf.file, "%d %d ", &ins->start_offset, &ins->end_offset);

            /* validation */
            if(!is_instruction_valid(*ins)) {
                ins->type = INVALID;
            }
            // skip_invalid_string();
            break;
        case 'W':
            /* decode */
            ins->type = WRITE;
            fscanf(reqf.file, "%d ", &ins->start_offset);

            /* validation */
            if(!is_instruction_valid(*ins)) {
                skip_invalid_string();
                ins->type = INVALID;
                break;
            }
            /* string assignment of instruction valid */
            assign_text_string(ins);
            break;
        case 'Q':
            ins->type = QUIT;
            break;

        default:
            break;
    }
}

/**
 * @brief executes the insturciotn
 * @param ins insturction instance to execute
 */
void execute_instruction(instruction ins) {

    int size;
    char * buf;
    if (ins.type == READ) {

        /* create a buffer */
        size = ins.end_offset - ins.start_offset + 1;
        buf = (char *)malloc(sizeof(char) * size);
  
        lseek(dataf.fd, ins.start_offset, SEEK_SET);

        // fseek(dataf.file, ins.start_offset, SEEK_SET);
        read(dataf.fd, buf, size);
        // fread(buf, sizeof(char), size, dataf.file);

        write(resf.fd, buf, size);
        // fwrite(buf, sizeof(char), size, resf.file);

        write(resf.fd, "\n", 1);
        // fwrite("\n", sizeof(char), 1, resf.file);

        free(buf);
    } else {
        
        /* get the reminder of the file */
        size = dataf.file_size - ins.start_offset;
        buf = (char *)malloc(sizeof(char) * size);
        int len = strlen(ins.text);
        dataf.file_size += len;

        lseek(dataf.fd, ins.start_offset, SEEK_SET);
        // fseek(dataf.file, (long) ins.start_offset, SEEK_SET);

        read(dataf.fd, buf, size);
        // fread(buf, sizeof(char), size, dataf.file);
        lseek(dataf.fd, ins.start_offset, SEEK_SET);
        // fseek(dataf.file, (long)ins.start_offset, SEEK_SET);

        write(dataf.fd, ins.text, len);
        // fwrite(ins.text, sizeof(char), len, dataf.file);

        write(dataf.fd, buf, size);
        // fwrite(buf, sizeof(char), size, dataf.file);

        free(buf);
    }
}

/**
 * @brief main function that runs the procedure
 */
void run() {

    instruction ins;
    ins.text = (char *)malloc(sizeof(char));
    ins.start_offset = 0;
    ins.end_offset = 0;

    /* fetch first instruction */
    fetch_next_instruction(&ins);
    
    /* run all while the instruction is not Q */
    while (ins.type != QUIT) {
        
        if (ins.type == INVALID) {
            fetch_next_instruction(&ins);
            continue;
        }

        /* execute */
        execute_instruction(ins);
        fetch_next_instruction(&ins);
    }
    free(ins.text);
}

void debug() {

    char * datap = "data.txt";
    char * reqp = "requests.txt";

    dataf.file = Fopen(datap, "r+", "data.txt");
    dataf.fd = fileno(dataf.file);
    dataf.file_size = get_file_size(dataf.fd);

    reqf.file =  Fopen(reqp, "r+", "requests.txt");
    reqf.fd = fileno(reqf.file);
    reqf.file_size = get_file_size(reqf.fd);
  
    resf.file = Fopen("read_results.txt", "w+", "failed to open file");
    resf.fd = fileno(resf.file);
    resf.file_size = get_file_size(resf.fd);

    run();

    /* close files */
    close(resf.fd);
    close(reqf.fd);
    close(dataf.fd);
}

/**
 * @brief Main
 * @param argv contains paths to data and requests txt files
 */
int main(int argc, char *argv[]) {

    char * datap = argv[1];
    char * reqp = argv[2];

    dataf.file = Fopen(datap, "r+", "data.txt");
    dataf.fd = fileno(dataf.file); /* get file descriptor */
    dataf.file_size = get_file_size(dataf.fd);

    reqf.file =  Fopen(reqp, "r+", "requests.txt");
    reqf.fd = fileno(reqf.file);
    reqf.file_size = get_file_size(reqf.fd);
  
    resf.file = Fopen("read_results.txt", "w+", "failed to open file");
    resf.fd = fileno(resf.file);
    resf.file_size = get_file_size(resf.fd);

    run();

    /* close files */
    close(resf.fd);
    close(reqf.fd);
    close(dataf.fd);

    return 0;
}
