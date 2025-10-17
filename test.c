#include <stdlib.h>
#include <stdio.h>

int main(){
    char s[24];
    char *a= malloc(sizeof(char) * 10);
    if (!a)
        return 1;
    a = "Allo !";
    s = a;
    printf("%s\n", a);
}