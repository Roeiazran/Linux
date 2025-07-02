#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 1024
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
    {
        perror("fork failed");
    }

    return pid;
}

void Execvp(char * args[]) {

    int st = execvp(args[0], args);

    if (st == -1) {
        perror("execvp failed");
        exit(1);
    }
}

char * Getcwd(char * buff, int len) {
    
    char * path = getcwd(buff, len);

    if (!path) {
        perror("getcwd failed");
        exit(1);
    }

    return path;
}

int Chdir(char path[]) {

    int st = chdir(path);
    return st;
}

char * extractDirName(char path[]) {
    
    int len = strlen(path);
    int lastIndex = 0;

    for (int i = 0; i < len; i++) {
        if (path[i] == '/') {
            lastIndex = i;
        }
    }
    
    return (path + lastIndex + 1);
}

int getLastSlashIndex(char path[]) {

    int len = strlen(path);
    char * copy = (char*)malloc(sizeof(char) * len);

    int lastIndex = 0;

    for (int i = 0; i < len; i++) {
        if (path[i] == '/') {
            lastIndex = i;
        }
    }

    return lastIndex;
}

void copyFile(char * srcPath, char *dstPath) {

    pid_t pid = Fork();

    if (!pid) {
        char * args[] = {"cp", srcPath, dstPath, NULL};
        Execvp(args);
    } else {
        waitpid(-1, NULL, 0);
        printf("Copied: %s -> %s\n", srcPath, dstPath);
    }
}

void Makedir(char dirName[], char * permissions) {

    pid_t pid = Fork();

    if (!pid) {
        char * args[] = {"mkdir",  "-m", permissions,"-p", dirName, NULL};
        Execvp(args);
    } else {
        waitpid(-1, NULL, 0);
    }
}

int cmp(char *str1, char *str2) {

    while ((*str1 != 0) && (*str2 != 0)) {

        if (*str1 > *str2) {
            return 1;
        } else if (*str1 < *str2) {
            return 2;
        }
        str1++;
        str2++;
    }

    if (*str1 == *str2) {
        return 0;
    }

    return ((*str1 - *str2) > 0) ? 1 : 2;
}

void sort(char * arr[], int len) {

    int c;
    for (int i = 0; i < len; i++) {
        for (int j = i + 1; j < len; j++) {
            int c = cmp(arr[i], arr[j]);
            if (c == 1) {
                char * t = arr[i];
                arr[i] = arr[j];
                arr[j] = t;
            }
        }
    }
}

int getFilesCount(char * path) {

    DIR * dirp = opendir(path);
    struct dirent *dptr;
    int len = 0;

    while ((dptr = readdir(dirp))) {
        if (dptr->d_type == 8) {
            len++;
        }
    }
    return len;
}

char ** getSortedFilesNames(char * path, int filesCount) {

    DIR * dirp = opendir(path);
    struct dirent *dptr;
    int index = 0;
    char ** names = (char **)malloc(sizeof(char*) * filesCount);
    for (int i = 0; i < filesCount; i++) {
        names[i] = (char *)malloc(sizeof(char) * MAX_NAME_LEN);
    }

    while ((dptr = readdir(dirp))) {
        if (dptr->d_type == 8) {
            names[index++] = dptr->d_name;
        }
    }

    sort(names, filesCount);
    return names;
}

char * concatFileNameToPath(char * path, char * name) {

    char * res = (char *)calloc(sizeof(char) * (MAX_NAME_LEN + MAX_PATH_LEN), sizeof(char));

    strcat(res, path);
    strcat(res, "/");
    strcat(res, name);
    return res;
}

int isDiff(char * srcPath, char * dstPath) {

    int pid = Fork();
    int status;
    if (!pid) {
        int null_fd = open("/dev/null", O_WRONLY);

        if (null_fd == -1) {
            perror("failed to open");
            exit(1);
        }

        dup2(null_fd, STDOUT_FILENO);
        if (close(null_fd) == -1) {
            perror("failed to close");
            exit(1);
        }

        execl("/usr/bin/diff", "diff", "-q", srcPath, dstPath, NULL);
    } else {
        waitpid(-1, &status, 0);

        if (WEXITSTATUS(status) == 2) {
            perror("failed to diff");
            exit(1);
        }
    }
    
    return WEXITSTATUS(status);
}

