#include<stdio.h>
#include<stdlib.h>
#include<time.h>

int read(){
    int val = 0;
    scanf("%d", &val);
    return val;
}

void write(int val){
    printf("%d\n", val);
}