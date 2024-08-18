#include "queue.h"
#include <stdio.h>

void initQueue(Queue *q) {
  q->front = 0;
  q->rear = -1;
  q->size = 0;
  q->sum = 0.0f;
}

int isFull(Queue *q) { return q->size == MAX; }

int isEmpty(Queue *q) { return q->size == 0; }

void enqueue(Queue *q, float value) {
  if (isFull(q)) {
    // 队列已满，移除最早的元素
    q->sum -= q->data[q->front];
    q->front = (q->front + 1) % MAX;
    q->size--;
  }
  q->rear = (q->rear + 1) % MAX;
  q->data[q->rear] = value;
  q->sum += value;
  q->size++;
}

float dequeue(Queue *q) {
  if (isEmpty(q)) {
    formatted_print("Queue is empty! Cannot dequeue elements.\n");
    return -1.0f; // Return -1.0f to indicate an error
  } else {
    float value = q->data[q->front];
    q->sum -= value;
    q->front = (q->front + 1) % MAX;
    q->size--;
    return value;
  }
}

float front(Queue *q) {
  if (isEmpty(q)) {
    formatted_print("Queue is empty! No front element.\n");
    return -1.0f; // Return -1.0f to indicate an error
  } else {
    return q->data[q->front];
  }
}

float getAverage(Queue *q) {
  if (isEmpty(q)) {
    formatted_print("Queue is empty! Cannot calculate average.\n");
    return 0.0f; // Return 0.0f as the average for an empty queue
  } else {
    return q->sum / q->size;
  }
}