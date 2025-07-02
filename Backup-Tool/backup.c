#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#define MAX_PATH_LENGTH 1024

/**
 * @brief Forks a process
 * @return 0 uppon successful completion
 */
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
    {
        perror("fork failed");
    }
    return pid;
}

/**
 * @brief Executes the execvp command
 * @param args Arguments array that forms the command
 */
void Execvp(char * args[]) {

    int st = execvp(args[0], args);

    if (st == -1) {
        perror("execvp failed");
        exit(1);
    }
}

/**
 * @brief Creates a directory
 * @param path The complete path on which the directory will be created
 * @param perm The permissions of the created directory
 */
void Makedir(char * path, char * perm) {

    pid_t pid = Fork();
    if (!pid) {
        char * args[] = {"mkdir", "-m", perm, "-p", path, NULL};
        Execvp(args);
    } else {
        waitpid(-1, NULL, 0);
    }
}

void Stat(char * path, struct stat * s) {

    if (lstat(path, s) == -1) {
        perror("stat");
        exit(1);
    }
}

/**
 * @brief Checks if a file exists in the file system
 * @param filepath The path to the file
 * @return 1 if the file exists and 0 otherwise
 */
int file_exists(char * filepath) {
    return !access(filepath, F_OK);
}

/**
 * @brief Checks if a given path points to a directory
 * @param path The path to be checked
 */
int is_directory(char * path) {

    struct stat s;
    if (stat(path, &s) != 0) {
        return 0;
    }
    return S_ISDIR(s.st_mode);
}

/**
 * @brief Concatenates one string to another
 * @param s1 The string to be placed first
 * @param s2 The string to be places last
 * @return Pointer to a string s1#s2
 */
char * concat_str(char * s1, char * s2) {

    int len = strlen(s1) + strlen(s2) + 2;
    char * res = (char *) malloc(sizeof(char) * len);
    strcpy(res, s1);
    strcat(res, "/");
    strcat(res, s2);
    return res;
}

/**
 * @brief Turns stat object mode of a directory to the form of the directory permissions
 * @param perm Pointer to array that will be modified to contain the permissions
 * @param mode Stat object contains the permissions
 */
void get_dir_permissions(char * perm, mode_t mode) {

    int pm = 0;
    pm |= ((mode & S_IRUSR) != 0) << 2;
    pm |= ((mode & S_IWUSR) != 0) << 1;
    pm |= ((mode & S_IXUSR) != 0);

    perm[0] = pm + '0';
    pm = 0;

    pm |= ((mode & S_IRGRP) != 0) << 2;
    pm |= ((mode & S_IWGRP) != 0) << 1;
    pm |= ((mode & S_IXGRP) != 0);
    
    perm[1] = pm + '0';
    pm = 0;

    pm |= ((mode & S_IROTH) != 0) << 2;
    pm |= ((mode & S_IWOTH) != 0) << 1;
    pm |= ((mode & S_IXOTH) != 0);
    
    perm[2] = pm + '0';
    perm[3] = '\0';
}

/**
 * @brief Creates the directory pointed by a source path in the destination path 
 * @param srcp The source path
 * @param dstp The destination path
 */
void create_dir(char * srcp, char * dstp) {

    struct stat s;
    Stat(srcp, &s);

    /* get the directory permissions and create it */
    char perm[4];
    get_dir_permissions(perm, s.st_mode);
    Makedir(dstp, perm);
}

/**
 * @brief Creates hard link between a source and a destination path
 * @param srcp Path to the source file
 * @param dstp Path to the destination
 */
void create_hard_link(char * srcp, char * dstp) {
    link(srcp, dstp);
}

/**
 * @brief Creates the hard links and the directory resides in a source path in the destination path.
 * Recursivly check directories and subdirectories and the files in them. 
 * @param src Source path to copy from the directories structure from
 * @param dst Destination path to the directories
 */
void create_hard_links_and_directories(char * src, char * dst) {

    DIR * dirp = opendir(src);
    struct dirent *dptr;
    struct stat s;
    char * csrc, * cdst;

    /* skip . and .. dirs */
    dptr = readdir(dirp);
    dptr = readdir(dirp);

    while ((dptr = readdir(dirp))) { /* loop through all files in the current directory */

        /* concate the current file name to the path to check its type */
        csrc = concat_str(src, dptr->d_name);
        cdst = concat_str(dst, dptr->d_name);
        Stat(csrc, &s);

        if (S_ISREG(s.st_mode)) {  /* this is a regular file */

            create_hard_link(csrc, cdst);  
        } else if (S_ISDIR(s.st_mode)) { /* this is a directory */
    
            /* create the directory and continue recursivly into it */
            create_dir(csrc, cdst);
            create_hard_links_and_directories(csrc, cdst);
        }
        free(cdst);
        free(csrc);
    }
    closedir(dirp);
}

/**
 * @brief Retrieves the last slash index of a given path
 * @param str Path string
 * @param len The length of str
 * @return The index found to be the last occurence of the char '/' in the path
 */
