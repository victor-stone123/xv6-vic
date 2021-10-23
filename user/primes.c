#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int p[35][2];
void child_process(int count);
int
main(int argc, char *argv[])
{
    
    int i;
    pipe(p[0]);

    if (fork()!=0)
    {
       close(p[0][0]);
       for (i=2; i<=35;i++)
       {
           write(p[0][1], &i, 4);
       }
       close(p[0][1]);
       wait(0);
       exit(0);
    }
    else
    {
      child_process(1);
    }
   return 0; 
}

void child_process(int count)
{
    int i =count;
    close(p[i-1][1]);
    int buf;
    if( read(p[i-1][0], &buf, 4) !=0)
       printf("prime %d \n",buf);
    else
    {
       close(p[i-1][1]);
       exit(0);
    }

    pipe(p[i]);
    if (fork()!=0)
    {
        close(p[i][0]);
        while( read(p[i-1][0], &buf, 4) != 0)
        {
             if (buf %(i+1))
                 write (p[i][1], &buf, 4);
        }
     
         close(p[i][1]);
         wait(0);
         exit(0);
    }
    else
    {
      child_process(++count); 
    }
    
}



