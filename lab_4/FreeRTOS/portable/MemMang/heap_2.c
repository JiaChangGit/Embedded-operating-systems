/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that permits
 * allocated blocks to be freed, but does not combine adjacent free blocks
 * into a single larger block (and so will fragment memory).  See heap_4.c for
 * an equivalent that does combine adjacent blocks into single larger blocks.
 *
 * See heap_1.c, heap_3.c and heap_4.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* A few bytes might be lost to byte aligning the heap start address. */
#define configADJUSTED_HEAP_SIZE	( configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT )

/*
 * Initialises the heap structures before their first use.
 */
static void prvHeapInit( void );

/* Allocate the memory for the heap. */
#if( configAPPLICATION_ALLOCATED_HEAP == 1 )
	/* The application writer has already defined the array used for the RTOS
	heap - probably so it can be placed in a special segment or address. */
	extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
	static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */


/* Define the linked list structure.  This is used to link free blocks in order
of their size. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;


static const uint16_t heapSTRUCT_SIZE	= ( ( sizeof ( BlockLink_t ) + ( portBYTE_ALIGNMENT - 1 ) ) & ~portBYTE_ALIGNMENT_MASK );
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( heapSTRUCT_SIZE * 2 ) )

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, xEnd;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = configADJUSTED_HEAP_SIZE;

/* STATIC FUNCTIONS ARE DEFINED AS MACROS TO MINIMIZE THE FUNCTION CALL DEPTH. */

/*
 * Insert a block into the list of free blocks - which is ordered by size of
 * the block.  Small blocks at the start of the list and large blocks at the end
 * of the list.
 */
#define prvInsertBlockIntoFreeList( pxBlockToInsert )									\
		{																						\
		BlockLink_t *pxIterator, *pxBlockPtr = pxBlockToInsert;									\
		size_t xBlockSize = pxBlockPtr->xBlockSize;												\
																								\
			size_t xStartAddress = (size_t) pxBlockPtr;											\
			size_t xEndAddress = (xStartAddress + xBlockSize);									\
																								\
			BlockLink_t *pxPrevBlock = &(xStart);												\
			BlockLink_t *pxCurBlock = xStart.pxNextFreeBlock;									\
																								\
			while ((void *) pxCurBlock != (void *) &(xEnd)) {									\
																								\
				size_t xCurBlockStartAddr = (size_t) pxCurBlock;								\
				size_t xCurBlockSize = pxCurBlock->xBlockSize;									\
				size_t xCurBlockEndAddr = (xCurBlockStartAddr + xCurBlockSize);					\
																								\
				if (xStartAddress != xCurBlockEndAddr && xEndAddress != xCurBlockStartAddr) {	\
					pxPrevBlock = pxCurBlock;													\
					pxCurBlock = pxCurBlock->pxNextFreeBlock;									\
					continue;																	\
				}																				\
																								\
				if (xStartAddress == xCurBlockEndAddr) {										\
					pxBlockPtr = pxCurBlock;													\
					pxBlockPtr->xBlockSize += xBlockSize;										\
				}																				\
				else if (xEndAddress == xCurBlockStartAddr) {									\
					pxBlockPtr->xBlockSize += xCurBlockSize;									\
				}																				\
				pxPrevBlock->pxNextFreeBlock = pxCurBlock->pxNextFreeBlock;						\
				pxCurBlock = pxCurBlock->pxNextFreeBlock;										\
																								\
			}																					\
																								\
			xBlockSize = pxBlockPtr->xBlockSize;  												\
			/* Iterate through the list until a block is found that has a larger size */		\
			/* than the block we are inserting. */												\
			for( pxIterator = &xStart; pxIterator->pxNextFreeBlock->xBlockSize < xBlockSize; pxIterator = pxIterator->pxNextFreeBlock )	\
			{																					\
				/* There is nothing to do here - just iterate to the correct position. */		\
			}																					\
																								\
			/* Update the list to include the block being inserted in the correct */			\
			/* position. */																		\
			pxBlockPtr->pxNextFreeBlock = pxIterator->pxNextFreeBlock;							\
			pxIterator->pxNextFreeBlock = pxBlockPtr;											\
		}
