// lab1 sleep
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int pipe1[2], pipe2[2]; //for fater, pipe1 for write, pipe2 for read, opposite for child.
    char buffer[] = " ";    //one byte
    int ret1 = pipe(pipe1), ret2 = pipe(pipe2);

    if (ret1 == -1 || ret2 == -1)
    {
        fprintf(2, "Cann't create pipe\n");
        exit(1);
    }
    int pid = fork();
    if (pid > 0) //father thread
    {
        printf("fater\n");
        close(pipe1[0]);
        close(pipe2[1]);

        write(pipe1[1], buffer, 1);
        close(pipe1[1]);
        read(pipe2[0], buffer, 1);
        printf("%d: received pong\n", getpid());
        close(pipe2[0]);
    }
    else if (!pid) //child thread
    {
        printf("child\n");
        close(pipe1[1]);
        close(pipe2[0]);

        read(pipe1[0], buffer, 1);
        close(pipe1[0]);
        printf("%d: received ping\n", getpid());
        write(pipe2[1], buffer, 1);
        close(pipe2[1]);
    }
    else
    {
        fprintf(2, "Pid error\n");
        exit(1);
    }
    exit(0);
}