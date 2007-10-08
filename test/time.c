#include <stdio.h>
#include <time.h>

int main()
{
    time_t t = time(NULL);
    t = 1190799491;
    printf("time: %d\n",t);
    printf("time: %s\n",ctime(&t));

}

