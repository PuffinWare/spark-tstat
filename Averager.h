//
// Created by John Sokel on 2/29/16.
//

#ifndef AVERAGER_H
#define AVERAGER_H

class Averager {
public:
  Averager(int size);
  int getAverage() { return curSize == 0 ? 0 : average; }
  int getLast() { return curSize == 0 ? 0 : last; }
  int getMax()  { return max; }
  bool addValue(int value);
  bool isFull() { return curSize == maxSize; }
  int getValue(int idx) const { return values[idx]; }

private:
  int *values;
  int maxSize;
  int curSize;
  int idx;
  int average;
  int last;
  int max;

};

#endif // AVERAGER_H
