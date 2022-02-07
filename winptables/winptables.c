/*
 * File Name:		winptables.c
 * Description:		The entry point and driver dispatch subroutines for the winptables NDIS Driver
 * Date:			2022.1.14
 * Author:			HBSnail
 */

#include "global.h"
#include "filter_subroutines.h"
#include "ring_buffer.h"
#include "winptables_comm_device.h"


NDIS_HANDLE filterDriverHandle = NULL;
NDIS_HANDLE filterDriverObject = NULL;
NDIS_SPIN_LOCK filterListLock;
LIST_ENTRY filterModuleList;


RING_BUFFER kernel2userRingBuffer_INBOUND;
RING_BUFFER kernel2userRingBuffer_OUTBOUND;
RING_BUFFER user2kernelRingBuffer_INBOUND;
RING_BUFFER user2kernelRingBuffer_OUTBOUND;

UNICODE_STRING deviceName;
UNICODE_STRING linkName;
DEVICE_OBJECT* winptablesCommunicationDevice;


VOID TransmitRoutine_INBOUND(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocatePoolWithTag(NonPagedPool, RING_BUFFER_BLOCK_SIZE, TEMP_POOL_ALLOC_TAG);
	
	while (1) {

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

			ULONG interfaceIndex = *(ULONG*)(((BYTE*)dataBuffer) + 4);
			ULONG ethLength = *(ULONG*)(((BYTE*)dataBuffer) + 8);

			FILTER_CONTEXT* fContext = GetFilterContextByMiniportInterfaceIndex(interfaceIndex);
			if (fContext == NULL) {
				break;
			}

			TransmitEthPacket(fContext, ethLength, ((BYTE*)dataBuffer) + 12, FilterToUpper, NO_FLAG);

		} while (FALSE);

	}

	ExFreePoolWithTag(dataBuffer, TEMP_POOL_ALLOC_TAG);
	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}

VOID TransmitRoutine_OUTBOUND(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocatePoolWithTag(NonPagedPool, RING_BUFFER_BLOCK_SIZE, TEMP_POOL_ALLOC_TAG);

	while (1) {

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

			ULONG interfaceIndex = *(ULONG*)(((BYTE*)dataBuffer) + 4);
			ULONG ethLength = *(ULONG*)(((BYTE*)dataBuffer) + 8);

			FILTER_CONTEXT* fContext = GetFilterContextByMiniportInterfaceIndex(interfaceIndex);
			if (fContext == NULL) {
				break;
			}

			TransmitEthPacket(fContext, ethLength, ((BYTE*)dataBuffer) + 12, FilterToNIC, NO_FLAG);

		} while (FALSE);

	}

	ExFreePoolWithTag(dataBuffer, TEMP_POOL_ALLOC_TAG);
	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}


VOID TestingRoutine1(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocatePoolWithTag(NonPagedPool, RING_BUFFER_BLOCK_SIZE, TEMP_POOL_ALLOC_TAG);

	while (1) {

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

	ExFreePoolWithTag(dataBuffer, TEMP_POOL_ALLOC_TAG);
	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}

VOID TestingRoutine2(VOID* must_null_ptr) {
	VOID* dataBuffer = ExAllocatePoolWithTag(NonPagedPool, RING_BUFFER_BLOCK_SIZE, TEMP_POOL_ALLOC_TAG);

	while (1) {

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

	ExFreePoolWithTag(dataBuffer, TEMP_POOL_ALLOC_TAG);
	DbgPrint("THREAD TERMINATE\n");
	NTSTATUS s = PsTerminateSystemThread(STATUS_SUCCESS);
	DbgPrint("THREAD TERMINATE %d\n", s);

}



/*++

Routine Description:

	Driver unload routine.
	Free all resources used by program

Arguments:

	driverObject - the driver object

Return Value:

	NULL

--*/
VOID DriverUnload(DRIVER_OBJECT* driverObject) {

	DbgPrint("DriverUnload\n");

	NdisFDeregisterFilterDriver(filterDriverHandle);

	IoDeleteSymbolicLink(&linkName);

	IoDeleteDevice(winptablesCommunicationDevice);

	FreeRingBuffer(&kernel2userRingBuffer_INBOUND);

	FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);

	FreeRingBuffer(&user2kernelRingBuffer_INBOUND);

	FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);

	return;
}

