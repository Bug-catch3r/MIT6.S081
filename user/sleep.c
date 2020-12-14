// lab1 sleep

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        int time = atoi(argv[1]);
        if (time < 0)
        {
            fprintf(2, "Arg must larger than 0\n");
            exit(0);
        }
        else
        {
            printf("sleep for a little while.\n");
            sleep(time);
        }
    }
    else if (argc == 1)
        fprintf(2, "Error, too less arg to call sleep\n");
    exit(0);
}