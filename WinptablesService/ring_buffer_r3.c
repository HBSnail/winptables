/*
 * File Name:		ring_buffer_r3.c
 * Description:		a ring buffer co-operate with ring buffer from kernel
 * Date:			2022.2.11
 * Author:			HBSnail
 */

#include "global.h"
#include "ring_buffer_r3.h"

ULONG GetRingBufferR3Available(RING_BUFFER_R3* ringBuffer) {
	ULONG ret = ((ringBuffer->sharedStructure->head) >= (ringBuffer->sharedStructure->tail)) ?
		((ringBuffer->sharedStructure->head) - (ringBuffer->sharedStructure->tail)) :
		ringBuffer->sharedStructure->bufferSize - (ringBuffer->sharedStructure->tail) + (ringBuffer->sharedStructure->head);

	return ret ;
}

BOOLEAN WriteRingBufferR3(
	RING_BUFFER_R3* destinationRingBuffer,
	VOID* sourceBuffer,
	ULONG length,
	BOOLEAN isLocked
) {

	BOOLEAN success = TRUE;

	//Before write must get the spin lock first. Only one thread can read or write the same ring buffer.
	//if (!isLocked) {
	//	NDIS_ACQUIRE_LOCK(&destinationRingBuffer->writeLock, dispatchLevel);
	//}

	//Check if the ring buffer has space to write
	if (GetRingBufferR3Available(destinationRingBuffer) + length < destinationRingBuffer->sharedStructure->bufferSize) {

		//If it has free space, check whether needs turning
		if (destinationRingBuffer->sharedStructure->head + length < destinationRingBuffer->sharedStructure->bufferSize) {
			//Direct copy memory
			memcpy(destinationRingBuffer->bufferAddress + destinationRingBuffer->sharedStructure->head, (BYTE*)sourceBuffer, length);
		}
		else {
			//Get the length and copy twice
			ULONG partLength = destinationRingBuffer->sharedStructure->bufferSize - destinationRingBuffer->sharedStructure->head;
			memcpy(destinationRingBuffer->bufferAddress + destinationRingBuffer->sharedStructure->head, (BYTE*)sourceBuffer, partLength);
			memcpy(destinationRingBuffer->bufferAddress, (BYTE*)sourceBuffer + partLength, length - partLength);
		}

		destinationRingBuffer->sharedStructure->head = (destinationRingBuffer->sharedStructure->head + length) & (destinationRingBuffer->sharedStructure->modFactor);

	}
	else {

		//There is no space for writing
		success = FALSE;
	}



	//After write release the spin lock. Allow other threads read or write.
	//if (!isLocked) {
	//	NDIS_RELEASE_LOCK(&destinationRingBuffer->writeLock, dispatchLevel);
	//}


	return success;
}


BOOLEAN ReadRingBufferR3(
	RING_BUFFER_R3* sourceRingBuffer,
	VOID* destinationBuffer,
	ULONG length,
	BOOLEAN isLocked
) {
	BOOLEAN success = TRUE;

	//Before read must get the spin lock first. Only one thread can read or write the same ring buffer.
	//if (!isLocked) {
	//	NDIS_ACQUIRE_LOCK(&sourceRingBuffer->readLock, dispatchLevel);
	//}


	if (GetRingBufferR3Available(sourceRingBuffer) >= length) {


		if (sourceRingBuffer->sharedStructure->tail + length < sourceRingBuffer->sharedStructure->bufferSize) {
			//Direct copy memory
			memcpy((BYTE*)destinationBuffer, sourceRingBuffer->bufferAddress + sourceRingBuffer->sharedStructure->tail, length);
		}
		else {
			//Get the length and copy twice
			ULONG partLength = sourceRingBuffer->sharedStructure->bufferSize - sourceRingBuffer->sharedStructure->tail;
			memcpy((BYTE*)destinationBuffer, sourceRingBuffer->bufferAddress + sourceRingBuffer->sharedStructure->tail, partLength);
			memcpy((BYTE*)destinationBuffer + partLength, sourceRingBuffer->bufferAddress, length - partLength);
		}

		sourceRingBuffer->sharedStructure->tail = (sourceRingBuffer->sharedStructure->tail + length) & (sourceRingBuffer->sharedStructure->modFactor);

	}
	else {

		//The available length less than the needs length
		success = FALSE;
	}


	//After read release the spin lock. Allow other threads enter.
	//if (!isLocked) {
	//	NDIS_RELEASE_LOCK(&sourceRingBuffer->readLock, dispatchLevel);
	//}


	return success;
}


BOOLEAN ReadBlockFromRingBufferR3(RING_BUFFER_R3* sourceRingBuffer,
	VOID* destinationBuffer
) {

	//Warning!!!
	//DO NOT attempt wait for a event/semaphore when holding the spin lock, otherwise it may cause deadlock.
	while (GetRingBufferR3Available(sourceRingBuffer) < RING_BUFFER_BLOCK_SIZE) {

		WaitForSingleObject(sourceRingBuffer->dataBlockWriteEventHandle, INFINITE);
	}

	return ReadRingBufferR3(sourceRingBuffer,destinationBuffer, RING_BUFFER_BLOCK_SIZE, FALSE);

}

BOOLEAN WriteBlockToRingBufferR3(RING_BUFFER_R3* destinationRingBuffer,
	VOID* sourceBuffer
) {

	BOOLEAN success = WriteRingBufferR3(destinationRingBuffer, sourceBuffer, RING_BUFFER_BLOCK_SIZE, FALSE);

	if (success) {
		SetEvent(destinationRingBuffer->dataBlockWriteEventHandle);
	}

	return success;

}
