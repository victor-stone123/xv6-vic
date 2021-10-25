#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#include "kernel/param.h"

int main (int argc, char * argv[])
{
    char * nargv[MAXARG];
    int i;
     
    char buf[512];
    char *p = buf;
    char *ps = buf;

    for (i=1; i<argc;i++)
        nargv[i-1] = argv[i];
    i--;
   
    int arg_start=0;
    while(read(0,p,1) )
    {
        if (*p == ' ')
        {   
            if (arg_start==1)
            {   
                *p++='\0';
                nargv[i++] = ps;
            }
            arg_start=0;
            continue;
        }
        
        if (*p =='\n')
        {
            if (arg_start==1)
            {   
                *p++='\0';
                nargv[i++] = ps;
            }
            nargv[i] =0;
            break;
        }
        if (arg_start == 0)
        { 
           ps=p;
           arg_start=1;
        }
        p++;

        if (i>= MAXARG)
        {
            printf("too much arguments\n");
            break;
        }
    }
    
    
    if (fork()==0)
    {
         exec(argv[1], nargv);
    }
    else
    {
        wait(0);
        exit(0);
    }
    return 0;
}



