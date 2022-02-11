#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>


#define WINPTABLES_DEVICE_NAME L"\\\\.\\winptables_comm"

#define WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_INBOUND L"Global\\winptables_ke_k2u_in"
#define WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_OUTBOUND L"Global\\winptables_ke_k2u_out"
#define WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_INBOUND L"Global\\winptables_ke_u2k_in"
#define WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_OUTBOUND L"Global\\winptables_ke_u2k_out"

#define RING_BUFFER_BLOCK_SIZE 2048


typedef struct _RING_BUFFER_SHARED_STRUCTURE
{
	ULONG head;
	ULONG tail;
	ULONG bufferSize;
	ULONG modFactor;
}RING_BUFFER_SHARED_STRUCTURE;

HANDLE commDevice;

HANDLE event_k2u_in;
HANDLE  event_k2u_out;
HANDLE  event_u2k_in;
HANDLE  event_u2k_out;


VOID* kernel2userRingBuffer_INBOUND_UserAddr;
VOID* kernel2userRingBuffer_OUTBOUND_UserAddr;
VOID* user2kernelRingBuffer_INBOUND_UserAddr;
VOID* user2kernelRingBuffer_OUTBOUND_UserAddr;

RING_BUFFER_SHARED_STRUCTURE* kernel2userRingBuffer_INBOUND_STRUCTURE;
RING_BUFFER_SHARED_STRUCTURE* kernel2userRingBuffer_OUTBOUND_STRUCTURE ;
RING_BUFFER_SHARED_STRUCTURE* user2kernelRingBuffer_INBOUND_STRUCTURE;
RING_BUFFER_SHARED_STRUCTURE* user2kernelRingBuffer_OUTBOUND_STRUCTURE ;




ULONG GetRingBufferAvailableByStructure(RING_BUFFER_SHARED_STRUCTURE* ringBuffer) {
	return ((ringBuffer->head) >= (ringBuffer->tail)) ?
		((ringBuffer->head) - (ringBuffer->tail)) :
		ringBuffer->bufferSize - (ringBuffer->tail) + (ringBuffer->head);
}

