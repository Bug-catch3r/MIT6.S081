## Source code reading

- [x] `kern/memlayout.h`, which captures the layout of memory.

```cpp
// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024) 128MB
```

- [x] `kern/vm.c`, which contains most virtual memory (VM) code. read the fxxking source code!

  - [x] copyout/copyin

  ```cpp
  // Copy from kernel to user.
  // Copy len bytes from src to virtual address dstva in a given page table.
  // Return 0 on success, -1 on error.
  int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
  {
    uint64 n, va0, pa0;
  
    while (len > 0)
    {
      va0 = PGROUNDDOWN(dstva);//vpn
      pa0 = walkaddr(pagetable, va0);
      if (pa0 == 0)
        return -1;
      n = PGSIZE - (dstva - va0);
      if (n > len)
        n = len;
      memmove((void *)(pa0 + (dstva - va0)), src, n);//(dstva - va0) is offset
  
      len -= n;//write to dstva page by page
      src += n;
      dstva = va0 + PGSIZE;
    }
    return 0;
  }
  ```

  - [x] copyinstru

  *Copy a null-terminated string from user to kernel.*

  - [x] uvmcopy(mappages)

  - [x] uvmfree(uvmunmap/freewalk)

    ```cpp
    // Free user memory pages,
    // then free page-table pages.
    void uvmfree(pagetable_t pagetable, uint64 sz)
    {
    if (sz > 0)
      uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 1);
    freewalk(pagetable);
    }
    ```

  - [x] uvmmalloc/uvmdealloce

    expand/shrink pagetable size，acctually this coding style of allocation is low efficiency.to sovle:compute size needed(including pages and pagetables, remain some space which will never be used, for simplified the computation)

  - [x] uvmclear(user can't access)

  - [x] uvmcreate/uvminit

  - [x] kvminit/kvmpa/kvmmap/kvminithart

    kernel pagetable is unique, and va = pa.

  - [x] walk/walkaddr(vpn -> ppn),also alloc new pages if `alloc` is set. **mappages() will set alloc to 1.**

  ```cpp
  pte_t *
  walk(pagetable_t pagetable, uint64 va, int alloc)
  {
    if (va >= MAXVA)
      panic("walk");
  
    for (int level = 2; level > 0; level--)
    {
      pte_t *pte = &pagetable[PX(level, va)];
      if (*pte & PTE_V)
      {
        pagetable = (pagetable_t)PTE2PA(*pte);
      } 
      else
      {
        if (!alloc || (pagetable = (pde_t *)kalloc()) == 0)
          return 0;
        memset(pagetable, 0, PGSIZE);
        *pte = PA2PTE(pagetable) | PTE_V;
      }
    }
    return &pagetable[PX(0, va)];
  }
  
  // extract the three 9-bit page table indices from a virtual address.
  #define PXMASK          0x1FF // 9 bits
  #define PXSHIFT(level)  (PGSHIFT+(9*(level)))
  #define PX(level, va) ((((uint64) (va)) >> PXSHIFT(level)) & PXMASK)
  
  #define PTE2PA(pte) (((pte) >> 10) << 12) //not equel to pte << 2,notice
  ```

- [x] `kernel/kalloc.c`, which contains code for allocating and freeing physical memory.

## Problem Solving

- [x] exercise1

一些提示：

- 将`vmprint()`放在`kernel / vm.c`中。
- 使用文件`kernel / riscv.h`末尾的宏。
- 理解`walk()`的实现。
- 在`kernel / defs.h`中定义`vmprint`；
- 使用`％p`打印出完整的64位十六进制PTE和地址。

```c
void vmprint(pagetable_t p)
{
  printf("page table %p\n", p);
  //travesal whole pagetable, print every valid entry recursively(here use loop insdead)
  for (int i = 0; i < PGSIZE / 8; ++i)
  {
    if (p[i] & PTE_V)
    {
      pagetable_t pa = (pagetable_t)PTE2PA(p[i]);
      printf("..%d: pte %p pa %p\n", i, p[i], pa);
      for (int ii = 0; ii < PGSIZE / 8; ++ii)
      {
        if (pa[ii] & PTE_V)
        {
          pagetable_t ppa = (pagetable_t)PTE2PA(pa[ii]);
          printf(".. ..%d: pte %p pa %p\n", ii, pa[ii], ppa);
          for (int iii = 0; iii < PGSIZE / 8; ++iii)
          {
            if (ppa[iii] & PTE_V)
            {
              pagetable_t pppa = (pagetable_t)PTE2PA(ppa[iii]);
              printf(".. .. ..%d: pte %p pa %p\n", iii, ppa[iii], pppa);
            }
          }
        }
      }
    }
  }
}
```

- [ ] Explain the output of `vmprint` in terms of Fig 3-4 from the text. What does page 0 contain? What is in page 2? When running in user mode, could the process read/write the memory mapped by page 1?

  - [x] page0 is the init code of a user program, less than one page

  - ```cpp
    // Load the user initcode into address 0 of pagetable,
    // for the very first process.
    // sz must be less than a page.
    void uvminit(pagetable_t pagetable, uchar *src, uint sz)
    {
      char *mem;
    
      if (sz >= PGSIZE)
        panic("inituvm: more than a page");
      mem = kalloc();
      memset(mem, 0, PGSIZE);
      mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W | PTE_R | PTE_X | PTE_U);
      memmove(mem, src, sz);
    }
    ```

  - [ ] page2?

- [x] exercise2(still remains some questions, but pass the test, after 3 days debug:) 

  - [ ] Whether trapframe mapping is required in uvm_kpt_init()？
  - [ ] Why not get the pagetable from satp register in kvmpa？
  - [ ] why not freewalk to free kpagetable?Still confusing.

  ```cpp
  void proc_freekpagetable(pagetable_t pagetable)
  {
    // there are 2^9 = 512 PTEs in a page table.
    for (int i = 0; i < 512; i++)
    {
      pte_t pte = pagetable[i];
      if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0)//still confusing
      {
        pagetable[i] = 0;
        uint64 child = PTE2PA(pte);
        proc_freekpagetable((pagetable_t)child);
      }
    }
    kfree((void *)pagetable);
  }
  ```

   - [x] Add a field to `struct proc` for the process's kernel page table.
   - [x] A reasonable way to produce a kernel page table for a new process is to implement a modified version of `kvminit` that makes a new page table instead of modifying `kernel_pagetable`. You'll want to call this function from `allocproc`.
   - [x] Make sure that each process's kernel page table has a mapping for that process's kernel stack. In unmodified xv6, all the kernel stacks are set up in `procinit`. You will need to move some or all of this functionality to `allocproc`.
   - [x] Modify `scheduler()` to load the process's kernel page table into the core's `satp` register (see `kvminithart` for inspiration). Don't forget to call `sfence_vma()` after calling `w_satp()`.
   - [x] `scheduler()` should use `kernel_pagetable` when no process is running.
   - [x] Free a process's kernel page table in `freeproc`.
   - [x] You'll need a way to free a page table without also freeing the leaf physical memory pages.
   - [x] `vmprint` may come in handy to debug page tables.
   - [x] It's OK to modify xv6 functions or add new functions; you'll probably need to do this in at least `kernel/vm.c` and `kernel/proc.c`. (But, don't modify `kernel/vmcopyin.c`, `kernel/stats.c`, `user/usertests.c`, and `user/stats.c`.)
   - [x] A missing page table mapping will likely cause the kernel to encounter a page fault. It will print an error that includes `sepc=0x00000000XXXXXXXX`. You can find out where the fault occurred by searching for `XXXXXXXX` in `kernel/kernel.asm`.

- [x] exercise3

  - [x] oldsz = divroundup(oldsz)？Because we don't need to copy the last page,because we do this previously.Of course, if you don't do this, the anwser will still be right.
  - [x] How to find sbrk()? sysproc.c::growproc().
  - [x] PAGEROUNDUP(sz + n), growproc，same as Q1.
  - [x] How do I determine the location of the replicated KPagetable in EXEC? After data segment, program segment, and stack allocation, i.e., after Pagetable mapping。

## Result

```shell
== Test pte printout == pte printout: OK (1.7s)
== Test answers-pgtbl.txt == answers-pgtbl.txt: OK
== Test count copyin == count copyin: OK (0.9s)
== Test usertests == (144.8s)
== Test   usertests: copyin ==
  usertests: copyin: OK
== Test   usertests: copyinstr1 ==
  usertests: copyinstr1: OK
== Test   usertests: copyinstr2 ==
  usertests: copyinstr2: OK
== Test   usertests: copyinstr3 ==
  usertests: copyinstr3: OK
== Test   usertests: sbrkmuch ==
  usertests: sbrkmuch: OK
== Test   usertests: all tests ==
  usertests: all tests: OK
== Test time ==
time: OK
Score: 66/66
```

