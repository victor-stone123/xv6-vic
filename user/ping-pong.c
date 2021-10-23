#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p[2][2];
    int pid[2];
    char buf[2];

    char byte[2] ={'p', 'c'}; 

    pipe(p[0]);
    pipe(p[1]);

    if (fork()==0)
    {   
        write(p[1][1],&byte[1],1);
        read(p[0][0],&buf[1],1 );
        pid[1]=getpid();
        printf("%d: received ping\n", pid[1]);
        exit(0);
    }
    else
    {
        write(p[0][1],&byte[0],1);
        read(p[1][0],&buf[0],1);
        pid[0]=getpid();
        wait(0);
        printf("%d: received pong\n", pid[0]);
        exit(0);
    }
}

