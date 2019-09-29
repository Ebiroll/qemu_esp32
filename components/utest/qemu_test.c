#include "qemu_test.h"

void exit_qemu(int val) {
    int *quemu_test=(int *)  0x3ff005f0;

    if (*quemu_test==0x42) {
      //printf("Running in qemu\n");
      *quemu_test=val;  
    } else {
    }

    return;
}
