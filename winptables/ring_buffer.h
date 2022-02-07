/*
 * File Name:		ring_buffer.h
 * Date:			2022.1.14
 * Author:			HBSnail
 */


#pragma once
#ifndef _RING_BUFFER_H_		

    #define _RING_BUFFER_H_

    #define RING_BUFFER_BLOCK_SIZE 2048
    #include "global.h"

    typedef struct _RING_BUFFER
    {

        NDIS_SPIN_LOCK writeLock;
        NDIS_SPIN_LOCK readLock;
        BYTE* bufferAddress;
        UINT head;
        UINT tail;
        UINT bufferSize;
        UINT modFactor;
        KEVENT dataWrite;

    }RING_BUFFER;

	NTSTATUS InitRingBuffer(IN RING_BUFFER* ringBuffer, IN UINT powerOf2length);

	VOID FreeRingBuffer(IN RING_BUFFER* ringBuffer);

	NTSTATUS WriteRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer, UINT length, BOOLEAN isLocked);

	NTSTATUS ReadRingBuffer(IN RING_BUFFER* sourceRingBuffer, VOID* destinationBuffer, UINT length, BOOLEAN isLocked);

	NTSTATUS ReadBlockFromRingBuffer(IN RING_BUFFER* sourceRingBuffer, IN VOID* destinationBuffer);

    NTSTATUS WriteBlockToRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer);


#endif

