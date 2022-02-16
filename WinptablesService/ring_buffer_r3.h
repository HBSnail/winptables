/*
 * File Name:		ring_buffer_r3.h
 * Date:			2022.1.14
 * Author:			HBSnail
 */
#pragma once
#ifndef _RING_BUFFER_R3_H_		
    #define _RING_BUFFER_R3_H_

    #include "global.h"

    #define RING_BUFFER_BLOCK_SIZE 2048
    typedef struct _RING_BUFFER_SHARED_STRUCTURE
    {
        ULONG head;
        ULONG tail;
        ULONG bufferSize;
        ULONG modFactor;
    }RING_BUFFER_SHARED_STRUCTURE;

    typedef struct _RING_BUFFER_R3
    {
        RING_BUFFER_SHARED_STRUCTURE* sharedStructure;

        HANDLE dataBlockWriteEventHandle;
        BYTE* bufferAddress;
        CRITICAL_SECTION readLock;
        CRITICAL_SECTION writeLock;

    }RING_BUFFER_R3;

    ULONG GetRingBufferR3Available(RING_BUFFER_R3* ringBuffer);

    BOOLEAN WriteRingBufferR3(RING_BUFFER_R3* destinationRingBuffer, VOID* sourceBuffer, ULONG length, BOOLEAN isLocked);

    BOOLEAN ReadRingBufferR3(RING_BUFFER_R3* sourceRingBuffer, VOID* destinationBuffer, ULONG length, BOOLEAN isLocked);

    BOOLEAN ReadBlockFromRingBufferR3(RING_BUFFER_R3* sourceRingBuffer, VOID* destinationBuffer);

    BOOLEAN WriteBlockToRingBufferR3(RING_BUFFER_R3* destinationRingBuffer, VOID* sourceBuffer);


#endif
