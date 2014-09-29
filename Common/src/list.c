/*************************************************************
 * list.c : Link List Implementation
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

#include "list.h"
#include "results.h"
#include <stdlib.h>
#include <stdio.h>

/**
 *   Initialize Link List
 */
void initList(LIST *root)
{
	root->first = NULL;
	root->last  = NULL;
	root->totalNode = 0;
}


/**
 *	ADD LAST TO THE LINK
 *
 */
RESULT addLast(LIST *root, void *data)
{
	RESULT res = G_OK;
	if(!root || !data)
	{
		//DEBUG(ERR,"root or data is null");
		return G_ERR;
	}
	LIST_NODE *temp = (LIST_NODE *)malloc(sizeof(LIST_NODE));
	if(!temp)
	{
		//DEBUG(ERR,"malloc failed");
		return G_ERR;
	}
	temp->data = data;
	temp->next = NULL;
	if(root->first == NULL)
	{
		root->first = temp;
		root->last  = temp;
	} 
	else 
	{
		root->last->next = temp;
		root->last = temp;
	}
	root->totalNode++;
	return res;	
}

/**
 *	ADD FIRST TO THE LINK
 *
 */

RESULT addFirst(LIST *root, void *data)
{
	RESULT res = G_OK;
	if(!root || !data)
	{
		//DEBUG(ERR,"root or data is null");
		return G_ERR;
	}

	LIST_NODE *temp = (LIST_NODE *)malloc(sizeof(LIST_NODE));
	if(!temp)
	{
		//DEBUG(ERR,"malloc failed");
		return G_ERR;
	}

	temp->data = data;
	temp->next = NULL;

	if(root->first == NULL)
	{
		root->first = temp;
		root->last  = temp;
	} 
	else 
	{
		temp->next  = root->first;
		root->first = temp;
	}
	root->totalNode++;
	return res;	

}

/**
 *	DELETE FIRST FROM THE LINK
 *
 */
void *deleteFirst(LIST *root)
{
	LIST_NODE *temp 	= NULL;
	void *tempData 	= NULL;

	if(!root || !root->first)
	{
		return NULL;
	}	

	if(root->first->next == NULL)
	{
		tempData = root->first->data;
		free(root->first);
		root->first = NULL;
		root->last  = NULL;
		root->totalNode--;
		return tempData;
	}
	else
	{
		temp = root->first;
		root->first = temp->next;
		tempData  = temp->data;
		free(temp);
	}
	root->totalNode--;
	return tempData;
}

/**
 *	DELETE LAST FROM THE LINK
 *
 */
void *deleteLast(LIST *root)
{
	LIST_NODE *temp = NULL;
	void *tempData = NULL;

	if(!root || !root->first)
	{
		return NULL;
	}

	if(root->first == root->last)
	{
		tempData = root->first->data;
		free(root->last);
		root->first = root->last = NULL;
		root->totalNode--;
		return tempData;
	}

	temp = root->first;
	while(temp->next != root->last)
		temp = temp->next;
	tempData = root->last->data;
	free(root->last);
	root->last = temp;
	
	return tempData;
}

LIST_NODE *getNextNode(LIST *root, LIST_NODE *currentNode)
{
	if(!root || !root->first)
	{
		return NULL;
	}
	if(currentNode == NULL)
	{
		return root->first;
	} else {
		if(currentNode->next != NULL)
			return currentNode->next;
		else
			return NULL;
	}
	return NULL;
}


void deleteAllFromList(LIST *root)
{
	void *data = NULL;
	while(root->first != NULL)
	{
		data = deleteFirst(root);
		free(data);
	}
}

RESULT deleteOneFromListHaving(LIST *root,deleteFuncPtr func)
{
	LIST_NODE *temp 	= NULL;
	LIST_NODE *temp1    = NULL;
	void *tempData 	= NULL;

	if(!root || !root->first)
	{
		return G_ERR;
	}
	temp = root->first;

	// Check first node is target node
	if(func(temp->data) == G_OK)
	{
		tempData = deleteFirst(root);
		free(tempData);
		return G_OK;
	}

	//Target node not found
	if(temp->next == NULL)
		return G_FAIL;
	
	while(temp->next != NULL)
	{
		if(func(temp->next->data) == G_OK)
			break;
		temp = temp->next;
	}

	// target node not found
	if(temp->next == NULL)
		return G_FAIL;

	//Target node found

	if(temp->next == root->last)
	{
		tempData = root->last->data;
		free(root->last);
		root->last = temp;
		temp->next = NULL;
		return G_OK;
	}
	tempData = temp->next->data;
	temp1 = temp->next->next;
	free(temp->next);
	free(tempData);
	temp->next = temp1;

	return G_OK;
}


/**
 * Print List Only for Testing
 */
void printList(LIST *root, void (*printCallback)(void *))
{
	if(!root || !root->first)
		return;
	LIST_NODE *temp = root->first;
	while(temp != NULL){
		printCallback(temp->data);
		temp = temp->next;
	}
	printf("\n");
}