BOOLEAN WriteRingBufferByStructure(
	RING_BUFFER_SHARED_STRUCTURE* destinationRingBuffer,
	BYTE* ringBufferDataAddress,
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
	if (GetRingBufferAvailableByStructure(destinationRingBuffer) + length < destinationRingBuffer->bufferSize) {

		//If it has free space, check whether needs turning
		if (destinationRingBuffer->head + length < destinationRingBuffer->bufferSize) {
			//Direct copy memory
			memcpy(ringBufferDataAddress + destinationRingBuffer->head, (BYTE*)sourceBuffer, length);
		}
		else {
			//Get the length and copy twice
			ULONG partLength = destinationRingBuffer->bufferSize - destinationRingBuffer->head;
			memcpy(ringBufferDataAddress + destinationRingBuffer->head, (BYTE*)sourceBuffer, partLength);
			memcpy(ringBufferDataAddress, (BYTE*)sourceBuffer + partLength, length - partLength);
		}

		destinationRingBuffer->head = (destinationRingBuffer->head + length) & (destinationRingBuffer->modFactor);

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


NTSTATUS ReadRingBufferByStructure(
	RING_BUFFER_SHARED_STRUCTURE* sourceRingBuffer,
	BYTE* ringBufferDataAddress, 
	VOID* destinationBuffer,
	ULONG length,
	BOOLEAN isLocked
) {
	BOOLEAN success = TRUE;

	//Before read must get the spin lock first. Only one thread can read or write the same ring buffer.
	//if (!isLocked) {
	//	NDIS_ACQUIRE_LOCK(&sourceRingBuffer->readLock, dispatchLevel);
	//}


	if (GetRingBufferAvailableByStructure(sourceRingBuffer) >= length) {


		if (sourceRingBuffer->tail + length < sourceRingBuffer->bufferSize) {
			//Direct copy memory
			memcpy((BYTE*)destinationBuffer, ringBufferDataAddress + sourceRingBuffer->tail, length);
		}
		else {
			//Get the length and copy twice
			ULONG partLength = sourceRingBuffer->bufferSize - sourceRingBuffer->tail;
			memcpy((BYTE*)destinationBuffer, ringBufferDataAddress + sourceRingBuffer->tail, partLength);
			memcpy((BYTE*)destinationBuffer + partLength, ringBufferDataAddress, length - partLength);
		}

		sourceRingBuffer->tail = (sourceRingBuffer->tail + length) & (sourceRingBuffer->modFactor);

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


BOOLEAN ReadBlockFromRingBufferByStructure(RING_BUFFER_SHARED_STRUCTURE* sourceRingBuffer,
	BYTE* ringBufferDataAddress , 
	IN VOID* destinationBuffer, 
	HANDLE eventHandle
) {


	//Warning!!!
	//DO NOT attempt wait for a event/semaphore when holding the spin lock, otherwise it may cause deadlock.
	while (GetRingBufferAvailableByStructure(sourceRingBuffer) < RING_BUFFER_BLOCK_SIZE) {
		
		WaitForSingleObject(eventHandle, INFINITE);
	}

	return ReadRingBufferByStructure(sourceRingBuffer, ringBufferDataAddress, destinationBuffer, RING_BUFFER_BLOCK_SIZE, FALSE);

}

BOOLEAN WriteBlockToRingBufferByStructure(RING_BUFFER_SHARED_STRUCTURE* destinationRingBuffer,
	BYTE* ringBufferDataAddress,
	VOID* sourceBuffer,
	HANDLE eventHandle
) {


	BOOLEAN success= WriteRingBufferByStructure(destinationRingBuffer, ringBufferDataAddress, sourceBuffer, RING_BUFFER_BLOCK_SIZE, FALSE);

	if (success) {
		SetEvent(eventHandle);
	}

	return success;

}




VOID TestingRoutine1(VOID* must_null_ptr) {
	VOID* dataBuffer = malloc(RING_BUFFER_BLOCK_SIZE);

	while (1) {

		if (dataBuffer == NULL) {
			return;
		}

		do {

			BOOLEAN success = ReadBlockFromRingBufferByStructure(kernel2userRingBuffer_INBOUND_STRUCTURE,
																	kernel2userRingBuffer_INBOUND_UserAddr,
																	dataBuffer, 
																	event_k2u_in
																);


			if (!success) {
				break;
			}


			//Ring buffer block structure:
			//direction 4 byte; ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			WriteBlockToRingBufferByStructure(user2kernelRingBuffer_INBOUND_STRUCTURE, 
												user2kernelRingBuffer_INBOUND_UserAddr, 
												dataBuffer, 
												event_u2k_in
											);

		} while (FALSE);

	}


	if (dataBuffer != NULL) {
		free(dataBuffer);
	}

	printf("THREAD TERMINATE\n");

}

VOID TestingRoutine2(VOID* must_null_ptr) {
	VOID* dataBuffer = malloc(RING_BUFFER_BLOCK_SIZE);

	while (1) {

		if (dataBuffer == NULL) {
			return;
		}
		do {

			BOOLEAN success = ReadBlockFromRingBufferByStructure(kernel2userRingBuffer_OUTBOUND_STRUCTURE, 
																	kernel2userRingBuffer_OUTBOUND_UserAddr,
																	dataBuffer, 
																	event_k2u_out
																);


			if (!success) {
				break;
			}

			//Ring buffer block structure:
			//direction 4 byte; ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			WriteBlockToRingBufferByStructure(user2kernelRingBuffer_OUTBOUND_STRUCTURE,
												user2kernelRingBuffer_OUTBOUND_UserAddr, 
												dataBuffer, 
												event_u2k_out
											);

		} while (FALSE);

	}

	if (dataBuffer != NULL) {
		free(dataBuffer);
	}

	printf("THREAD TERMINATE\n");

}



int main() {

	do {
		commDevice = CreateFile(WINPTABLES_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (commDevice == INVALID_HANDLE_VALUE || commDevice== NULL) {
			printf("[ERROR] winptables communicate device open failed. HANDLE_VALUE: 0x%p\n", commDevice);
			break;
		}

		printf("[OK] winptables communicate device open success. HANDLE_VALUE: 0x%p\n", commDevice);

		VOID** outBuffer = malloc(8 * sizeof(VOID*));

		if (outBuffer == NULL) {
			printf("[ERROR] Memory allocate failed.");
			break;
		}
		printf("[OK] Address buffer memory allocate success. ADDRESS: 0x%p\n", outBuffer);

		DWORD outLeng = 0;

		BOOL getMDLOk = DeviceIoControl(commDevice,
											CTL_CODE(FILE_DEVICE_UNKNOWN,
												0x801, METHOD_BUFFERED, 
												FILE_GENERIC_WRITE
											),
											NULL,
											0,
											outBuffer,
											8 * sizeof(VOID*),
											&outLeng,
											NULL
										);

		if (!getMDLOk) {
			printf("[ERROR] Get kernel mapped address failed.");
			break;
		}

		 kernel2userRingBuffer_INBOUND_UserAddr = *outBuffer;
		 kernel2userRingBuffer_OUTBOUND_UserAddr = *(outBuffer + 1);
		 user2kernelRingBuffer_INBOUND_UserAddr = *(outBuffer + 2);
		 user2kernelRingBuffer_OUTBOUND_UserAddr = *(outBuffer + 3);

		 kernel2userRingBuffer_INBOUND_STRUCTURE = *(outBuffer + 4);
		 kernel2userRingBuffer_OUTBOUND_STRUCTURE = *(outBuffer + 5);
		 user2kernelRingBuffer_INBOUND_STRUCTURE = *(outBuffer + 6);
		 user2kernelRingBuffer_OUTBOUND_STRUCTURE = *(outBuffer + 7);


		printf("[OK] Get kernel mapped address success. \n	\
			kernel2userRingBuffer_INBOUND:0x%p\n	\
			kernel2userRingBuffer_OUTBOUND:0x%p\n	\
			user2kernelRingBuffer_INBOUND:0x%p\n	\
			user2kernelRingBuffer_OUTBOUND:0x%p\n	\
			kernel2userRingBuffer_INBOUND_STRUCTURE:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n	\
			kernel2userRingBuffer_OUTBOUND_STRUCTURE:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n	\
			user2kernelRingBuffer_INBOUND_STRUCTURE:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n	\
			user2kernelRingBuffer_OUTBOUND_STRUCTURE:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n",
			kernel2userRingBuffer_INBOUND_UserAddr, 
			kernel2userRingBuffer_OUTBOUND_UserAddr, 
			user2kernelRingBuffer_INBOUND_UserAddr, 
			user2kernelRingBuffer_OUTBOUND_UserAddr,
			kernel2userRingBuffer_INBOUND_STRUCTURE,
			kernel2userRingBuffer_INBOUND_STRUCTURE->head, kernel2userRingBuffer_INBOUND_STRUCTURE->tail, kernel2userRingBuffer_INBOUND_STRUCTURE->bufferSize, kernel2userRingBuffer_INBOUND_STRUCTURE->modFactor,
			kernel2userRingBuffer_OUTBOUND_STRUCTURE,
			kernel2userRingBuffer_OUTBOUND_STRUCTURE->head, kernel2userRingBuffer_OUTBOUND_STRUCTURE->tail, kernel2userRingBuffer_OUTBOUND_STRUCTURE->bufferSize, kernel2userRingBuffer_OUTBOUND_STRUCTURE->modFactor,
			user2kernelRingBuffer_INBOUND_STRUCTURE,
			user2kernelRingBuffer_INBOUND_STRUCTURE->head, user2kernelRingBuffer_INBOUND_STRUCTURE->tail, user2kernelRingBuffer_INBOUND_STRUCTURE->bufferSize, user2kernelRingBuffer_INBOUND_STRUCTURE->modFactor,
			user2kernelRingBuffer_OUTBOUND_STRUCTURE,
			user2kernelRingBuffer_OUTBOUND_STRUCTURE->head, user2kernelRingBuffer_OUTBOUND_STRUCTURE->tail, user2kernelRingBuffer_OUTBOUND_STRUCTURE->bufferSize, user2kernelRingBuffer_OUTBOUND_STRUCTURE->modFactor
		);


		event_k2u_in = OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_INBOUND);
		if (event_k2u_in == INVALID_HANDLE_VALUE || event_k2u_in == NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (k2u_in) open failed. HANDLE_VALUE: 0x%p\n", event_k2u_in);

			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (k2u_in) open success. HANDLE_VALUE: 0x%p\n", event_k2u_in);

		event_k2u_out = OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_OUTBOUND);
		if (event_k2u_out == INVALID_HANDLE_VALUE || event_k2u_out== NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (k2u_out) open failed. HANDLE_VALUE: 0x%p\n", event_k2u_out);
			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (k2u_out) open success. HANDLE_VALUE: 0x%p\n", event_k2u_out);

		event_u2k_in = OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_INBOUND);
		if (event_u2k_in == INVALID_HANDLE_VALUE || event_u2k_in== NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (u2k_in) open failed. HANDLE_VALUE: 0x%p\n", event_u2k_in);
			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (u2k_in) open success. HANDLE_VALUE: 0x%p\n", event_u2k_in);

		event_u2k_out = OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_OUTBOUND);
		if (event_u2k_out == INVALID_HANDLE_VALUE || event_u2k_out ==NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (u2k_out) open failed. HANDLE_VALUE: 0x%p\n", event_u2k_out);
			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (u2k_out) open success. HANDLE_VALUE: 0x%p\n", event_u2k_out);

		CreateThread(NULL, 0, TestingRoutine1, NULL, 0, NULL);

		CreateThread(NULL, 0, TestingRoutine2, NULL, 0, NULL);
		
		/*memset(kernel2userRingBuffer_INBOUND_STRUCTURE_UserAddr, 0, 32);
		memset(kernel2userRingBuffer_OUTBOUND_STRUCTURE_UserAddr, 0, 32);
		memset(kernel2userRingBuffer_INBOUND_UserAddr, 0, 32);
		memset(user2kernelRingBuffer_INBOUND_UserAddr, 0, 32);*/
		int t;
		scanf_s("%d",&t);

	} while (FALSE);


	return 0;
}