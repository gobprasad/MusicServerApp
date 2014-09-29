/*************************************************************
 * list.h : Link List Implementation
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

#ifndef __LIST_H__
#define __LIST_H__
#include "results.h"

typedef RESULT (*deleteFuncPtr)(void *);

typedef struct listNode 
{
	void *data;
	struct listNode *next;
}LIST_NODE;

typedef struct list
{
	LIST_NODE *first;
	LIST_NODE *last;
	unsigned int totalNode;
	unsigned int maxNode;
}LIST;

void initList(LIST *);

LIST_NODE *getNextNode(LIST *,LIST_NODE *);


RESULT addFirst(LIST *, void *);
RESULT addLast(LIST *, void *);
void deleteAllFromList(LIST *);

void *deleteFirst(LIST *);
void *deleteLast(LIST *);

void printList(LIST *root, void (*printCallback)(void *));


#endif //__LIST_H__
