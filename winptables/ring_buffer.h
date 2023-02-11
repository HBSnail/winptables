/*
 * File Name:		ring_buffer.h
 * Date:			2022.1.14
 * Author:			HBSnail
 */


#pragma once
#ifndef _RING_BUFFER_H_		

    #define _RING_BUFFER_H_

    #define RING_BUFFER_BLOCK_SIZE 1660
    #include "global.h"

    typedef struct _RING_BUFFER
    {
        struct _RING_BUFFER_SHARED_VARIABLES
        {
            ULONG head;
            ULONG tail;
            ULONG bufferSize;
            ULONG modFactor;
        }RING_BUFFER_SHARED_VARIABLES;

        KEVENT* dataBlockWrite;
        HANDLE dataBlockWriteEventHandle;
        NDIS_SPIN_LOCK writeLock;
        NDIS_SPIN_LOCK readLock;
        BYTE* bufferAddress;

    }RING_BUFFER;

    NTSTATUS InitRingBuffer(IN RING_BUFFER* ringBuffer, IN ULONG powerOf2length, IN UNICODE_STRING* syncEventName);

	VOID FreeRingBuffer(IN RING_BUFFER* ringBuffer);

	NTSTATUS WriteRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer, ULONG length, BOOLEAN isLocked);

	NTSTATUS ReadRingBuffer(IN RING_BUFFER* sourceRingBuffer, VOID* destinationBuffer, ULONG length, BOOLEAN isLocked);

	NTSTATUS ReadBlockFromRingBuffer(IN RING_BUFFER* sourceRingBuffer, IN VOID* destinationBuffer);


    NTSTATUS WriteBlockToRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer);


#endif

