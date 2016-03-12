//
// Created by John Sokel on 2/29/16.
//

#ifndef AVERAGER_H
#define AVERAGER_H

class Averager {
public:
  Averager(int size);
  int getAverage() { return curSize == 0 ? 0 : average; }
  int getLast() { return curSize == 0 ? 0 : values[0]; }
  int getMax()  { return max; }
  int getMin()  { return min; }
  void reset()  { max = average; min = average; }
  void addValue(int value);
  bool isFull() { return curSize == maxSize; }
  int getValue(int idx) const { return idx>=maxSize ? 0 : values[idx]; }

private:
  int *values;
  int maxSize;
  int curSize;
  int average;
  int max;
  int min;
};

#endif // AVERAGER_H
