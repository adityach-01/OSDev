// common.h -- Defines typedefs and some global functions.
// From JamesM's kernel development tutorials.
#ifndef COMMON_H
#define COMMON_H

// Some nice typedefs, to standardise sizes across platforms.
// These typedefs are written for 32-bit X86.
typedef unsigned int   u32int;
typedef          int   s32int;
typedef unsigned short u16int;
typedef          short s16int;
typedef unsigned char  u8int;
typedef          char  s8int;

void memset(u8int *ptr, int val, int num_bytes);
void memcpy(u8int *src, u8int *dst, int num_bytes);
int strlen(char *s);
void PANIC(char *s);



#endif