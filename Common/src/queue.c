/*************************************************************
 * queue.c : Stack and Queue Implementation
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

#include "queue.h"
#include "list.h"
#include "results.h"
#include <pthread.h>

/**
 * Initialize Queue
 */

void initQueue(QUEUE *q)
{
	initList(&q->qList);
	pthread_mutex_init(&q->lock, NULL);
}

/**
 * Insert into Queue
 */
RESULT enqueue(QUEUE *q, void *data)
{
	RESULT res = G_OK;
	pthread_mutex_lock(&q->lock);
		res  = addLast(&q->qList, data);
	pthread_mutex_unlock(&q->lock);
	return res;
}

/**
 * Delete from Queue
 */
void *dequeue(QUEUE *q)
{
	void *ret = NULL;
	pthread_mutex_lock(&q->lock);
		ret = deleteFirst(&q->qList);
	pthread_mutex_unlock(&q->lock);
	return ret;
}

/**
 * Initialize Circular queue
 */
void initCircQueue(CIRCULAR_QUEUE *cq, int qSize, int qLength)
{
	int i =0;
	void *data = NULL;
	initList(&cq->cQ);
	for(i=0; i<qLength; i++)
	{
		data = malloc(sizeof(char)*qSize);
		addLast(&cq->cQ, data);
	}
	// CurrentPosition is List first node
	cq->currentPos = cq->cQ.first;

	// Link last node to first node for making it as circular queue
	cq->cQ.last->next = cq->cQ.first;
}

/**
 * returns circular queue current item
 */
void *getCircularQueueNext(CIRCULAR_QUEUE *cq)
{
	void *temp = cq->currentPos->data;
	cq->currentPos = cq->currentPos->next;
	return temp;
}

/**
 * Delete Circular queue with its associated data
 */
void cleanCircQueue(CIRCULAR_QUEUE *cq)
{
	LIST_NODE *tempQ = NULL;
	cq->cQ.last = NULL;
	void *data = NULL;
	tempQ = cq->cQ.first;
	while(	tempQ != NULL )
	{
		data = deleteFirst(tempQ);
		free(data);
		tempQ = tempQ->next;			
	}
}

/**
 * Initialize Stack
 */
void initStack(STACK *s)
{
	initList(&s->sList);
	pthread_mutex_init(&s->lock, NULL);
}

/**
 * Insert into stack
 */

RESULT push(STACK *s, void *data)
{
	RESULT res = G_OK;
	pthread_mutex_lock(&s->lock);
		res = addFirst(&s->sList, data);
	pthread_mutex_unlock(&s->lock);
	return res;
}

/**
 * Delete from stack
 */
void *pop(STACK *s)
{
	void *ret = NULL;
	pthread_mutex_lock(&s->lock);
		ret = deleteFirst(&s->sList);
	pthread_mutex_unlock(&s->lock);
	return ret;
}
