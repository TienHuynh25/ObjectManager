#include<stdio.h>
#include<assert.h>
#include <stdlib.h>
#include "ObjectManager.h"
#include<String.h>

//------------------------LINKEDLIST CLASS-------------------------------------

//-----------------------------------------------------------------------------
// CONSTANTS AND TYPES
//-----------------------------------------------------------------------------
//typedef enum BOOL {false, true } Boolean;

// Linked list node definition
typedef struct Node node;

//node struct
struct Node
{
    unsigned long numBytes ;
    unsigned long startAdd ;
    unsigned long ref;
    int count;
    node *next;
};

struct LINKEDLIST{
    node *top ;
    node *tail ;
};
typedef struct LINKEDLIST *linkPter;

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------

// add an element to the linkedlist
static void insert(linkPter link,unsigned long numBytes, unsigned long startAdd, int ref, int count);

//destroy the whole linkedlist
static void destroy(linkPter link);

//remove given element in linkedlist
static bool removeNode(linkPter link,unsigned long ref);

//search a node for a given ref
static node *search(linkPter link, unsigned long ref );

//get the size of linkedList.
static int getSize(linkPter link);

//creating-node method prototypes
static node *createNode(unsigned long numBytes, unsigned long startAdd, int ref, int count, node* pointer);

//create new LinkedList
static linkPter createLink();

//test link
static void validate(linkPter link);

//------------------------OBJECT MANAGER-------------------------------------

//-----------------------------------------------------------------------------
// CONSTANTS AND TYPES
//-----------------------------------------------------------------------------
static uchar *b1 = NULL;
static uchar *b2 = NULL;
static uchar *B = NULL;
static ulong remainMem =  MEMORY_SIZE;
static ulong curAdd = 0;
static int curRef = 1;
static linkPter link;

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
//clean up buffer
static void compact();
//calculate remainning memory
static void calRemain();
//validate object manager
void validateOM();


//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Name: initPool
// Parameter: NONE
// Purpose: creating a new Object Manager
// Return:NONE
//-----------------------------------------------------------------------------
void initPool(){
    if(B == NULL){
        assert(B == NULL && b1 == NULL && b2 == NULL);
        b1 = (uchar*)malloc(MEMORY_SIZE);
        b2 = (uchar*)malloc(MEMORY_SIZE);
        B = b1;
        link = createLink();
    }
    validateOM();
}

//-----------------------------------------------------------------------------
// Name: destroyPool
// Parameter: NONE
// Purpose: destroy Object Manager
// Return:NONE
//-----------------------------------------------------------------------------
void destroyPool(){
    free(b1);
    free(b2);
    destroy(link);
    b1 = NULL;
    b2 = NULL;
    link = NULL;
    curAdd = 0;
    curRef = 1;
}

//-----------------------------------------------------------------------------
// Name: dumpPool
// Parameter: NONE
// Purpose: printing out details of each block
// Return:NONE
//-----------------------------------------------------------------------------
void dumpPool(){
    if(link == NULL)
        return;

    node *cur = link->top;

    while(cur != NULL){
        
        fprintf(stdout,"\nblock's reference ID is %lu, its starting address is %lu, and its size is %lu",
                cur->ref,cur->startAdd, cur->numBytes);
        cur = cur->next;
    }
}

//-----------------------------------------------------------------------------
// Name: insertObject
// Parameter: ulong size
// Purpose: allocate a block of given size from buffer
// Return:Ref id
//-----------------------------------------------------------------------------
Ref insertObject( ulong size ){

    assert(size >0);
    //if insert before initPool
    if(B == NULL){
        return NULL_REF;
    }

    validateOM();

    //need to collect unused memory???
    if(remainMem <= size){
        compact();
        calRemain();
    }

    //still not enough
    if(remainMem < size){
        fprintf(stdout,"\nUnable to successfully complete memory allocation request.\n");
        return NULL_REF;
    }

    //insert new object to link
    insert(link,size,curAdd,curRef,1);

    curAdd += size; //update current address
    curRef++;       //increment current ref
    calRemain();    // calculate remaining memory

    validateOM();

    return curRef-1; //return ref id to users
}

