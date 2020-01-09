#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <zlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>


# define DIFFICULTY 16
# define FALSE_THREAD_TIME 2


typedef struct block_node {
    int         height;        // Incrementeal ID of the block in the chain
    int         timestamp;    // Time of the mine in seconds since epoch
    unsigned int     hash;        // Current block hash value
    unsigned int     prev_hash;    // Hash value of the previous block
    int        difficulty;    // Amount of preceding zeros in the hash
    int         nonce;        // Incremental integer to change the hash value
    int         relayed_by;    // Miner ID
} BLOCK_T;

typedef struct list_node
 {
 	struct block_node blockData;
    struct list_node*  next;
}ListNode;

typedef struct list
{
   	ListNode* head;
   	ListNode* tail;
} List;


void *minersAction(void* arg);
void *falseMinerAction(void* arg);
void *serverAction();
void updateBlockHeightAndPrevHash(ListNode * tail,ListNode * newTail);
void appendNewBlockToChain();
void updateBlockToWorkOn(ListNode * tail);
void makeEmptyList(List* lst);
void insertDataToEndList(List * lst, BLOCK_T blockToAdd);
void freeList(List* lst);
void initiallizeGenesisBlock(BLOCK_T* genesisBlock);
char* createStringForCrc(BLOCK_T blockToCalculate);
bool checkCrcResult(uInt crcResult);

BLOCK_T globalBlockToWorkOn;
uInt globalCrcToCheck;
bool globalServerNeedToWork;
bool globalMinersNeedToChangeWorkingBlock;

pthread_mutex_t g_lock;			// = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cv_not_empty;		// = PTHREAD_COND_INITIALIZER;


int main()
{
    pthread_t ptFirstMiner,ptSecondMiner,ptThirdMiner,ptFourthMiner,ptFalseMiner,ctServer;
	initiallizeGenesisBlock(&globalBlockToWorkOn);
	int arr[5] = {1,2,3,4,5}; 

	pthread_mutex_init(&g_lock, NULL);
	pthread_cond_init(&g_cv_not_empty, NULL);

	pthread_create(&ctServer, NULL, serverAction, NULL);

    pthread_create(&ptFirstMiner, NULL, minersAction, &arr[0]);
    pthread_create(&ptSecondMiner, NULL, minersAction, &arr[1]);
	pthread_create(&ptThirdMiner, NULL, minersAction, &arr[2]);
	pthread_create(&ptFourthMiner, NULL, minersAction, &arr[3]);
	pthread_create(&ptFalseMiner, NULL, falseMinerAction, &arr[4]);


    pthread_join(ptFirstMiner, NULL);
	pthread_join(ptSecondMiner, NULL);
    pthread_join(ptThirdMiner, NULL);
    pthread_join(ptFourthMiner, NULL);
    pthread_join(ptFalseMiner, NULL);

    pthread_join(ctServer, NULL);

	return 0;

}

void* minersAction(void* arg)
{
	int minerIdPtr = *(int*)arg;
	uInt crcResult;
	bool resultOfCrc;
	int runningTime;
	clock_t beginOfMine,endOfMine;
	BLOCK_T threadBlockToWorkOn;
	threadBlockToWorkOn = globalBlockToWorkOn;
    double cpu_time_used;

	beginOfMine = clock();

	for(;;)
	{
		//update nonce,time and minder id on the block
		endOfMine = clock();
		cpu_time_used = ((double) (endOfMine - beginOfMine)) / CLOCKS_PER_SEC;
		threadBlockToWorkOn.timestamp = (int)cpu_time_used;
		threadBlockToWorkOn.nonce++; 
		threadBlockToWorkOn.relayed_by = minerIdPtr;

        //calculate crc and validation of it
        char* stringForCrc = createStringForCrc(threadBlockToWorkOn);
		crcResult = crc32(0,stringForCrc,strlen(stringForCrc) -1);
		resultOfCrc =  checkCrcResult(crcResult);

		if(resultOfCrc)
		{
			// Lock before reading and writing to g_counter
		    pthread_mutex_lock(&g_lock);

			//need to contact the server and inform him about that
			globalCrcToCheck = crcResult;
			globalServerNeedToWork = true;
			beginOfMine = clock();
			globalBlockToWorkOn = threadBlockToWorkOn;

            // Release mutex and broadcast the consumers
			pthread_mutex_unlock(&g_lock);
			pthread_cond_signal(&g_cv_not_empty);
		}

		if(globalBlockToWorkOn.height != threadBlockToWorkOn.height)
		{
			//that means a new block was added and we need to update the miners
			threadBlockToWorkOn = globalBlockToWorkOn;
			beginOfMine = clock();
		}


	}   
}

