/*
 * File Name:		ring_buffer.c
 * Description:		A thread safe ring buffer
 * Date:			2022.1.14
 * Author:			HBSnail
 */

#include "global.h"
#include "ring_buffer.h"

 /*++

 Routine Description:

	Init a ring buffer structure

 Arguments:

	RING_BUFFER* ringBuffer - the pointer of a ring buffer structure
	UINT powerOf2length - the maxium length of the ring buffer will be set to (2^powerOf2length)bytes

 Return Value:

	 NTSTATUS indecate if the ring buffer init successful

 --*/
NTSTATUS InitRingBuffer(IN RING_BUFFER* ringBuffer, IN UINT powerOf2length) {

	NTSTATUS status = STATUS_SUCCESS;

	do {

		if (ringBuffer == NULL) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		UINT length = (1 << powerOf2length);

		if (length == 0) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		ringBuffer->head = 0;
		ringBuffer->tail = 0;
		ringBuffer->available = 0;

		NdisAllocateSpinLock(&(ringBuffer->resLock));

		if (&ringBuffer->resLock == NULL) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		ringBuffer->bufferAddress = ExAllocatePoolWithTag(NonPagedPool, length, RING_BUFFER_ALLOC_TAG);
		if (ringBuffer->bufferAddress == NULL) {
			status = STATUS_UNSUCCESSFUL;
			NdisFreeSpinLock(&ringBuffer->resLock);
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

	NdisFreeSpinLock(&ringBuffer->resLock);

	ringBuffer->head = 0;
	ringBuffer->tail = 0;
	ringBuffer->bufferSize = 0;
	ringBuffer->available = 0;
	ringBuffer->modFactor = 0;

}


/*++

Routine Description:

   Write data to ring buffer

Arguments:

	RING_BUFFER* destinationRingBuffer - the pointer of a ring buffer structure to be written
	VOID* sourceBuffer
	UINT length - the length to be written
	BOOLEAN isLocked - indecate whether the resource lock of the ring buffer has been acquired or not

Return Value:

	NTSTATUS - whether writing successful

--*/
NTSTATUS WriteRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer, UINT length, BOOLEAN isLocked) {

	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN dispatchLevel = (KeGetCurrentIrql() == DISPATCH_LEVEL);

	//Before write must get the spin lock first. Only one thread can read or write the same ring buffer.
	if (!isLocked) {
		NDIS_ACQUIRE_LOCK(&destinationRingBuffer->resLock, dispatchLevel);
	}

	//Check if the ring buffer has space to write
	if (destinationRingBuffer->available + length < destinationRingBuffer->bufferSize) {

		//If it has free space, check whether needs turning
		if (destinationRingBuffer->head + length < destinationRingBuffer->bufferSize) {
			//Direct copy memory
			NdisMoveMemory(destinationRingBuffer->bufferAddress + destinationRingBuffer->head, (BYTE*)sourceBuffer, length);
		}
		else {
			//Get the length and copy twice
			UINT partLength = destinationRingBuffer->bufferSize - destinationRingBuffer->head;
			NdisMoveMemory(destinationRingBuffer->bufferAddress + destinationRingBuffer->head, (BYTE*)sourceBuffer, partLength);
			NdisMoveMemory(destinationRingBuffer->bufferAddress, (BYTE*)sourceBuffer + partLength, length - partLength);
		}

		destinationRingBuffer->head = (destinationRingBuffer->head + length) & (destinationRingBuffer->modFactor);
		destinationRingBuffer->available += length;

	}
	else {

		//There is no space for writing
		status = STATUS_UNSUCCESSFUL;
	}



	//After write release the spin lock. Allow other threads read or write.
	if (!isLocked) {
		NDIS_RELEASE_LOCK(&destinationRingBuffer->resLock, dispatchLevel);
	}
	

	return status;
}


/*++

Routine Description:

   Read data from ring buffer

Arguments:

	RING_BUFFER* sourceRingBuffer - the pointer of a ring buffer structure to be read
	VOID* destinationBuffer
	UINT length - the length to be read
	BOOLEAN isLocked - indecate whether the resource lock of the ring buffer has been acquired or not

Return Value:

	NTSTATUS - whether reading successful

--*/
NTSTATUS ReadRingBuffer(IN RING_BUFFER* sourceRingBuffer, VOID* destinationBuffer, UINT length, BOOLEAN isLocked) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN dispatchLevel = (KeGetCurrentIrql() == DISPATCH_LEVEL);

	//Before read must get the spin lock first. Only one thread can read or write the same ring buffer.
	if (!isLocked) {
		NDIS_ACQUIRE_LOCK(&sourceRingBuffer->resLock, dispatchLevel);
	}
		

	if (sourceRingBuffer->available >= length) {


		if (sourceRingBuffer->tail + length < sourceRingBuffer->bufferSize) {
			//Direct copy memory
			NdisMoveMemory((BYTE*)destinationBuffer, sourceRingBuffer->bufferAddress + sourceRingBuffer->tail, length);
		}
		else {
			//Get the length and copy twice
			UINT partLength = sourceRingBuffer->bufferSize - sourceRingBuffer->tail;
			NdisMoveMemory((BYTE*)destinationBuffer, sourceRingBuffer->bufferAddress + sourceRingBuffer->tail, partLength);
			NdisMoveMemory((BYTE*)destinationBuffer + partLength, sourceRingBuffer->bufferAddress, length - partLength);
		}

		sourceRingBuffer->tail = (sourceRingBuffer->tail + length) & (sourceRingBuffer->modFactor);
		sourceRingBuffer->available -= length;

	}
	else {

		//The available length less than the needs length
		status = STATUS_UNSUCCESSFUL;
	}


	//After read release the spin lock. Allow other threads enter.
	if (!isLocked) {
		NDIS_RELEASE_LOCK(&sourceRingBuffer->resLock, dispatchLevel);
	}
		

	return status;
}