//-----------------------------------------------------------------------------
// Name: compact
// Parameter: NONE
// Purpose: free unused space of memory
// Return:NONE
//-----------------------------------------------------------------------------
static void compact(){
    //number of current object 
    int numberOfblock = getSize(link);
    //temporary char pointer 
    static unsigned char *from = NULL, *to = NULL;

    //count number of bytes
    ulong count = 0;

    assert(link != NULL);
    validateOM();

    //first object in the link
    node *cur = link->top;
    linkPter link2 = createLink(), temp;
    assert(link2 != NULL);

    //variables to hold info of current object
    ulong startAdd = 0, numByte =0;
    ulong collected = curAdd;

    //does B point to b1 ??
    bool isB1 = false;

    //there nothing to compact
    if(numberOfblock == 0){
        return;
    }

    if(B == b1){
        isB1 = true; //B point to b1
    }

    //set curAdd to the beginning 
    curAdd = 0;

    while(cur != NULL){
        startAdd = cur->startAdd;
        numByte = cur->numBytes;
        count =0;

        //transfer object to link2
        insert(link2,numByte,curAdd,cur->ref,cur->count);

        if(isB1){
            from = &b1[startAdd];
            to = &b2[curAdd];
        }
        else{
            from = &b2[startAdd];
            to = &b1[curAdd];
        }

        //copy data
        while(count < numByte){
            *to = *from;
            to += 1;
            from += 1;
            count++;
        }
       
        curAdd += numByte; //update curAdd
        cur = cur->next; //get next object
    }

    //print out stat
    fprintf(stdout, "\nGarbage collector statistics:");
    fprintf(stdout, "\nObject: %d   bytes: %lu    freed: %lu", numberOfblock, curAdd, collected-curAdd);

    temp = link; //temp get address from link
    link = link2;//link get address from link2
    destroy(temp); //destroy temp(unused Memory)


    if(isB1){
        B = b2;
        free(b1);
        b1 = (uchar*)malloc(MEMORY_SIZE);
    }
    else{
        B = b1;
        free(b2);
        b2 = (uchar*)malloc(MEMORY_SIZE);
    }
    validateOM();
}

//-----------------------------------------------------------------------------
// Name: CalRemain
// Parameter: NONE
// Purpose: calculating the remaining memory
// Return:NONE
//-----------------------------------------------------------------------------
static void calRemain(){
    remainMem = MEMORY_SIZE - curAdd;
}

//-----------------------------------------------------------------------------
// Name: retrieveObject
// Parameter: Ref ref
// Purpose: return pointer to memory given ref id
// Return:void pointer
//-----------------------------------------------------------------------------
void *retrieveObject( Ref ref ){

    node *cur = search(link,ref);

    if(cur != NULL){
        return &B[cur->startAdd];
    }

    return NULL_REF;
}

//-----------------------------------------------------------------------------
// Name: addReference
// Parameter: Ref ref
// Purpose: add reference to object given ref id
// Return:NONE
//-----------------------------------------------------------------------------
void addReference( Ref ref ){

    node *cur = search(link,ref);
    if(cur != NULL){
        assert(cur != NULL && cur->count >= 1);
        cur->count++;
    }
}

