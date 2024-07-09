#include <stdio.h>
#include <stdlib.h>
long long int fib(int n){
    // fib seq : 1,1,2,3,5,8,13.....
    // works till fib 46
    if (n<=0 )
    {
        printf("Not possible\n");
        exit(0);
    }
    if (n>=47)
    {
        printf("Out of range\n");
        exit(1);
    }
    int a=1;
    int b=1;
    int temp;
    for(int i=2;i<n;i++)
    {
        temp=b;
        b=a+b;
        a=temp;
    }
    return b;
   
}

void main(int argc, char const *argv[])
{
    printf("Fib(%d) = %lld\n",atoi(argv[1]),fib(atoi(argv[1])));
}