int isEarlier(char * srcPath, char * dstPath) {

    struct stat srcSt;
    struct stat dstSt;
    char time[50];
    stat(srcPath, &srcSt);
    stat(dstPath, &dstSt);

    return (srcSt.st_ctime > dstSt.st_ctime);
}

void syncronize(char * src, char * dst) {

    int srcFilesCount = getFilesCount(src);
    char ** srcNames = getSortedFilesNames(src, srcFilesCount);

    int dstFilesCount = getFilesCount(dst);
    char ** dstNames = getSortedFilesNames(dst, dstFilesCount);
    
    char * srcFullPath;
    char * dstFullPath;

    /* check for empty dst directory */
    if (dstFilesCount == 0) {
        for (int i = 0; i < srcFilesCount; i++) {
            srcFullPath = concatFileNameToPath(src, srcNames[i]);
            dstFullPath = concatFileNameToPath(dst, srcNames[i]);

            printf("New file found: %s\n", srcNames[i]);

            copyFile(srcFullPath, dstFullPath);
            free(srcFullPath);
            free(dstFullPath);
        }
        return;
    } else if (srcFilesCount == 0) {
        return;
    }

    int res;
    for (int i = 0, dsti = 0; i < srcFilesCount; i++) {

        if (dsti == dstFilesCount) { // reached end of dst files list
            res = 2; // no match for any file
        } else {
            res = cmp(dstNames[dsti], srcNames[i]); // look for a file with the same name
        }
        
        while ((res == 2) && (dsti < dstFilesCount)) { 
            res = cmp(dstNames[dsti++], srcNames[i]);
        }

        srcFullPath = concatFileNameToPath(src, srcNames[i]);
        dstFullPath = concatFileNameToPath(dst, srcNames[i]);

        if (res != 0) { // not found a file with name srcNames[i]
            printf("New file found: %s\n", srcNames[i]);
            copyFile(srcFullPath, dstFullPath);
        } else {
            if (isDiff(srcFullPath, dstFullPath)) { // found diff between files

                if(isEarlier(srcFullPath, dstFullPath)) { // the src file was change later
                    printf("File %s is newer in source. Updating...\n", srcNames[i]);
                    copyFile(srcFullPath, dstFullPath);
                } else {
                    printf("File %s is newer in destination. Skipping..\n", srcNames[i]);
                }
            } else {
                printf("File %s is identical. Skipping...\n", srcNames[i]);
            }
        }
        free(srcFullPath);
        free(dstFullPath);
    }
}

int main(int argc, char *argv[])
{
    char srcPath[MAX_PATH_LEN];
    char dstPath[MAX_PATH_LEN];
    char curr[MAX_PATH_LEN];
    Getcwd(curr, MAX_PATH_LEN);
    Chdir(curr);
    printf("Current working directory: %s\n", curr);
    
    if (argc < 3) {
        printf("Usage: file_sync <source_directory> <destination_directory>\n");
        exit(1);
    }

    if (Chdir(argv[1])) {
        char * dirName = extractDirName(argv[1]);
        printf("Error: Source directory '%s' does not exist.\n", dirName);
        exit(1);
    }
    Getcwd(srcPath, MAX_PATH_LEN);

    Chdir(curr);
    if (Chdir(argv[2])) {

        /* creating the directory at given destination */
        int lastIndex = getLastSlashIndex(argv[2]);
        
        char dirName[MAX_PATH_LEN];

        if (lastIndex != 0) {
            strcpy(dirName, argv[2] + lastIndex + 1);

            // argv[2][lastIndex] = '\0'; // remove directory name from destination path
            // Chdir(argv[2]);
            // Makedir(dirName, "0700");
            // argv[2][lastIndex] = '/'; // restore destination path
        } else {
            strcpy(dirName, argv[2] + lastIndex);  
            // Makedir(dirName, "0700");
        }

        Makedir(argv[2], "0700");

        printf("Created destination directory '%s'.\n", dirName);
        Chdir(argv[2]);
    }

    Getcwd(dstPath, MAX_PATH_LEN);

    printf("Synchronizing from %s to %s\n",srcPath, dstPath);

    syncronize(srcPath, dstPath);

    printf("Synchronization complete.\n");
    return 0;
}
