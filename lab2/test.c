#include <unistd.h>
#include <stdio.h>
#include <lib.h>


int setProcGroup(int pid, int group)
{
    message m;
    m.m1_i1 = pid;
    m.m1_i2 = group;
    return _syscall(MM, SETPROCGROUP, &m);
}


int main(int argc, char ** argv)
{
    int a_procs, b_procs, c_procs;
    int pid;
    int result;
    int i;

    if(argc != 4)
    {
        printf("Wrong number of arguments\n");
        return -1;
    }

    a_procs = atoi(argv[1]);
    b_procs = atoi(argv[2]);
    c_procs = atoi(argv[3]);

    for(i = 0; i < a_procs; ++i)
    {
        if(fork() == 0)
        {
            pid = getpid();
            result = setProcGroup(pid, 0);
            if(result != 0)
            {
                printf("Error setting group for process %d\n", pid);
            }
            while(1);
        }
    }

    for(i = 0; i < b_procs; ++i)
    {
        if(fork() == 0)
        {
            pid = getpid();
            result = setProcGroup(pid, 1);
            if(result != 0)
            {
                printf("Error setting group for process %d\n", pid);
            }
            while(1);
        }
    }

    for(i = 0; i < c_procs; ++i)
    {
        if(fork() == 0)
        {
            pid = getpid();
            result = setProcGroup(pid, 2);
            if(result != 0)
            {
                printf("Error setting group for process %d\n", pid);
            }
            while(1);
        }
    }
    return 0;
}
