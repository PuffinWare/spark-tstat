#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Averager.h"

Averager::Averager(int size) {
  maxSize = size;
  values = (int*)malloc(sizeof(int) * size);
  memset(values, 0, sizeof(int) * size);
  curSize = 0;
  max = 0;
  min = 0;
}

void Averager::addValue(int value) {
  curSize += curSize==maxSize ? 0 : 1;
  // Want values pushed in for historical display
  for (int i=curSize-1; i>0; i--) {
    values[i] = values[i-1];
  }
  values[0] = value;
  last = value;
  if (curSize == 1 || value > max) {
    max = value;
  }
  if (curSize == 1 || value < min) {
    min = value;
  }
  int total = 0;
  for (int i=0; i<curSize; i++) {
    total += values[i];
  }
  average = total / curSize;
}
