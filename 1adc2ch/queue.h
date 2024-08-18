#ifndef QUEUE_H
#define QUEUE_H

#include "CalcLib_FFT.h"

#define MAX 10

typedef struct {
  float data[MAX];
  int front;
  int rear;
  int size;
  float sum; // 用于存储当前队列中所有元素的和
} Queue;

void initQueue(Queue *q);
int isFull(Queue *q);
int isEmpty(Queue *q);
void enqueue(Queue *q, float value);
float dequeue(Queue *q);
float front(Queue *q);
float getAverage(Queue *q);

#endif // QUEUE_H