int get_last_slash_index(char * str, int len) {

    int i, j = len;
    
    for(i = len - 1; i >= 0; i--) {
        if (str[i] == '/') {
            return i;
        }
    }
    return i + 1;
}

/**
 * @brief Retrieve the abolute path to the file pointed by a symbolic link.
 * Works exactly like realpath
 * @param symbolic_path Path to the symbolic file
 * @param rel_path Redirection of the symbolic link to the file
 * @return String of the absolute path
 */
char * get_pointed_file_path(char * symbolic_path, char * rel_path) {

    char * buff = (char *)malloc(sizeof(char) * MAX_PATH_LENGTH);
    char curr_path[MAX_PATH_LENGTH];

    /* store the current path */
    getcwd(curr_path, MAX_PATH_LENGTH);

    /* get to the path of the file pointed by the symbolic link */
    chdir(symbolic_path);
    chdir(rel_path);

    /* store the path in buf and return the the previous path */
    getcwd(buff, MAX_PATH_LENGTH);
    chdir(curr_path);
    return buff;
}

/**
 * @brief Creates soft link at the destination path to a redirecion path
 * @param redl Relative path from the destination directory
 * @param dstp Path to the directory to create the link
 * @param fname String having the symbol file name
 */
void create_soft_link(char * redl, char * dstp, char * fname) {

    char cur[MAX_PATH_LENGTH];
    getcwd(cur, MAX_PATH_LENGTH);

    /* create the symbolic link in the destination under the given name*/
    chdir(dstp);
    symlink(redl, fname);

    chdir(cur);
}

/**
 * @brief 
 */
void create_links_recursivly(char * src_file_path, char * dst_file_path) {
    char buf[MAX_PATH_LENGTH];
    int len = readlink(src_file_path, buf, MAX_PATH_LENGTH);
    
    /* removing the file name from the redirection */
    int bls = get_last_slash_index(buf, len);
    char prevbuf = buf[bls];
    buf[len] = '\0'; /* readlink does not null termineting the string */
    buf[bls] = '\0';

    /* removing the file name from the file path */
    int slc = get_last_slash_index(src_file_path, strlen(src_file_path));
    src_file_path[slc] = '\0';
    int dls = get_last_slash_index(dst_file_path, strlen(dst_file_path));
    dst_file_path[dls] = '\0';

    /* get the pointed file path and change the dst accoridnly */
    char * src_file_next = get_pointed_file_path(src_file_path, buf);
    char * dst_file_next = get_pointed_file_path(dst_file_path, buf);

    /* restore buf file name and append it to the next path */
    buf[bls] = prevbuf;
    if (bls == 0) { /* no slash in redirection path */
        strcat(src_file_next, "/");
        strcat(src_file_next, buf);
        strcat(dst_file_next, "/");
        strcat(dst_file_next, buf);
    } else {
        strcat(src_file_next, buf + bls);
        strcat(dst_file_next, buf + bls);
    }

     /* check if the file exists and stop the recursion */
    if (!file_exists(dst_file_next)) {
        create_links_recursivly(src_file_next, dst_file_next);
    } 

    /* create the soft link */
    create_soft_link(buf, dst_file_path, dst_file_path + dls + 1);

    free(src_file_next);
    free(dst_file_next);
    return;
}

/**
 * @brief Creates the soft links resides in a source directory at a destination directory
 * @param src Source path 
 * @param dst Destination path
 */
void create_soft_links(char *src, char * dst) {

    DIR * dirp = opendir(src);
    struct dirent *dptr;
    struct stat s;
    char * csrc, * cdst;

    /* skip . and .. dirs */
    dptr = readdir(dirp);
    dptr = readdir(dirp);

    while ((dptr = readdir(dirp))) {

        csrc = concat_str(src, dptr->d_name);
        cdst = concat_str(dst, dptr->d_name);
        Stat(csrc, &s);

        if (S_ISLNK(s.st_mode)) { /* the file is a symbilic link */

            if (file_exists(cdst)) { /* the file already was created in previous recurse*/
                continue;
            }
            create_links_recursivly(csrc, cdst);
        } else if (S_ISDIR(s.st_mode)) { /* recurse inside subdirectories */
            create_soft_links(csrc, cdst);
        }
        free(cdst);
        free(csrc);
    }
    closedir(dirp);
}

int debug() {
 
    if (!is_directory("../text")) { /* is srcdir not exists */

        perror("src dir");
        exit(1);
    }
    
    if (is_directory("./t")) { /* is dstdir exists */

        perror("backup dir");
        exit(1);
    } else { /* create the dst directory */

        create_dir("../text", "./t");
    }

    create_hard_links_and_directories("../text", "./t");
    create_soft_links("../text", "./t");
    return 0;
}

int main(int argc, char *argv[])
{
    if (!is_directory(argv[1])) { /* is srcdir not exists */

        perror("src dir");
        exit(1);
    }
    
    if (is_directory(argv[2])) { /* is dstdir exists */

        perror("backup dir");
        exit(1);
    } else { /* create the dst directory */

        create_dir(argv[1], argv[2]);
    }

    create_hard_links_and_directories(argv[1], argv[2]);
    create_soft_links(argv[1], argv[2]);
    return 0;
}