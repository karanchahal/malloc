#include "imports.h"

int nearestSize(int size) {
    /*
    Size classes:
    8, 16, 32, 64, 128, 256, 512
    */
   if(size < 0) return 0;

   if(size >= 0 && size <= 8) {
       return 8;
   } else if(size <= 16) {
       return 16;
   } else if(size <= 32) {
       return 32;
   } else if(size <= 64) {
       return 64;
   } else if(size <= 128) {
       return 128;
   } else if(size <= 256) {
       return 256;
   } else if(size <= 512) {
       return 512;
   } else {
       return -1; // need to mmap directly.
   }
}