/*++

Routine Description:

	Driver entry point.

Arguments:

	DRIVER_OBJECT* driverObject 
	UNICODE_STRING* registryPath

Return Value:

	NTSTATUS - indecate if the driver create all necessary resources success and ok for running.

--*/
NTSTATUS DriverEntry(DRIVER_OBJECT* driverObject, UNICODE_STRING* registryPath) {


	DbgPrint("DriverEntry\n");

	NTSTATUS status = STATUS_SUCCESS;

	do {

		//Check the NDIS version
		//winptables only support NDIS_VERSION >= 6.20(win7&server2008)
		ULONG ndisVersion = NdisGetVersion();
		if (ndisVersion < NDIS_RUNTIME_VERSION_620)
		{
			status = NDIS_STATUS_UNSUPPORTED_REVISION;
			break;
		}

		//Prepare the variables used in NDIS filter driver registration
		filterDriverObject = driverObject;

		NDIS_FILTER_DRIVER_CHARACTERISTICS filterDriverCharacteristics;

		NdisZeroMemory(&filterDriverCharacteristics, sizeof(filterDriverCharacteristics));

		filterDriverCharacteristics.Header.Type = NDIS_OBJECT_TYPE_FILTER_DRIVER_CHARACTERISTICS;
		filterDriverCharacteristics.Header.Size = sizeof(filterDriverCharacteristics);
		filterDriverCharacteristics.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_2;

		filterDriverCharacteristics.MajorNdisVersion = NDIS_MAJOR_VERSION;
		filterDriverCharacteristics.MinorNdisVersion = NDIS_MINOR_VERSION;

		filterDriverCharacteristics.MajorDriverVersion = DRIVER_MAJOR_VERSION;
		filterDriverCharacteristics.MinorDriverVersion = DRIVER_MINOR_VERSION;

		filterDriverCharacteristics.Flags = 0;

		filterDriverCharacteristics.FriendlyName = (NDIS_STRING)RTL_CONSTANT_STRING(FILTER_FRIENDLY_NAME);
		filterDriverCharacteristics.UniqueName = (NDIS_STRING)RTL_CONSTANT_STRING(FILTER_GUID);
		filterDriverCharacteristics.ServiceName = (NDIS_STRING)RTL_CONSTANT_STRING(FILTER_SERVICE_NAME);

		//Set dispatch subroutines (include in file filter_subroutines.c)
		filterDriverCharacteristics.SetOptionsHandler = WPTFilterSetOptions;
		filterDriverCharacteristics.SetFilterModuleOptionsHandler = WPTFilterSetModuleOptions;
		filterDriverCharacteristics.AttachHandler = WPTFilterAttach;
		filterDriverCharacteristics.DetachHandler = WPTFilterDetach;
		filterDriverCharacteristics.RestartHandler = WPTFilterRestart;
		filterDriverCharacteristics.PauseHandler = WPTFilterPause;
		filterDriverCharacteristics.ReceiveNetBufferListsHandler = WPTReceivedFromNIC; //When NIC receive data from the wire this subroutine will be invoked
		filterDriverCharacteristics.ReturnNetBufferListsHandler = WPTSendToUpperFinished; //When the data from NIC to the upper finished processing by the upper NDIS protocol driver this subroutine will be invoked
		filterDriverCharacteristics.SendNetBufferListsHandler = WPTReceivedFromUpper; //When upper NDIS protocol driver wants to send packet this subroutine will be invoked
		filterDriverCharacteristics.SendNetBufferListsCompleteHandler = WPTSendToNICFinished;//When the data upper NDIS protocol driver send finished processing by the NIC this subroutine will be invoked

		//Set other not used dispatch subroutines to NULL
		filterDriverCharacteristics.OidRequestHandler = NULL;
		filterDriverCharacteristics.OidRequestCompleteHandler = NULL;
		filterDriverCharacteristics.CancelOidRequestHandler = NULL;
		filterDriverCharacteristics.DevicePnPEventNotifyHandler = NULL;
		filterDriverCharacteristics.NetPnPEventHandler = NULL;
		filterDriverCharacteristics.StatusHandler = NULL;
		filterDriverCharacteristics.CancelSendNetBufferListsHandler = NULL;

		//Init the write lock for filterModuleList and link list head
		NdisAllocateSpinLock(&filterListLock);
		InitializeListHead(&filterModuleList);

		//Register the NDIS filter driver
		status = NdisFRegisterFilterDriver(driverObject,filterDriverObject,&filterDriverCharacteristics,&filterDriverHandle);

		//Check if the NDIS filter driver registered successful
		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			break;
		}

		//Init driver dispatch subroutine
		driverObject->DriverUnload = DriverUnload;

		//Create the device to communicate with Ring3 
		RtlInitUnicodeString(&deviceName, WINPTABLES_COMMUNICATION_DEVICE_NAME);
		status = IoCreateDevice(driverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &winptablesCommunicationDevice);

		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			break;
		}

		//Use direct IO instead of buffering IO for high speed
		winptablesCommunicationDevice->Flags |= DO_DIRECT_IO;

		//Create a symbolic link for the device
		RtlInitUnicodeString(&linkName, WINPTABLES_COMMUNICATION_DEVICE_LINK);
		status = IoCreateSymbolicLink(&linkName, &deviceName);
		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			break;
		}

		//Set the IRP dispatch subroutines for driver
		driverObject->MajorFunction[IRP_MJ_CREATE] = WPTCommDeviceCreate;
		driverObject->MajorFunction[IRP_MJ_CLOSE] = WPTCommDeviceClose;
		driverObject->MajorFunction[IRP_MJ_CLEANUP] = WPTCommDeviceClean;
		driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WPTCommDeviceIOCtl;
		driverObject->MajorFunction[IRP_MJ_READ] = WPTCommDeviceRead;
		driverObject->MajorFunction[IRP_MJ_WRITE] = WPTCommDeviceWrite;

		//Init the ring buffer which can share data with Ring3
		//20 means 1<<20 Bytes = 1MB
		//Init ring buffer with size of 1MB
		status = InitRingBuffer(&kernel2userRingBuffer_INBOUND, 20);

		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			break;
		}

		status = InitRingBuffer(&kernel2userRingBuffer_OUTBOUND, 20);

		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			break;
		}

		status = InitRingBuffer(&user2kernelRingBuffer_INBOUND, 20);

		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			break;
		}

		status = InitRingBuffer(&user2kernelRingBuffer_OUTBOUND, 20);

		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

		
		//Create thread handling the user2kernelRingBuffer
		HANDLE readingThread = NULL ;
		status = PsCreateSystemThread(&readingThread,0,NULL,NULL,NULL, (PKSTART_ROUTINE)TransmitRoutine_INBOUND, NULL);
		status =  ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

		status = PsCreateSystemThread(&readingThread, 0, NULL, NULL, NULL, (PKSTART_ROUTINE)TransmitRoutine_OUTBOUND, NULL);
		status =  ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}


		status = PsCreateSystemThread(&readingThread, 0, NULL, NULL, NULL, (PKSTART_ROUTINE)TestingRoutine1, NULL);
		status = ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

		status = PsCreateSystemThread(&readingThread, 0, NULL, NULL, NULL, (PKSTART_ROUTINE)TestingRoutine2, NULL);
		status = ZwClose(readingThread);
		if (!NT_SUCCESS(status)) {
			NdisFreeSpinLock(&filterListLock);
			NdisFDeregisterFilterDriver(filterDriverHandle);
			IoDeleteDevice(winptablesCommunicationDevice);
			FreeRingBuffer(&kernel2userRingBuffer_INBOUND);
			FreeRingBuffer(&kernel2userRingBuffer_OUTBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_INBOUND);
			FreeRingBuffer(&user2kernelRingBuffer_OUTBOUND);
			break;
		}

		


	} while (FALSE);

	return status;
}
