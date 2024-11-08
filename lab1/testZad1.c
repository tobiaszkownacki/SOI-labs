#include <unistd.h>
#include <stdio.h>
#include <lib.h>

int main(int argc, char **argv)
{
    int i, j;
    message m;
    int ret;
    int children;
    int A, B;

    if (argc != 4)
        return 1;
    
    children = atoi(argv[3]);
    A = atoi(argv[1]);
    B = atoi(argv[2]);

    m.m1_i1 = 1;
    m.m1_i2 = 100;

    ret = _syscall(MM, MAXCHILDREN, &m);
    printf("Number of init children before fork: %d\n", ret);

    m.m1_i1 = A;
    m.m1_i2 = B;

    if(fork() == 0)
    {
        for(i = 0; i < 2*children; ++i)
        {
            if(fork() == 0)
            {
                sleep(10);
                return 0;
            }
        }
        sleep(10);
        return 0;
    }

    if(fork() == 0)
    {
        for(i = 0; i < children; ++i)
        {
            if(fork() == 0)
            {
                for(j = 0; j < children; ++j)
                {
                    if(fork() == 0)
                    {
                        sleep(10);
                        return 0;
                    }
                }
                sleep(10);
                return 0;
            }
        }
        sleep(10);
        return 0;
    }
    sleep(1);

    ret = _syscall(MM, MAXCHILDREN, &m);
    printf("Max children between %d and %d: %d\n", A, B, ret);

    m.m1_i1 = A;
    m.m1_i2 = B;

    ret = _syscall(MM, WHOMAXCHILDREN, &m);
    printf("Pid of process with max children between %d and %d: %d\n", A, B, ret);
    return 0;
}