void* falseMinerAction(void* arg)
{
	//this thread sends false block to the server
	uInt falseHash;
	int minerId = *(int*)arg;
	falseHash = 0;

     for(;;)
	 {
		
		pthread_mutex_lock(&g_lock);
		globalCrcToCheck = falseHash;
		globalServerNeedToWork = true;
		globalBlockToWorkOn.relayed_by = minerId;
		globalBlockToWorkOn.hash = falseHash;

		// Release mutex and broadcast the consumers
		pthread_mutex_unlock(&g_lock);
		pthread_cond_signal(&g_cv_not_empty);

        //sleep for few seconds
		sleep(FALSE_THREAD_TIME);
		falseHash++;
	 }


}
void* serverAction()
{
	uInt crcResult;
	List blockChanList;
	BLOCK_T copyOfBlock;

    for(;;)
	 {
		// Lock before reading and writing to g_counter
		pthread_mutex_lock(&g_lock);

		if(!globalServerNeedToWork)
		{
			globalMinersNeedToChangeWorkingBlock = false;

			//the miners didn't find anything - so keep on waiting
		    pthread_cond_wait(&g_cv_not_empty,&g_lock);
		}

		if(globalServerNeedToWork)
		{
			//a miner found something - we need to calculate the hash and see if it's the same
			char* stringForCrc = createStringForCrc(globalBlockToWorkOn);
		    crcResult = crc32(0,stringForCrc,strlen(stringForCrc) - 1);

			if(globalCrcToCheck == crcResult)
			{
				//the miner and server calculated the same hash
				globalBlockToWorkOn.hash = crcResult;
				copyOfBlock = globalBlockToWorkOn;

				printf("Miner #%d mined a new block %d , with the hash %u. \n",
				copyOfBlock.relayed_by,copyOfBlock.height,copyOfBlock.hash);

                printf("Server New block added by %d , with attributes : height(%d), timestamp(%d), hash(%u), prev_hash(%u), diffculty(%d), nonce(%d). \n",
				copyOfBlock.relayed_by,copyOfBlock.height,copyOfBlock.timestamp,copyOfBlock.hash,copyOfBlock.prev_hash,copyOfBlock.difficulty,copyOfBlock.nonce);

				//enter the new node
				insertDataToEndList(&blockChanList,copyOfBlock);

				//update the globalBlockToWorkOn for miners with new values
				updateBlockToWorkOn(blockChanList.tail);
			}
			else
			{
				//the miner and server mismatch
				printf("Wrong block number %d was calculated by miner %d ,received hash %u but expected hash %u. \n",
				globalBlockToWorkOn.height,globalBlockToWorkOn.relayed_by,globalBlockToWorkOn.hash,crcResult);
			}

			globalServerNeedToWork = false;//last change the bool to false
		}

		// Release mutex and broadcast the consumers
		pthread_mutex_unlock(&g_lock);

	 }   
}
void makeEmptyList(List* lst)
 {
   	//function to make sure that the list is empty
   	lst->head = lst->tail = NULL;
 }

 void insertDataToEndList(List* lst, BLOCK_T blockToAdd)
{
    	//function to insert values into the list
    	ListNode* newTail = (ListNode*)malloc(sizeof(ListNode));
    	newTail->blockData = blockToAdd;
    	newTail->next = NULL;

    	if (lst->head == NULL)
    	{
    		lst->head = lst->tail = newTail;
    	}
    	else
    	{
			//if that's not the first block in chain - update it's height,prevhash and diffculty
			updateBlockHeightAndPrevHash(lst->tail,newTail);
    		lst->tail->next = newTail;
    		lst->tail = newTail;
    	}
   
}
void updateBlockHeightAndPrevHash(ListNode * tail,ListNode * newTail)
{
	newTail->blockData.height = tail->blockData.height + 1;
	newTail->blockData.prev_hash = tail->blockData.hash;
	newTail->blockData.difficulty = tail->blockData.difficulty;
}
void updateBlockToWorkOn(ListNode * tail)
{
	globalBlockToWorkOn.difficulty = tail->blockData.difficulty;
	globalBlockToWorkOn.hash = 0;
	globalBlockToWorkOn.height = tail->blockData.height + 1;
	globalBlockToWorkOn.timestamp = 0;
	globalBlockToWorkOn.prev_hash = tail->blockData.hash;
	globalBlockToWorkOn.nonce = 0;
}
void freeList(List* lst)
{
   	ListNode* curr = lst->head, *next;
  
   	while (curr != NULL)
  	{
   		next = curr->next;
   		//free(curr->dataPtr);
   		free(curr);
  		curr = next;
   	}
   
}

void initiallizeGenesisBlock(BLOCK_T* genesisBlock)
{
    genesisBlock->height = 0;
    genesisBlock->hash = 0;
    genesisBlock->difficulty=DIFFICULTY;
}



char* createStringForCrc(BLOCK_T blockToCalculate)
{
	int height = blockToCalculate.height;
	int time = blockToCalculate.timestamp;
	unsigned int prevHash = blockToCalculate.prev_hash;
	int nonce = blockToCalculate.nonce;
	int minerId = blockToCalculate.relayed_by;

    /*printf("height %d",height);
	printf("\n");
	printf("time %d",time);
	printf("\n");
	printf("prevHash %d",prevHash);
	printf("\n");
	printf("nonce %d",nonce);
	printf("\n");
	printf("miner id %d",minerId);
	printf("\n");*/


	char heightStr[100]; 
	char timeStr[100]; 
	char prevHashStr[100]; 
	char nonceStr[32]; 
	char minerIdStr[10]; 
	
    sprintf(heightStr,"%d",height);
    sprintf(timeStr,"%d",time);
    sprintf(prevHashStr,"%u",prevHash);
    sprintf(nonceStr,"%d",nonce);
    sprintf(minerIdStr,"%d",minerId);

    char * finalString = malloc(strlen(heightStr) +strlen(prevHashStr)+strlen(timeStr)+strlen(nonceStr)+strlen(minerIdStr)+1);
	finalString[0] = '\0';

	strcat(finalString,heightStr); 
	strcat(finalString,timeStr); 
	strcat(finalString,prevHashStr); 
	strcat(finalString,nonceStr); 
	strcat(finalString,minerIdStr);

	return finalString;
}

bool checkCrcResult(uInt crcResult)
{
	bool result = crcResult <= (1<<(sizeof(crcResult)*8 - DIFFICULTY)-1)?true:false;
	return result;
}




