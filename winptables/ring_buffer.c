/*
 * File Name:		ring_buffer.c
 * Description:		A thread safe ring buffer
 * Date:			2022.1.14
 * Author:			HBSnail
 */

#include "global.h"
#include "ring_buffer.h"


ULONG GetRingBufferAvailable(RING_BUFFER* ringBuffer) {
	return ((ringBuffer->head) >= (ringBuffer->tail)) ?
		((ringBuffer->head) - (ringBuffer->tail)) :
		ringBuffer->bufferSize - (ringBuffer->tail) + (ringBuffer->head);
}
 /*++

 Routine Description:

	Init a ring buffer structure

 Arguments:

	RING_BUFFER* ringBuffer - the pointer of a ring buffer structure
	ULONG powerOf2length - the maxium length of the ring buffer will be set to (2^powerOf2length)bytes

 Return Value:

	 NTSTATUS indecate if the ring buffer init successful

 --*/
NTSTATUS InitRingBuffer(IN RING_BUFFER* ringBuffer, IN ULONG powerOf2length) {

	NTSTATUS status = STATUS_SUCCESS;

	do {

		if (ringBuffer == NULL) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		ULONG length = (1 << powerOf2length);

		if (length == 0) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		ringBuffer->head = 0;
		ringBuffer->tail = 0;

		NdisAllocateSpinLock(&(ringBuffer->readLock));

		if (&ringBuffer->readLock == NULL) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		NdisAllocateSpinLock(&(ringBuffer->writeLock));

		if (&ringBuffer->writeLock == NULL) {
			NdisFreeSpinLock(&ringBuffer->readLock);
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		ringBuffer->bufferAddress = ExAllocatePoolWithTag(NonPagedPool, length, RING_BUFFER_ALLOC_TAG);
		if (ringBuffer->bufferAddress == NULL) {
			status = STATUS_UNSUCCESSFUL;
			NdisFreeSpinLock(&ringBuffer->readLock);
			NdisFreeSpinLock(&ringBuffer->writeLock);
			break;
		}


		KeInitializeEvent(&ringBuffer->dataWrite, SynchronizationEvent,FALSE);

		ringBuffer->bufferSize = length;
		ringBuffer->modFactor = length - 1;


	} while (FALSE);

	return status;
}


/*++

Routine Description:

   Free the resources used by ring buffer

Arguments:

	 RING_BUFFER* ringBuffer - the pointer of a ring buffer structure to be free

Return Value:

	NULL

--*/
VOID FreeRingBuffer(IN RING_BUFFER* ringBuffer) {

	if (ringBuffer == NULL) {
		return;
	}

	if (ringBuffer->bufferAddress != NULL) {
		ExFreePoolWithTag(ringBuffer->bufferAddress, RING_BUFFER_ALLOC_TAG);
		ringBuffer->bufferAddress = NULL;
	}

	NdisFreeSpinLock(&ringBuffer->writeLock);
	NdisFreeSpinLock(&ringBuffer->readLock);

	ringBuffer->head = 0;
	ringBuffer->tail = 0;
	ringBuffer->bufferSize = 0;
	ringBuffer->modFactor = 0;

}


/*++

Routine Description:

   Write data to ring buffer

Arguments:

	RING_BUFFER* destinationRingBuffer - the pointer of a ring buffer structure to be written
	VOID* sourceBuffer
	ULONG length - the length to be written
	BOOLEAN isLocked - indecate whether the resource lock of the ring buffer has been acquired or not

Return Value:

	NTSTATUS - whether writing successful

--*/
NTSTATUS WriteRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer, ULONG length, BOOLEAN isLocked) {

	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN dispatchLevel = (KeGetCurrentIrql() == DISPATCH_LEVEL);

	//Before write must get the spin lock first. Only one thread can read or write the same ring buffer.
	if (!isLocked) {
		NDIS_ACQUIRE_LOCK(&destinationRingBuffer->writeLock, dispatchLevel);
	}

	//Check if the ring buffer has space to write
	if (GetRingBufferAvailable  (destinationRingBuffer) + length < destinationRingBuffer->bufferSize) {

		//If it has free space, check whether needs turning
		if (destinationRingBuffer->head + length < destinationRingBuffer->bufferSize) {
			//Direct copy memory
			NdisMoveMemory(destinationRingBuffer->bufferAddress + destinationRingBuffer->head, (BYTE*)sourceBuffer, length);
		}
		else {
			//Get the length and copy twice
			ULONG partLength = destinationRingBuffer->bufferSize - destinationRingBuffer->head;
			NdisMoveMemory(destinationRingBuffer->bufferAddress + destinationRingBuffer->head, (BYTE*)sourceBuffer, partLength);
			NdisMoveMemory(destinationRingBuffer->bufferAddress, (BYTE*)sourceBuffer + partLength, length - partLength);
		}

		destinationRingBuffer->head = (destinationRingBuffer->head + length) & (destinationRingBuffer->modFactor);

	}
	else {

		//There is no space for writing
		status = STATUS_UNSUCCESSFUL;
	}



	//After write release the spin lock. Allow other threads read or write.
	if (!isLocked) {
		NDIS_RELEASE_LOCK(&destinationRingBuffer->writeLock, dispatchLevel);
	}
	

	return status;
}


