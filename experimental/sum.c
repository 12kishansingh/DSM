#include<stdio.h>

 

int main(){
    int sum = 0;
    int a;
    while(scanf("%d",&a) == 1){
        sum += a;
    }
    printf("Sum: %d\n",sum);
}