#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"



uint64
sys_sysinfo(void)
{  

    // printf ("sys_info entered...\n");
    uint64 addr;
    struct proc *p = myproc(); 
    struct sysinfo sinfo;

    if (argaddr(0, &addr) <0)
        return -1;
    if (addr > (MAXVA))
        return 0xffffffffffffffff;


    sinfo.freemem = getfreebytemem();
    // printf("freemem obtained...\n");

    sinfo.nproc = getnumprocesses();


    // printf("sysinfo obtained...\n");


    if(copyout(p->pagetable, addr, (char *)&sinfo, sizeof(sinfo)) < 0)
        return -1;
    
    return 0;
}