//-----------------------------------------------------------------------------
// Name: dropReference
// Parameter: Ref ref
// Purpose: drop reference to object given ref id
// Return:NONE
//-----------------------------------------------------------------------------
void dropReference( Ref ref ){

    node *cur = search(link,ref);
    if(cur != NULL){

        assert(cur->count >0);
        cur->count--;

        if(cur->count == 0){

            removeNode(link,ref);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: validateOM
// Parameter: NONE
// Purpose: validate Object manager
// Return:NONE
//-----------------------------------------------------------------------------
void validateOM(){

    if(B != b1){
        assert(B == b2);
    }

    assert(curAdd >= 0 && curAdd <= MEMORY_SIZE);

    assert(remainMem >=0 && remainMem <= MEMORY_SIZE);

    if(link != NULL && link->tail != NULL){
    
        node *cur = link->top;

        while(cur != NULL){

            assert(cur->count > 0);
            cur = cur->next;

        }
    }

}


//------------------------LINKEDLIST CLASS-------------------------------------

//-----------------------------------------------------------------------------
// Name: CreateLink
// Parameter: linkPter link
// Purpose: creating a new linkedlist
// Return:NONE
//-----------------------------------------------------------------------------
static linkPter createLink(){
    linkPter link = (linkPter)malloc(sizeof(struct LINKEDLIST));

    //set top and tail pointer to NULL
    link->top = NULL;
    link->tail = NULL;

    assert(link != NULL);
    validate(link);
    return link;
}

//-----------------------------------------------------------------------------
// Name: CreateNode
// Parameter: unsigned long numBytes, unsigned long startAdd, int ref, int count, node* pointer
// Purpose: creating a new Node 
// Return: new node pointer
//-----------------------------------------------------------------------------
static node *createNode(unsigned long numBytes, unsigned long startAdd, int ref, int count, node* pointer){

    node *newNode = NULL;
    //allocate the memory
    newNode = (node*)malloc( sizeof( node ) );
    //hold in item
    newNode->numBytes = numBytes;
    newNode->startAdd = startAdd;
    newNode->ref = ref;
    newNode->count = count;
    //point to next node
    newNode->next = pointer;
    //check new node before return
    assert(newNode != NULL);
    //return new node
    return newNode;
}

//-----------------------------------------------------------------------------
// Name: insert
// Parameter: unsigned long numBytes, unsigned long startAdd, int ref, int count
// Purpose: inserting new node to linkedlist
// Return: NONE
//-----------------------------------------------------------------------------
static void insert(linkPter link,unsigned long numBytes, unsigned long startAdd, int ref, int count){
    if(link != NULL){
        
        node *newNode = createNode(numBytes,startAdd,ref,count,NULL);

        assert(newNode != NULL);
        assert(link != NULL);

        //first node
        if(link->top == NULL){
            link->top = newNode;
            link->tail = newNode;
        }
        else{
            link->tail->next = newNode;
            link->tail = newNode;
        }
        validate(link);
    }
}

//-----------------------------------------------------------------------------
// Name: destroy
// Parameter: NONE
// Purpose: clear all linkedlist. 
// Return: NONE(result is table should be empty)
//-----------------------------------------------------------------------------
static void destroy(linkPter link)
{
  if(link != NULL){
    node *curr = link->top;
  
    validate(link);
  
    while ( link->top != NULL )
    {
        link->top = link->top->next;
        free( curr );
    
        curr = link->top;
    }

    link->top = NULL;
    link->tail = NULL;
    validate(link);
    free(link);
    link = NULL;
  }
}

//-----------------------------------------------------------------------------
// Name: remove
// Parameter: unsigned long ref
// Purpose: removing a node given a ref 
// Return: NONE
//-----------------------------------------------------------------------------
static bool removeNode(linkPter link,unsigned long ref){

    //not a valid link
    if(link == NULL)
        return false;

    assert(link != NULL);
    validate(link);

    node *prev = NULL;
	node *cur = link->top;
		
    //looping until the condition is met
	while( (cur != NULL) && (cur->ref != ref) ) {

		prev = cur;
		cur = cur->next;
	}

    //node is not found
    if(cur == NULL)
        return false;

    //found node
    if(cur != NULL){
        if(prev != NULL){// delete last or middle item

            if(cur->next == NULL){//update tail if delete node is the last node
                link->tail = prev;
            }

            prev->next = cur->next;   
        }
        else{           // delete first or only item

            link->top = cur->next;   

            if(cur->next == NULL)
                link->tail = cur->next;
        }

        free(cur);
        cur = NULL;
        validate(link);

        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Name: search
// Parameter: unsigned long ref
// Purpose: searching for a given item
// Return: true is item is found, false otherwise
//-----------------------------------------------------------------------------
static node *search(linkPter link, unsigned long ref ){
    if(link == NULL)
        return NULL;

    node *found = NULL;
    node *curr = link->top;
    int search_count = 0;  
  
  
    while ( curr != NULL && !found )
    {
        if ( curr->ref == ref )
        {
            found = curr;
        
            // make sure we're still in our list...
            assert( search_count <= getSize(link) );
        }
        else
        {
            curr = curr->next;
            search_count++;
        }
    }
  
    // if it's not found then we should be at the end of the list
    assert( found || (search_count == getSize(link)) );
  
    return found;
}

//-----------------------------------------------------------------------------
// Name: getSize()
// Parameter: NONE
// Purpose: get size of linkedList
// Return: size of linkedList
//-----------------------------------------------------------------------------
static int getSize(linkPter link){
    int count =0;

    if(link == NULL)
        return count;

    node *cur = link->top;
    while(cur != NULL){
        count++;
        cur = cur->next;
    }

    return count;
}

//-----------------------------------------------------------------------------
// Name: validateTable
// Parameter: linkPter link
// Purpose: test link
// Return: NONE
//-----------------------------------------------------------------------------
static void validate(linkPter link)
{    
    if(link == NULL){
        return;
    }
    assert(link != NULL);
    if ( getSize(link) == 0 )
        assert( link->top == NULL && link->tail == NULL);
    else if ( getSize(link) == 1 )
        assert( link->top->next == NULL && link->tail == link->top);
    else // num_nodes > 1
        assert( link->top != NULL && link->top->next != NULL && link->tail != NULL);
}


