#include<stdio.h>
#include<memory/vaddr.h>
// word_t vaddr_ifetch(vaddr_t addr, int len);
// word_t vaddr_read(vaddr_t addr, int len);
// void vaddr_write(vaddr_t addr, int len, word_t data);
void mytest() {
    word_t a = vaddr_ifetch(0x80000000, 1);
    Warn("mytest: 0x%08x\n", a);
}