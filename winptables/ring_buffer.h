/*
 * File Name:		ring_buffer.h
 * Date:			2022.1.14
 * Author:			HBSnail
 */


#pragma once
#ifndef _RING_BUFFER_H_		

    #define _RING_BUFFER_H_

    #include "global.h"

    typedef struct _RING_BUFFER
    {

        NDIS_SPIN_LOCK resLock;
        BYTE* bufferAddress;
        UINT head;
        UINT tail;
        UINT available;
        UINT bufferSize;
        UINT modFactor;
        KEVENT dataWrite;

    }RING_BUFFER;

	NTSTATUS InitRingBuffer(IN RING_BUFFER* ringBuffer, IN UINT powerOf2length);

	VOID FreeRingBuffer(IN RING_BUFFER* ringBuffer);

	NTSTATUS WriteRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer, UINT length, BOOLEAN isLocked);

	NTSTATUS ReadRingBuffer(IN RING_BUFFER* sourceRingBuffer, VOID* destinationBuffer, UINT length, BOOLEAN isLocked);

	UINT ReadEthFrameFromRingBuffer(IN RING_BUFFER* sourceRingBuffer, IN VOID* destinationBuffer);

    NTSTATUS WriteEthFrameToRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer, UINT length, FILTER_CONTEXT* context, TRANSFER_DIRECION transferDirection);


#endif

