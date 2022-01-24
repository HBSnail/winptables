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

extern RING_BUFFER commRingBuffer;

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
		UINT packetLength = ReadEthFrameFromRingBuffer(&commRingBuffer, kernelAddr);
		irp->IoStatus.Information = packetLength;
	}

	irp->IoStatus.Status = status;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS WPTCommDeviceWrite(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;

	IO_STACK_LOCATION* stack = IoGetCurrentIrpStackLocation(irp);
	ULONG writeLength = stack->Parameters.Write.Length;
	ULONG mdlLength = MmGetMdlByteCount(irp->MdlAddress);
	irp->IoStatus.Information = 0;
	do {

		if (mdlLength != writeLength) {
			break;
		}

		BYTE* kernelAddr = (BYTE*)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);

		if (kernelAddr == NULL) {
			break;
		}

		BYTE direction;
		ULONG interfaceIndex;
		UINT ethLength;
		NdisMoveMemory(&direction, kernelAddr, sizeof(BYTE));
		NdisMoveMemory(&interfaceIndex, kernelAddr + sizeof(BYTE), sizeof(ULONG));
		NdisMoveMemory(&ethLength, kernelAddr + sizeof(BYTE) + sizeof(ULONG), sizeof(UINT));
		FILTER_CONTEXT* fContext = GetFilterContextByMiniportInterfaceIndex(interfaceIndex);
		if (fContext == NULL) {
			break;
		}
		TransmitEthPacket(fContext, ethLength, kernelAddr + sizeof(BYTE) + sizeof(ULONG) + sizeof(UINT), direction, NO_FLAG);
		irp->IoStatus.Information = sizeof(BYTE) + sizeof(ULONG), sizeof(UINT) + ethLength;

	} while (FALSE);


	irp->IoStatus.Status = status;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


