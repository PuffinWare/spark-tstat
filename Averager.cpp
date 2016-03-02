#include <stdlib.h>
#include "Averager.h"

Averager::Averager(int size) {
  maxSize = size;
  values = (int*)malloc(sizeof(int) * size);
  curSize = 0;
  idx = 0;
  max = 0;
}

bool Averager::addValue(int value) {
  curSize += curSize==maxSize ? 0 : 1;
  values[idx] = value;
  last = value;
  if (value > max) {
    max = value;
  }
  idx++;
  if (idx == maxSize) {
    idx = 0;
  }
  int total = 0;
  for (int i=0; i<curSize; i++) {
    total += values[i];
  }
  average = total / curSize;
  return curSize == maxSize;
}
