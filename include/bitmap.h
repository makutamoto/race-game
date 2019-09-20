#ifndef BITMAP_H
#define BITMAP_H

#include "./borland.h"

#ifndef __BORLANDC__
#include<stdint.h>
#endif

#pragma pack(1)

typedef struct _BitmapHeader {
  volatile char magicNumber[2];
  volatile uint32_t size;
  volatile uint32_t reserved;
  volatile uint32_t offset;
  volatile uint32_t dibSize;
} BitmapHeader;

typedef struct _BitmapInfoHeader {
  int32_t width;
  int32_t height;
  uint16_t nofColorPlanes;
  uint16_t nofBitsPPixel;
  uint32_t compressionMethod;
  uint32_t imageSize;
  uint32_t hResolution;
  uint32_t vResolution;
  uint32_t nofColors;
  uint32_t nofImportantColors;
} BitmapInfoHeader;

#pragma pack()

#endif