/*++

Routine Description:

   Read one ethernet frame from ring buffer

Arguments:

	RING_BUFFER* sourceRingBuffer - the pointer of a ring buffer structure to be read
	VOID* destinationBuffer

Return Value:

	UINT - the length copy to destinationBuffer in byte

--*/
UINT ReadEthFrameFromRingBuffer(IN RING_BUFFER* sourceRingBuffer, IN VOID* destinationBuffer) {


	BOOLEAN dispatchLevel = (KeGetCurrentIrql() == DISPATCH_LEVEL);

	//Warning!!!
	//DO NOT attempt wait for a event/semaphore when holding the spin lock, otherwise it may cause deadlock.
	while (sourceRingBuffer->available == 0) {
		KeWaitForSingleObject(&sourceRingBuffer->dataWrite, Executive, KernelMode, FALSE, NULL);
	}


	//Before read must get the spin lock first. Only one thread can read or write the same ring buffer.
	NDIS_ACQUIRE_LOCK(&sourceRingBuffer->resLock, dispatchLevel);
	UINT totalLength;

	if (sourceRingBuffer->available > sizeof(UINT)) {
		
		ReadRingBuffer(sourceRingBuffer, &totalLength, sizeof(UINT), TRUE);
		totalLength -= sizeof(UINT);
		ReadRingBuffer(sourceRingBuffer, destinationBuffer, totalLength, TRUE);
	}
	else {

		totalLength = 0;
		//No data to read
	}

	NDIS_RELEASE_LOCK(&sourceRingBuffer->resLock, dispatchLevel);

	return totalLength;
}


/*++

Routine Description:

   Write one ethernet frame from ring buffer

Arguments:

	RING_BUFFER* destinationRingBuffer - the pointer of a ring buffer structure to be written
	VOID* sourceBuffer - contains the ethernet frame
	UINT length - the length of the ethernet frame
	FILTER_CONTEXT* context - the context of the ethernet frame to indecate the NIC
	TRANSFER_DIRECION transferDirection 

Return Value:

	NTSTATUS - whether writing ethernet frame successful

--*/
NTSTATUS WriteEthFrameToRingBuffer(IN RING_BUFFER* destinationRingBuffer, VOID* sourceBuffer, UINT length, FILTER_CONTEXT* context, TRANSFER_DIRECION transferDirection) {

	NTSTATUS status = STATUS_SUCCESS;


	BYTE direction = transferDirection;
	UINT lengthWrite = length;
	UINT totalLength = sizeof(UINT) + sizeof(BYTE) + sizeof(ULONG) + sizeof(UINT) + lengthWrite;
	BYTE* tempbuffer = ExAllocatePoolWithTag(NonPagedPool, totalLength, TEMP_POOL_ALLOC_TAG);

	if (tempbuffer == NULL) {
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	NdisMoveMemory(tempbuffer, &totalLength, sizeof(UINT));
	NdisMoveMemory(tempbuffer + sizeof(UINT), &direction, sizeof(BYTE));
	NdisMoveMemory(tempbuffer + sizeof(UINT) + sizeof(BYTE), &(context->miniportIfIndex), sizeof(ULONG));
	NdisMoveMemory(tempbuffer + sizeof(UINT) + sizeof(BYTE) + sizeof(ULONG), &lengthWrite, sizeof(UINT));
	NdisMoveMemory(tempbuffer + sizeof(UINT) + sizeof(BYTE) + sizeof(ULONG) + sizeof(UINT), sourceBuffer, lengthWrite);

	status = WriteRingBuffer(destinationRingBuffer, tempbuffer, totalLength, FALSE);

	ExFreePoolWithTag(tempbuffer, TEMP_POOL_ALLOC_TAG);
	tempbuffer = NULL;

	if (NT_SUCCESS(status)) {
		KeSetEvent(&destinationRingBuffer->dataWrite, IO_NO_INCREMENT, FALSE);
	}

	return status;

}

