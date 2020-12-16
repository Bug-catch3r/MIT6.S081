## 1.Sleep

这个直接用系统调用(实现在kernel/proc.c中)，再按要求打印错误信息即可，然后添加变量到Makefile中的UPROGS，之后不再赘述

```cpp
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDDER_FILENO 2

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(STDDER_FILENO, "usage: sleep <number>\n");
        exit(1);
    }
    int number = atoi(argv[1]);
    sleep(number);
    exit(0);
}
```

## 2.Pingpong

添加两个管道，管道一父进程写子进程读，管道二反之，记得写完关闭关闭父进程/子进程的写端，防止读端收不到EOF信号一直阻塞

```cpp
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDDER_FILENO 2

#define READEND 0
#define WRITEEND 1

typedef int pid_t;

int main(void)
{
    // build two pipe
    int pfd[2];
    int cfd[2];

    char buf[10];
    pid_t pid;
    
    pipe(pfd);
    pipe(cfd);

    if ((pid = fork()) < 0)
    {
        fprintf(STDDER_FILENO, "fork error\n");
        exit(1);
    }
    else if (pid == 0) // child process
    {
        close(pfd[WRITEEND]);
        close(cfd[READEND]);
        read(pfd[READEND], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        write(cfd[WRITEEND], "pong", 4);
        close(cfd[WRITEEND]);
    }
    else // parent process
    {
        close(pfd[READEND]);
        close(cfd[WRITEEND]);
        write(pfd[WRITEEND], "ping", 4);
        close(pfd[WRITEEND]);
        read(cfd[READEND], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
    }

    exit(0);
}
```

## 3.Primes

题意为用并发编程的方式实现埃拉托斯特尼素数筛，先理一下思路，大概框架如下:

```cpp
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define STDDER_FILENO 2

#define READEND 0
#define WRITEEND 1

typedef int pid_t;

int main(void)
{
    int numbers[36], fd[2];
    int i, index = 0;
    pid_t pid;
    for (i = 2; i <= 35; i++)
    {
        numbers[index++] = i;
    }
    while (index > 0)
    {
        pipe(fd);
        if ((pid = fork()) < 0) 
        {
            fprintf(STDDER_FILENO, "fork error\n");
            exit(0);
        }
        else if (pid > 0)
        {
            close(fd[READEND]);
            for (i = 0; i < index; i++)
            {
                write(fd[WRITEEND], &numbers[i], sizeof(numbers[i]));
            }
            close(fd[WRITEEND]);
            wait((int *)0);
            exit(0);
        }
        else 
        {
            close(fd[WRITEEND]);
            int prime = 0;
            int temp = 0;
            index = -1;
            
            while (read(fd[READEND], &temp, sizeof(temp)) != 0)
            {
                // the first number must be prime
                if (index < 0) prime = temp, index ++;
                else
                {
                    if (temp % prime != 0) numbers[index++] = temp;
                }
            }
            printf("prime %d\n", prime);
            // fork again until no prime
            close(fd[READEND]);
        }
    }
    exit(0);
}
```

## 4.Find

实现一个find函数的功能，指导书推荐我们看一下**user/ls.c**的实现，大部分代码和**ls.c**是类似的，首先先清楚一下fstat的功能，输入一个文件描述符得到相关信息，xv6中定义的文件信息结构

```cpp
struct stat {
  int dev;     // 文件系统设备号
  uint ino;    // Inode 值
  short type;  // 文件类型
  short nlink; // 文件被链接数
  uint64 size; // 文件大小
};
```

这其中有用的是type，它可以查看是普通文件还是目录文件，如果是普通文件就查看文件名是否对应一样，目录则遍历其目录项递归查找，但要注意递归的时候跳过“.”（本目录）和“..”（上级目录）两个特殊文件名防止死循环，下面是代码实现

```cpp
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define STDDER_FILENO 2
#define O_RDONLY 0

char* fmtname(char * path)
{
    static char buf[DIRSIZ+1];
    char *p;
    
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p ++;
    if (strlen(p) >= DIRSIZ) return p;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;
    return buf;
}

void find(char* path, char *name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(STDDER_FILENO, "find open %s error\n", path);
        exit(1);
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(STDDER_FILENO, "fstat %s error\n", path);
        close(fd);
        exit(1);
    }

    switch (st.type)
    {
    case T_FILE: // if file check the name
        if (strcmp(fmtname(path), name) == 0) printf("%s\n", path);
        break;
    
    case T_DIR: // if directory recursive call find
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
        {
            fprintf(STDDER_FILENO, "find: path too long\n");
            break;
        }

        // add '/'
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            // inum == 0 means invalid directory entry
            if (de.inum == 0) continue;

            // add de.name to path
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            
            // don't find . and ..
            if (!strcmp(de.name, ".") || !strcmp(de.name, "..")) continue;
            
            // recursive call find
            find(buf, name);
        }
        break;
    }
    close(fd);
}


int main(int argc, char *argv[])
{
    if (argc != 3) 
    {
        fprintf(STDDER_FILENO, "usage: find <path> <name>\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
```

## 5.Xargs

首先就是先明确这个函数的[用法](https://link.zhihu.com/?target=http%3A//www.ruanyifeng.com/blog/2019/08/xargs-tutorial.html)，然后就是处理标准输入放入参数数组，**fork**子进程**exec**

```cpp
#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define STDIN_FILENO 0
#define MAXLINE 1024

int main(int argc, char *argv[])
{
    char line[MAXLINE];
    char* params[MAXARG];
    int n, args_index = 0;
    int i;

    char* cmd = argv[1];
    for (i = 1; i < argc; i++) params[args_index++] = argv[i];

    while ((n = read(STDIN_FILENO, line, MAXLINE)) > 0)
    {
        if (fork() == 0) // child process
        {
            char *arg = (char*) malloc(sizeof(line));
            int index = 0;
            for (i = 0; i < n; i++)
            {
                if (line[i] == ' ' || line[i] == '\n')
                {
                    arg[index] = 0;
                    params[args_index++] = arg;
                    index = 0;
                    arg = (char*) malloc(sizeof(line));
                }
                else arg[index++] = line[i];
            }
            arg[index] = 0;
            params[args_index] = 0;
            exec(cmd, params);
        }
        else wait((int*)0);
    }
    exit(0);
}
```

## 实验结果

```bash
$ ./grade-lab-util
make: 'kernel/kernel' is up to date.
== Test sleep, no arguments == sleep, no arguments: OK (1.9s)
== Test sleep, returns == sleep, returns: OK (0.8s)
== Test sleep, makes syscall == sleep, makes syscall: OK (1.1s)
== Test pingpong == pingpong: OK (1.0s)
== Test primes == primes: OK (1.0s)
== Test find, in current directory == find, in current directory: OK (1.1s)
== Test find, recursive == find, recursive: OK (1.2s)
== Test xargs == xargs: OK (1.0s)
== Test time ==
time: OK
Score: 100/100
```
