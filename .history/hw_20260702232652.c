#include<stdio.h>
#include<stdint.h>

#define u8 uint8_t

int main(){
    u8 *ans;
    u8 ans2;
    u8 a[8] = {6,5,3,7,0,1,4,2};
    u8 *p = &a[0];
    u8 *q = &a[2];
    u8 *r = &a[3];

    //ans = *((u8*)*p);
    //ans = (u8*)p;
    ans2 = r[2];

    printf("%d\n",ans2);
}