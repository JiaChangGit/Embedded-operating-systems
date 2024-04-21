Author: Jia-Chang Chang
Date: 2024-03-21
Lab_1
environment: Windows 11, STM32F407VGTx, STM32CubeIDE 1.11.0
---

1. Lab1
● MultiTasking
Two tasks:
1) LED controlling
2) Button handling
(Using Inter-Task Communication (ITC) mechanism.)
● LED-task will have two states (S1, S2)
○ S1: ( 紅綠LED輪流 )
First, only Green LED lights up for 2 seconds,
and then only Red LED lights up for 2 seconds,
and then switches back to the Green LED, then Red, and so on.
○ S2: ( 橘色LED )
Only ORANGE LED is blinking (1 second ON, 1 second OFF, …)
● Button-task: If the button is pressed, the LED-task will switch to the other state.
(And execute from the start point of that state.)
○ Debounce handling
○ Edge-detection handling 

---

PD12: Green_LED
PD13: Orange_LED
PD14: Red_LED
PD15: Blue_LED

PA0: Blue_Button

---

Notice:

1. .ioc 右鍵 Enter User Label, Label name: 不要有空白

2. "FreeRTOS.h" should before "task.h"

3. xQueueCreate 用法:

// https://www.freertos.org/zh-cn-cmn-s/a00116.html

```
struct AMessage
{
    char ucMessageID;
    char ucData[ 20 ];
};

void vATask( void *pvParameters )
{
QueueHandle_t xQueue1, xQueue2;

    /* Create a queue capable of containing 10 unsigned long values. */
    xQueue1 = xQueueCreate( 10, sizeof( unsigned long ) );

    if( xQueue1 == NULL )
    {
        /* Queue was not created and must not be used. */
    }

    /* Create a queue capable of containing 10 pointers to AMessage
    structures.  These are to be queued by pointers as they are
    relatively large structures. */
    xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );

    if( xQueue2 == NULL )
    {
        /* Queue was not created and must not be used. */
    }

    /* ... Rest of task code. */
 }
 ```

4. xQueueReceive 用法:

// https://www.freertos.org/zh-cn-cmn-s/a00118.html


```
/* Define a variable of type struct AMMessage.  The examples below demonstrate
how to pass the whole variable through the queue, and as the structure is
moderately large, also how to pass a reference to the variable through a queue. */
struct AMessage
{
   char ucMessageID;
   char ucData[ 20 ];
} xMessage;

/* Queue used to send and receive complete struct AMessage structures. */
QueueHandle_t xStructQueue = NULL;

/* Queue used to send and receive pointers to struct AMessage structures. */
QueueHandle_t xPointerQueue = NULL;


void vCreateQueues( void )
{
   xMessage.ucMessageID = 0xab;
   memset( &( xMessage.ucData ), 0x12, 20 );

   /* Create the queue used to send complete struct AMessage structures.  This can
   also be created after the schedule starts, but care must be task to ensure
   nothing uses the queue until after it has been created. */
   xStructQueue = xQueueCreate(
                         /* The number of items the queue can hold. */
                         10,
                         /* Size of each item is big enough to hold the
                         whole structure. */
                         sizeof( xMessage ) );

   /* Create the queue used to send pointers to struct AMessage structures. */
   xPointerQueue = xQueueCreate(
                         /* The number of items the queue can hold. */
                         10,
                         /* Size of each item is big enough to hold only a
                         pointer. */
                         sizeof( &xMessage ) );

   if( ( xStructQueue == NULL ) || ( xPointerQueue == NULL ) )
   {
      /* One or more queues were not created successfully as there was not enough
      heap memory available.  Handle the error here.  Queues can also be created
      statically. */
   }
}

/* Task that writes to the queues. */
void vATask( void *pvParameters )
{
struct AMessage *pxPointerToxMessage;

   /* Send the entire structure to the queue created to hold 10 structures. */
   xQueueSend( /* The handle of the queue. */
               xStructQueue,
               /* The address of the xMessage variable.  sizeof( struct AMessage )
               bytes are copied from here into the queue. */
               ( void * ) &xMessage,
               /* Block time of 0 says don't block if the queue is already full.
               Check the value returned by xQueueSend() to know if the message
               was sent to the queue successfully. */
               ( TickType_t ) 0 );

   /* Store the address of the xMessage variable in a pointer variable. */
   pxPointerToxMessage = &xMessage

   /* Send the address of xMessage to the queue created to hold 10    pointers. */
   xQueueSend( /* The handle of the queue. */
               xPointerQueue,
               /* The address of the variable that holds the address of xMessage.
               sizeof( &xMessage ) bytes are copied from here into the queue. As the
               variable holds the address of xMessage it is the address of xMessage
               that is copied into the queue. */
               ( void * ) &pxPointerToxMessage,
               ( TickType_t ) 0 );

   /* ... Rest of task code goes here. */
}

/* Task that reads from the queues. */
void vADifferentTask( void *pvParameters )
{
struct AMessage xRxedStructure, *pxRxedPointer;

   if( xStructQueue != NULL )
   {
      /* Receive a message from the created queue to hold complex struct AMessage
      structure.  Block for 10 ticks if a message is not immediately available.
      The value is read into a struct AMessage variable, so after calling
      xQueueReceive() xRxedStructure will hold a copy of xMessage. */
      if( xQueueReceive( xStructQueue,
                         &( xRxedStructure ),
                         ( TickType_t ) 10 ) == pdPASS )
      {
         /* xRxedStructure now contains a copy of xMessage. */
      }
   }

   if( xPointerQueue != NULL )
   {
      /* Receive a message from the created queue to hold pointers.  Block for 10
      ticks if a message is not immediately available.  The value is read into a
      pointer variable, and as the value received is the address of the xMessage
      variable, after this call pxRxedPointer will point to xMessage. */
      if( xQueueReceive( xPointerQueue,
                         &( pxRxedPointer ),
                         ( TickType_t ) 10 ) == pdPASS )
      {
         /* *pxRxedPointer now points to xMessage. */
      }
   }

   /* ... Rest of task code goes here. */
}
```

5. Bounce:

Assumption: a legal button-pushing will be stable for a while.

Sol:

a. hardware:

b. software:

6. Color:

0xFE00: Green
