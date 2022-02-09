/*
 * File Name:		transfer_routines.c
 * Description:		The routines that transfer data between each ringbuffer
 * Date:			2022.2.9
 * Author:			HBSnail
 */

#include "global.h"
#include "transfer_routines.h"
#include "ring_buffer.h"
#include "filter_subroutines.h"

extern RING_BUFFER kernel2userRingBuffer_INBOUND;
extern RING_BUFFER kernel2userRingBuffer_OUTBOUND;
extern RING_BUFFER user2kernelRingBuffer_INBOUND;
extern RING_BUFFER user2kernelRingBuffer_OUTBOUND;


extern NPAGED_LOOKASIDE_LIST ringBufferBlockPoolList;

BOOLEAN threadFLAG;

VOID TransmitRoutine_INBOUND(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocateFromNPagedLookasideList(&ringBufferBlockPoolList);

	while (threadFLAG) {

		if (dataBuffer == NULL) {
			return;
		}
		do {

			NTSTATUS status = ReadBlockFromRingBuffer(&user2kernelRingBuffer_INBOUND, dataBuffer);


			if (!NT_SUCCESS(status)) {
				break;
			}

			//Ring buffer block structure:
			//ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			ULONG interfaceIndex = *(ULONG*)(((BYTE*)dataBuffer) + 0);
			ULONG ethLength = *(ULONG*)(((BYTE*)dataBuffer) + 4);

			FILTER_CONTEXT* fContext = GetFilterContextByMiniportInterfaceIndex(interfaceIndex);
			if (fContext == NULL) {
				break;
			}

			TransmitEthPacket(fContext, ethLength, ((BYTE*)dataBuffer) + 8, FilterToUpper, NO_FLAG);

		} while (FALSE);

	}


	if (dataBuffer != NULL) {
		ExFreeToNPagedLookasideList(&ringBufferBlockPoolList, dataBuffer);
	}


	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}

VOID TransmitRoutine_OUTBOUND(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocateFromNPagedLookasideList(&ringBufferBlockPoolList);

	while (threadFLAG) {

		if (dataBuffer == NULL) {
			return;
		}
		do {

			NTSTATUS status = ReadBlockFromRingBuffer(&user2kernelRingBuffer_OUTBOUND, dataBuffer);


			if (!NT_SUCCESS(status)) {
				break;
			}

			//Ring buffer block structure:
			//ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			ULONG interfaceIndex = *(ULONG*)(((BYTE*)dataBuffer) + 0);
			ULONG ethLength = *(ULONG*)(((BYTE*)dataBuffer) + 4);

			FILTER_CONTEXT* fContext = GetFilterContextByMiniportInterfaceIndex(interfaceIndex);
			if (fContext == NULL) {
				break;
			}

			TransmitEthPacket(fContext, ethLength, ((BYTE*)dataBuffer) + 8, FilterToNIC, NO_FLAG);

		} while (FALSE);

	}


	if (dataBuffer != NULL) {
		ExFreeToNPagedLookasideList(&ringBufferBlockPoolList, dataBuffer);
	}

	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}


VOID TestingRoutine1(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocateFromNPagedLookasideList(&ringBufferBlockPoolList);

	while (threadFLAG) {

		if (dataBuffer == NULL) {
			return;
		}
		do {

			NTSTATUS status = ReadBlockFromRingBuffer(&kernel2userRingBuffer_INBOUND, dataBuffer);


			if (!NT_SUCCESS(status)) {
				break;
			}

			//Ring buffer block structure:
			//direction 4 byte; ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			WriteBlockToRingBuffer(&user2kernelRingBuffer_INBOUND, dataBuffer);

		} while (FALSE);

	}


	if (dataBuffer != NULL) {
		ExFreeToNPagedLookasideList(&ringBufferBlockPoolList, dataBuffer);
	}

	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}

VOID TestingRoutine2(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocateFromNPagedLookasideList(&ringBufferBlockPoolList);

	while (threadFLAG) {

		if (dataBuffer == NULL) {
			return;
		}
		do {

			NTSTATUS status = ReadBlockFromRingBuffer(&kernel2userRingBuffer_OUTBOUND, dataBuffer);


			if (!NT_SUCCESS(status)) {
				break;
			}

			//Ring buffer block structure:
			//direction 4 byte; ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			WriteBlockToRingBuffer(&user2kernelRingBuffer_OUTBOUND, dataBuffer);

		} while (FALSE);

	}

	if (dataBuffer != NULL) {
		ExFreeToNPagedLookasideList(&ringBufferBlockPoolList, dataBuffer);
	}
	
	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}

NTSTATUS InitTransferRoutine(){

	NTSTATUS status = STATUS_SUCCESS;

	do {
		//Init the ring buffer which can share data with Ring3
		//20 means 1<<20 Bytes = 1MB
		//Init ring buffer with size of 1MB

		status = InitRingBuffer(&kernel2userRingBuffer_INBOUND, 20, &(UNICODE_STRING)RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\Ring0KernelEvent1"));

		if (!NT_SUCCESS(status)) {
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			break;
		}

		status = InitRingBuffer(&kernel2userRingBuffer_OUTBOUND, 20, &(UNICODE_STRING)RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\Ring0KernelEvent2"));

		if (!NT_SUCCESS(status)) {
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			break;
		}

		status = InitRingBuffer(&user2kernelRingBuffer_INBOUND, 20, &(UNICODE_STRING)RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\Ring0KernelEvent3"));

		if (!NT_SUCCESS(status)) {
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			break;
		}

		status = InitRingBuffer(&user2kernelRingBuffer_OUTBOUND, 20, &(UNICODE_STRING)RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\Ring0KernelEvent4"));

		if (!NT_SUCCESS(status)) {
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

		//Create thread handling the user2kernelRingBuffer
		threadFLAG = TRUE;
		HANDLE readingThread = NULL;
		status = PsCreateSystemThread(&readingThread, 0, NULL, NULL, NULL, (PKSTART_ROUTINE)TransmitRoutine_INBOUND, NULL);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}
		status = ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

		status = PsCreateSystemThread(&readingThread, 0, NULL, NULL, NULL, (PKSTART_ROUTINE)TransmitRoutine_OUTBOUND, NULL);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}
		status = ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}


		status = PsCreateSystemThread(&readingThread, 0, NULL, NULL, NULL, (PKSTART_ROUTINE)TestingRoutine1, NULL);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}
		status = ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

		status = PsCreateSystemThread(&readingThread, 0, NULL, NULL, NULL, (PKSTART_ROUTINE)TestingRoutine2, NULL);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}
		status = ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			threadFLAG = FALSE;
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

	} while (FALSE);

	return status;

}