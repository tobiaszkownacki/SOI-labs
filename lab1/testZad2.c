#include <unistd.h>
#include <stdio.h>
#include <lib.h>

int main(int argc, char **argv)
{
    int i, j;
    message m;
    int ret;
    int children;
    int N;

    if (argc != 2)
        return 1;

    N = atoi(argv[1]);

    if(fork() == 0)
    {
        if(fork() == 0)
        {
            for(i=0; i<8; ++i)
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
        sleep(10);
        return 0;
    }


    if(fork() == 0)
    {
        for(i=0; i<2; ++i)
        {
            if(fork() == 0)
            {
                for(j=0; j<5; ++j)
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


    m.m1_i1 = N;
    ret = _syscall(MM, MAXCHILDRENWITHINLEVELS, &m);
    printf("Max children at level %d: %d\n", N, ret);

    m.m1_i1 = N;
    ret = _syscall(MM, WHOMAXCHILDRENWITHINLEVELS, &m);
    printf("Process with max children at level %d: %d\n", N, ret);
}
