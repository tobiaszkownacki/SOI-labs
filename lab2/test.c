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
    int myGroup = 0;
    int myPid;
    int result;
    unsigned int i, j;

    if(argc < 2)
    {
        printf("Zla liczba argumentow\n");
        return -1;
    }

    myGroup = atoi(argv[1]);
    if(myGroup < 0 || myGroup > 2)
    {
        printf("Zly numer grupy\n");
        return -1;
    }
    myPid = getpid();

    result = setProcGroup(myPid, myGroup);
    printf("Wynik: %d\n", result);

    while(1);

    printf("Koniec\n");

}
