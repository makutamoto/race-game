#ifdef __BORLANDC__
#include<stdio.h>
#include<Windows.h>

#include "./include/borland.h"

int fopen_s(FILE** pFile, const char *filename, const char *mode) {
  *pFile = fopen(filename, mode);
  if(*pFile == NULL) return -1;
  return 0;
}

float roundf(float x) {
  union {
    float value;
    uint32_t word;
  } data;
  int exponent;
  data.value = x;
  exponent = (int)((data.word & 0x7F800000) >> 23) - 127;
  if(exponent < 23) {
    if(exponent < 0) {
      data.word &= 0x80000000;
      if(exponent == -1) data.word |= 127 << 23;
    } else {
      uint32_t exponent_mask = 0x007FFFFF >> exponent;
      if((data.word & exponent_mask) == 0) return data.value;
      data.word += 0x00400000 >> exponent;
      data.word &= ~exponent_mask;
    }
  } else {
      return data.value;
  }

	return data.value;
}

#endif
