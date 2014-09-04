/*************************************************************
 * gthreads.c : Thread Pool Implementation
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

#include "gthreads.h"
#include "queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define  MAX_WORKER_THREAD	4


void *workerRoutine(void *arg);

typedef struct
{
	void *funcArg;
	threadRoutine func;

}JOB_STRUCT;

QUEUE workQueue;

pthread_cond_t workerCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t	workerLock  = PTHREAD_MUTEX_INITIALIZER;

/**
 * General API : Create Thread Pool
 * Creates worker threads of MAX_WORKER_THREAD
 * 
 */
void createThreadPool()
{
	initQueue(&workQueue);
	pthread_t threads[MAX_WORKER_THREAD];

	int i = 0;
	for(i=0; i<MAX_WORKER_THREAD; i++)
	{
		pthread_create(&threads[i],NULL,workerRoutine,(void *)&i);
		sleep(1);
	}
}


/**
 * General API : Add jobs to job queue
 * Add job to job queue. will be picked
 * by worker threads for processing
 */
RESULT addJobToQueue(threadRoutine func, void *funcArg){
	
	RESULT res = G_OK;
	JOB_STRUCT *tempJob = (JOB_STRUCT *)malloc(sizeof(JOB_STRUCT));
	tempJob->funcArg = funcArg;
	tempJob->func    = func;
	
	//Lock the Job Queue for adding jobs to queue
	res = enqueue(&workQueue,tempJob);

	if(res == G_OK){
		//Signal to worker threads
		pthread_cond_broadcast(&workerCond);
	} else {
		free(tempJob);
	}
	return res;
}

/**
 * This is worker thread routine
 * All the threads waits on single condition variable
 * Once signaled, checks Queue, if not empty dequeue
 * one job and process it. After processing one job
 * again check queue is empty, if not process job
 * do the same till Job Queue becomes empty
 */

void *workerRoutine(void *arg)
{
	JOB_STRUCT *job = NULL;
	int threadId = *(int*)arg;
	printf("STARTING WORKER THREAD ID %d\n",threadId);
	while(1)
	{
		pthread_mutex_lock(&workerLock);
			pthread_cond_wait(&workerCond,&workerLock);
			job = dequeue(&workQueue);
		pthread_mutex_unlock(&workerLock);
		if(job == NULL)
			continue;

		do 
		{
			if(job != NULL)
			{
				if(job->func != NULL)
				{
					printf("Printing Using THREAD ID %d \n",threadId);
					job->func(job->funcArg);
					free(job);
					job = NULL;
				}
			}
			job = dequeue(&workQueue);

		} while(job != NULL);
	}
}
