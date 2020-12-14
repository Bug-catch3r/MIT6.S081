//find.c
// use pre order travasel to recursively visit direcotry
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    fd = open(path, 0);

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) //buffer size should sufficient to ensure dir entry name to add
        printf("ls: path too long\n");
    strcpy(buf, path); //use buffer to store path
    p = buf + strlen(buf);
    *p++ = '/'; //add '/' at the end of path
    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0 || !strcmp(".", de.name) || !strcmp("..", de.name))
            continue;
        memmove(p, de.name, DIRSIZ); //add file/dir name behind '/'
        p[DIRSIZ] = 0;
        
        if (stat(buf, &st) < 0) //use new path to get struct stat
        {
            printf("ls: cannot stat %s\n", buf);
            continue;
        }
        if (!strcmp(name, de.name))
            printf("%s/%s\n", path, name);
        else if (st.type == T_DIR)
            find(buf, name);
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    // printf("%s %s\n", argv[1], argv[2]);
    find(argv[1], argv[2]);
    exit(0);
}