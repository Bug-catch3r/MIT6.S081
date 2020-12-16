## overview

本次实验旨在让同学们掌握添加系统调用的流程。涉及的两个文件`trace.c`和`sysinfotest.c`均已存在且不需要修改。我们需要在kernel中添加一些语句来让这两个系统调用work起来。

## Trace

总体不是很难，但是改的地方比较多:

- 将`$ U / _trace`添加到Makefile中的UPROGS

- 将系统调用的原型添加到`user / user.h`，将`user`存根添加到`user / usys.pl`，以及`kernel / syscall.h`的syscall号。

- 在`struct proc`中添加成员变量`int mask`；

- 在`kernel / sysproc.c`中添加一个`sys_trace（）`函数，该函数通过记住`proc`结构中新变量中的参数来实现新的系统调用。

  ```cpp
  uint64
  sys_trace(void)
  {
    int n;
    if(argint(0, &n) < 0)
      return -1;
    myproc()->mask = n;
    return 0;
  }
  ```

- 修改`fork（）`（请参阅`kernel / proc.c`）以将跟踪掩码从父进程复制到子进程，即添加语句`np->mask = p->mask;`。

- 修改`kernel / syscall.c`的`syscall（）`函数以打印跟踪输出。需要添加一个系统调用名称数组以建立索引。

  ```cpp
  void syscall(void)
  {
    int num;//syscall number, stored at a7
    struct proc *p = myproc();
  
    num = p->trapframe->a7;
    if (num > 0 && num < NELEM(syscalls) && syscalls[num])
    {
      p->trapframe->a0 = syscalls[num]();//syscall ret value, need to store in a0
      //trace
      if (p->mask & (1 << num))
        printf("%d: syscall %s -> %d\n", p->pid, syscall_names[num], p->trapframe->a0);
    }
  
    else
    {
      printf("%d %s: unknown sys call %d\n",
             p->pid, p->name, num);
      p->trapframe->a0 = -1;
    }
  }
  ```

- 运行：

  ```shell
  litang@LAPTOP-A13E53DB:/mnt/c/Users/litang/githubWorkSpace/xv6-labs-2020$ ./grade-lab-syscall trace
  make: 'kernel/kernel' is up to date.
  == Test trace 32 grep == trace 32 grep: OK (7.8s)
      (Old xv6.out.trace_32_grep failure log removed)
  == Test trace all grep == trace all grep: OK (2.3s)
      (Old xv6.out.trace_all_grep failure log removed)
  == Test trace nothing == trace nothing: OK (2.4s)
  == Test trace children == trace children: OK (22.7s)
  ```

## Sysinfo

- 将`$U/_sysinfotest`添加到Makefile中的UPROGS

- 按照与上一个作业相同的步骤添加系统调用sysinfo。要在`user / user.h`声明sysinfo()，需要预先声明`struct sysinfo`：

  ```
      struct sysinfo; 
      int sysinfo（struct sysinfo *）;
  ```

- sysinfo需要将`struct sysinfo`复制回用户空间。`copyout（）`的定义：

  ```cpp
  // in kernel/vm.c
  // Copy from kernel to user.
  // Copy len bytes from src to virtual address dstva in a given page table.
  // Return 0 on success, -1 on error.
  int
  copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
  ```

- 要收集可用内存量，需要在`kernel / kalloc.c`中添加一个函数：

  ```cpp
  uint64
  freememory()
  {
    struct run *p = kmem.freelist;
    uint64 num = 0;
    while (p)
    {
      num++;
      p = p->next;
    }
    return num * PGSIZE;
  }
  ```

- 要收集进程数，需要在`kernel / proc.c`添加一个函数：

  ```cpp
  int proc_size(void)
  {
    int cnt = 0;
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++)
      if (p->state != UNUSED)
        cnt++;
    return cnt;
  }
  ```


实验结果：

       ```cpp
$ ./grade-lab-syscall
make: 'kernel/kernel' is up to date.
== Test trace 32 grep == trace 32 grep: OK (1.6s)
== Test trace all grep == trace all grep: OK (0.8s)
== Test trace nothing == trace nothing: OK (0.9s)
== Test trace children == trace children: OK (10.4s)
== Test sysinfotest == sysinfotest: OK (2.0s)
== Test time ==
time: OK
Score: 35/35
       ```