/*++

Routine Description:

   Read data from ring buffer

Arguments:

	RING_BUFFER* sourceRingBuffer - the pointer of a ring buffer structure to be read
	VOID* destinationBuffer
	ULONG length - the length to be read
	BOOLEAN isLocked - indecate whether the resource lock of the ring buffer has been acquired or not

Return Value:

	NTSTATUS - whether reading successful

--*/
NTSTATUS ReadRingBuffer(IN RING_BUFFER* sourceRingBuffer, VOID* destinationBuffer, ULONG length, BOOLEAN isLocked) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN dispatchLevel = (KeGetCurrentIrql() == DISPATCH_LEVEL);

	//Before read must get the spin lock first. Only one thread can read or write the same ring buffer.
	if (!isLocked) {
		NDIS_ACQUIRE_LOCK(&sourceRingBuffer->readLock, dispatchLevel);
	}
		

	if (GetRingBufferAvailable(sourceRingBuffer) >= length) {


		if (sourceRingBuffer->tail + length < sourceRingBuffer->bufferSize) {
			//Direct copy memory
			NdisMoveMemory((BYTE*)destinationBuffer, sourceRingBuffer->bufferAddress + sourceRingBuffer->tail, length);
		}
		else {
			//Get the length and copy twice
			ULONG partLength = sourceRingBuffer->bufferSize - sourceRingBuffer->tail;
			NdisMoveMemory((BYTE*)destinationBuffer, sourceRingBuffer->bufferAddress + sourceRingBuffer->tail, partLength);
			NdisMoveMemory((BYTE*)destinationBuffer + partLength, sourceRingBuffer->bufferAddress, length - partLength);
		}

		sourceRingBuffer->tail = (sourceRingBuffer->tail + length) & (sourceRingBuffer->modFactor);

	}
	else {

		//The available length less than the needs length
		status = STATUS_UNSUCCESSFUL;
	}


	//After read release the spin lock. Allow other threads enter.
	if (!isLocked) {
		NDIS_RELEASE_LOCK(&sourceRingBuffer->readLock, dispatchLevel);
	}
		

	return status;
}


/*++

Routine Description:

   Read a data block from ring buffer

Arguments:

	RING_BUFFER* sourceRingBuffer - the pointer of a ring buffer structure to be read
	VOID* destinationBuffer

Return Value:

	NTSTATUS - whether read successful

--*/
NTSTATUS ReadBlockFromRingBuffer(IN RING_BUFFER* sourceRingBuffer, IN VOID* destinationBuffer) {


	//Warning!!!
	//DO NOT attempt wait for a event/semaphore when holding the spin lock, otherwise it may cause deadlock.
	while (GetRingBufferAvailable(sourceRingBuffer) < RING_BUFFER_BLOCK_SIZE) {
		KeWaitForSingleObject(&sourceRingBuffer->dataWrite, Executive, KernelMode, FALSE, NULL);
	}

	return ReadRingBuffer(sourceRingBuffer, destinationBuffer, RING_BUFFER_BLOCK_SIZE, FALSE);
	
}


/*++

Routine Description:

   Write a data block from ring buffer

Arguments:

	RING_BUFFER* destinationRingBuffer - the pointer of a ring buffer structure to be written
	VOID* sourceBuffer

Return Value:

	NTSTATUS - whether read successful

--*/
NTSTATUS WriteBlockToRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer) {

	NTSTATUS status = STATUS_SUCCESS;

	status = WriteRingBuffer(destinationRingBuffer, sourceBuffer, RING_BUFFER_BLOCK_SIZE, FALSE);

	if (NT_SUCCESS(status)) {
		KeSetEvent(&destinationRingBuffer->dataWrite, IO_NO_INCREMENT, FALSE);
	}

	return status;

}

