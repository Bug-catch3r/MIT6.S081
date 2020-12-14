
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char args[MAXARG][MAXARG];
    int i = 0, j = 0;
    char ch;
    //read single char for each loop
    while( read(0, &ch, 1) && (ch != '\0'))
    {
        if(ch == '\n')
        {
            ch = '\0';
            args[i][j] = ch;    
            i++;
            j = 0;
        }
        else 
        {
            args[i][j++] = ch;
        }
    }
    args[i++][j] = '\0';

    for (j = 0; j < i; ++j)
    {
        // printf("%s\n", args[j]);
        char *arg[3];
        arg[0] = argv[1];
        arg[1] = argv[2];
        arg[2] = args[j];
        if (fork())
            wait(0);
        else
        {
            exec("grep", arg);
            exit(0);
        }
    }
    exit(0);
}