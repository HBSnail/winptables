/*
 * File Name:		winptables_comm_device.c
 * Description:		The subroutines for handling the device IRP requests and communicate request from Ring3
 * Date:			2022.1.14
 * Author:			HBSnail
 */

#include "global.h"
#include "winptables_comm_device.h"
#include "ring_buffer.h"
#include "filter_subroutines.h"

extern NDIS_HANDLE filterDriverHandle;
extern RING_BUFFER kernel2userRingBuffer_INBOUND;
extern RING_BUFFER kernel2userRingBuffer_OUTBOUND;
extern RING_BUFFER user2kernelRingBuffer_INBOUND;
extern RING_BUFFER user2kernelRingBuffer_OUTBOUND;

NTSTATUS WPTCommDeviceCreate(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;


	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS WPTCommDeviceClose(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS WPTCommDeviceClean(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS WPTCommDeviceIOCtl(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;

	irp->IoStatus.Status = STATUS_SUCCESS;

	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS WPTCommDeviceRead(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;

	IO_STACK_LOCATION* stack = IoGetCurrentIrpStackLocation(irp);
	ULONG readLength = stack->Parameters.Read.Length;
	ULONG mdlLength = MmGetMdlByteCount(irp->MdlAddress);

	if (mdlLength != readLength) {
		irp->IoStatus.Information = 0;
	}
	else {
		BYTE* kernelAddr = (BYTE*)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	
		if (kernelAddr != NULL) {

			MDL* kernel2userRingBuffer_INBOUND_MDL = NdisAllocateMdl(filterDriverHandle, kernel2userRingBuffer_INBOUND.bufferAddress, kernel2userRingBuffer_INBOUND.bufferSize);
			MDL* kernel2userRingBuffer_OUTBOUND_MDL = NdisAllocateMdl(filterDriverHandle, kernel2userRingBuffer_OUTBOUND.bufferAddress, kernel2userRingBuffer_OUTBOUND.bufferSize);
			MDL* user2kernelRingBuffer_INBOUND_MDL = NdisAllocateMdl(filterDriverHandle, user2kernelRingBuffer_INBOUND.bufferAddress, user2kernelRingBuffer_INBOUND.bufferSize);
			MDL* user2kernelRingBuffer_OUTBOUND_MDL = NdisAllocateMdl(filterDriverHandle, user2kernelRingBuffer_OUTBOUND.bufferAddress, user2kernelRingBuffer_OUTBOUND.bufferSize);
			VOID* kernel2userRingBuffer_INBOUND_UserAddr = MmMapLockedPagesSpecifyCache(kernel2userRingBuffer_INBOUND_MDL, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
			VOID* kernel2userRingBuffer_OUTBOUND_UserAddr = MmMapLockedPagesSpecifyCache(kernel2userRingBuffer_OUTBOUND_MDL, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
			VOID* user2kernelRingBuffer_INBOUND_UserAddr = MmMapLockedPagesSpecifyCache(user2kernelRingBuffer_INBOUND_MDL, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
			VOID* user2kernelRingBuffer_OUTBOUND_UserAddr = MmMapLockedPagesSpecifyCache(user2kernelRingBuffer_OUTBOUND_MDL, UserMode, MmCached, NULL, FALSE, NormalPagePriority);

			NdisMoveMemory(kernelAddr + sizeof(VOID*) * 0, &kernel2userRingBuffer_INBOUND_UserAddr, sizeof(VOID*));
			NdisMoveMemory(kernelAddr + sizeof(VOID*) * 1, &kernel2userRingBuffer_OUTBOUND_UserAddr, sizeof(VOID*));
			NdisMoveMemory(kernelAddr + sizeof(VOID*) * 2, &user2kernelRingBuffer_INBOUND_UserAddr, sizeof(VOID*));
			NdisMoveMemory(kernelAddr + sizeof(VOID*) * 3, &user2kernelRingBuffer_OUTBOUND_UserAddr, sizeof(VOID*));
		}

		irp->IoStatus.Information = sizeof(VOID*) * 4 ;
	}

	irp->IoStatus.Status = status;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS WPTCommDeviceWrite(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;

	IO_STACK_LOCATION* stack = IoGetCurrentIrpStackLocation(irp);
	

	irp->IoStatus.Status = status;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}