/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
static BaseType_t xHeapHasBeenInitialised = pdFALSE;
void *pvReturn = NULL;
size_t BlockSize, WantedSize;
char data[80];
WantedSize = xWantedSize;

	vTaskSuspendAll();
	{
		/* If this is the first call to malloc then the heap will require
		initialisation to setup the list of free blocks. */
		if( xHeapHasBeenInitialised == pdFALSE )
		{
			prvHeapInit();
			xHeapHasBeenInitialised = pdTRUE;
		}

		/* The wanted size is increased so it can contain a BlockLink_t
		structure in addition to the requested amount of bytes. */
		if( xWantedSize > 0 )
		{
			xWantedSize += heapSTRUCT_SIZE;

			/* Ensure that blocks are always aligned to the required number of bytes. */
			if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0 )
			{
				/* Byte alignment required. */
				xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
			}
		}

		if( ( xWantedSize > 0 ) && ( xWantedSize < configADJUSTED_HEAP_SIZE ) )
		{
			/* Blocks are stored in byte order - traverse the list from the start
			(smallest) block until one of adequate size is found. */
			pxPreviousBlock = &xStart;
			pxBlock = xStart.pxNextFreeBlock;
			while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
			{
				pxPreviousBlock = pxBlock;
				pxBlock = pxBlock->pxNextFreeBlock;
			}

			/* If we found the end marker then a block of adequate size was not found. */
			if( pxBlock != &xEnd )
			{
				/* Return the memory space - jumping over the BlockLink_t structure
				at its start. */
				pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + heapSTRUCT_SIZE );

				/* This block is being returned for use so must be taken out of the
				list of free blocks. */
				pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

				/* If the block is larger than required it can be split into two. */
				if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
				{
					/* This block is to be split into two.  Create a new block
					following the number of bytes requested. The void cast is
					used to prevent byte alignment warnings from the compiler. */
					pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

					/* Calculate the sizes of two blocks split from the single
					block. */
					pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
					pxBlock->xBlockSize = xWantedSize;

					/* Insert the new block into the list of free blocks. */
					prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
				}

				xFreeBytesRemaining -= pxBlock->xBlockSize;
			}
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
	( void ) xTaskResumeAll();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
	}
	#endif

    BlockSize = xWantedSize;
    sprintf(data, "pvReturn: %p | heapSTRUCT_SIZE: %0d | WantedSize: %3d | BlockSize: %3d\n\r", pvReturn, heapSTRUCT_SIZE, WantedSize, BlockSize);
	HAL_UART_Transmit(&huart2, (uint8_t *)data, strlen(data), 0xffff);

	return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;

	if( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= heapSTRUCT_SIZE;

		/* This unexpected casting is to keep some compilers from issuing
		byte alignment warnings. */
		pxLink = ( void * ) puc;

		vTaskSuspendAll();
		{
			/* Add this block to the list of free blocks. */
			prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
			xFreeBytesRemaining += pxLink->xBlockSize;
			traceFREE( pv, pxLink->xBlockSize );
		}
		( void ) xTaskResumeAll();
	}
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit( void )
{
BlockLink_t *pxFirstFreeBlock;
uint8_t *pucAlignedHeap;

	/* Ensure the heap starts on a correctly aligned boundary. */
	pucAlignedHeap = ( uint8_t * ) ( ( ( portPOINTER_SIZE_TYPE ) &ucHeap[ portBYTE_ALIGNMENT ] ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
	xStart.xBlockSize = ( size_t ) 0;

	/* xEnd is used to mark the end of the list of free blocks. */
	xEnd.xBlockSize = configADJUSTED_HEAP_SIZE;
	xEnd.pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = configADJUSTED_HEAP_SIZE;
	pxFirstFreeBlock->pxNextFreeBlock = &xEnd;
}
/*-----------------------------------------------------------*/

void vPadding(char str [], int padNum) {
	for (int i = 0; i < padNum; ++i) {
		strcat(str, " ");
	}
}

void vPrintFreeList(void)
{
    char msg [100];
    memset(msg, '\0', sizeof(msg));
    sprintf(msg, "StartAddress heapSTRUCT_SIZE xBlockSize EndAddress\n\r");
    HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 0xffff);

    BlockLink_t *curNode = xStart.pxNextFreeBlock;
    while ( (void *) curNode != (void *) &(xEnd) ) {

    	memset(msg, '\0', sizeof(msg));

    	char startAddr [20];
    	memset(startAddr, '\0', sizeof(startAddr));
    	itoa((size_t) curNode, startAddr, 16);
    	strcat(msg, "0x");
    	strcat(msg, startAddr);
    	vPadding(msg, 9);

    	char heapStructSize [5];
    	memset(heapStructSize, '\0', sizeof(heapStructSize));
    	itoa((uint16_t) heapSTRUCT_SIZE, heapStructSize, 10);
    	strcat(msg, heapStructSize);
    	vPadding(msg, 10);

    	char blockSize [10];
    	memset(blockSize, '\0', sizeof(blockSize));
    	itoa((size_t) curNode->xBlockSize, blockSize, 10);
    	char tmp [10];
    	memset(tmp, '\0', sizeof(tmp));
    	vPadding(tmp, 5 - strlen(blockSize));
    	strcat(tmp, blockSize);
    	strcat(msg, tmp);
    	vPadding(msg, 5);

    	char endAddr [20];
    	memset(endAddr, '\0', sizeof(endAddr));
    	itoa((size_t) curNode + curNode->xBlockSize, endAddr, 16);
    	strcat(msg, "0x");
    	strcat(msg, endAddr);
    	strcat(msg, "\n\r");

    	HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 0xffff);

    	curNode = curNode->pxNextFreeBlock;
    }

    memset(msg, '\0', sizeof(msg));
	sprintf(msg, "configADJUSTED_HEAP_SIZE: %d xFreeBytesRemaining: %d\n\r", configADJUSTED_HEAP_SIZE, xFreeBytesRemaining);
	HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 0xffff);
}
