/*************************************************************
 * queue.h : Stack and queue Implementation
 * Copyright (C) 2014  Gobind Prasad (gobprasad@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *
 ************************************************************/

#ifndef __QUEUE_H__
#define __QUEUE_H__
#include "results.h"
#include "list.h"
#include <pthread.h>

typedef struct 
{
	LIST qList;
	pthread_mutex_t lock;
}QUEUE;


typedef struct
{
	LIST sList;
	pthread_mutex_t lock;
}STACK;

/**
 * Circular Queue Structure
 */
typedef struct cirQ
{
	LIST cQ;
	LIST_NODE *currentPos;	

}CIRCULAR_QUEUE;

void initQueue(QUEUE *);
RESULT enqueue(QUEUE *, void *);
void *dequeue(QUEUE *);

void initCircQueue(CIRCULAR_QUEUE *cq, int qSize, int qLength);
void *getCircularQueueNext(CIRCULAR_QUEUE *cq);
void cleanCircQueue(CIRCULAR_QUEUE *q);

#endif //__QUEUE_H__
