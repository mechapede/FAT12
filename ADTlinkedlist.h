/* Linkedlist header */

#ifndef _ADTLINKEDLIST_H
#define _ADTLINKEDLIST_H

#include <stdio.h>

typedef struct ADTlinkednode{
    void * val; 
    struct ADTlinkednode * next;
} ADTlinkednode;

typedef struct ADTlinkedlist{
    int num; //number of ellements
    struct ADTlinkednode * head;
    struct ADTlinkednode * tail;
}ADTlinkedlist;


//caller responsible for all memory free/allocations

/* Initiate the linkedlist structure to safe default values */
void adtInitiateLinkedList(ADTlinkedlist * list);

/*Initiate the structure of a node to safe values with val set */
void adtInitiateLinkedNode(ADTlinkednode * node, void * val);

/*Add node to linked list. Returns -1 or failure. 0 otherwise*/
int adtAddLinkedNode(ADTlinkedlist * list, ADTlinkednode * node, int index);

/* Add a node to end of linked list */
int adtAddEndLinkedNode(ADTlinkedlist * list, ADTlinkednode * node);

/* Find the index of a value in linked list if it exsists. Must provide compare function. 
 * Returns -1 if not found 
 * */
int adtFindLinkedValue(ADTlinkedlist * list, void * val, int (*compare)(void * val1, void * val2) );

/* Get node from list, freeing will cauce corruption in the list 
 * Returns NULL on failure*/
ADTlinkednode * adtPeakLinkedNode(ADTlinkedlist * list, int index);

/*Remove node from list. Caller responsible for free. 
 * Returns NULL on failure*/
ADTlinkednode * adtPopLinkedNode(ADTlinkedlist * list, int index);


#endif
