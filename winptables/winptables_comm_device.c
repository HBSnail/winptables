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

extern RING_BUFFER kernel2userRingBuffer;

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

	irp->IoStatus.Status = status;


	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS WPTCommDeviceRead(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;


	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = status;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS WPTCommDeviceWrite(DEVICE_OBJECT* deviceObject, IRP* irp) {
	NTSTATUS status = STATUS_SUCCESS;

	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = status;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


