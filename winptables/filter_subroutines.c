/*
 * File Name:		filter_subroutines.c
 * Description:		The subroutines for handling NDIS filter
 * Date:			2022.1.14
 * Author:			HBSnail
 */


#include "global.h"
#include "filter_subroutines.h"
#include "ring_buffer.h"

extern NDIS_HANDLE filterDriverHandle;
extern NDIS_HANDLE filterDriverObject;
extern NDIS_SPIN_LOCK filterListLock;
extern LIST_ENTRY filterModuleList;
extern RING_BUFFER kernel2userRingBuffer;

NTSTATUS TransmitEthPacket(FILTER_CONTEXT* filterContext, UINT length, BYTE* ethDataPtr, TRANSFER_DIRECION direction, ULONG flag) {

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do {

		VOID* ethMDLSpace = ExAllocatePoolWithTag(NonPagedPool, length, ETH_FRAME_POOL_ALLOC_TAG);
		if (ethMDLSpace == NULL) {
			break;
		}
		MDL* ethMDL = NdisAllocateMdl(filterContext->filterHandle, ethMDLSpace, length);
		if (ethMDL == NULL) {
			ExFreePoolWithTag(ethMDLSpace, ETH_FRAME_POOL_ALLOC_TAG);
			break;
		}
		NdisMoveMemory(ethMDLSpace, ethDataPtr, length);
		NET_BUFFER_LIST* iNBLfromEthpacket = NdisAllocateNetBufferAndNetBufferList(filterContext->sendNetBufferListPool, 0, 0, ethMDL, 0, length);
		if (iNBLfromEthpacket == NULL) {
			ExFreePoolWithTag(ethMDLSpace, ETH_FRAME_POOL_ALLOC_TAG);
			NdisFreeMdl(ethMDL);
			break;
		}
		iNBLfromEthpacket->SourceHandle = filterContext->filterHandle;

		if ((BYTE)direction == (BYTE)FilterToNIC) {
			status = STATUS_SUCCESS;
			NdisFSendNetBufferLists(filterContext->filterHandle, iNBLfromEthpacket, NDIS_DEFAULT_PORT_NUMBER, flag);
		}
		else if ((BYTE)direction == (BYTE)FilterToUpper) {
			status = STATUS_SUCCESS;
			NdisFIndicateReceiveNetBufferLists(filterContext->filterHandle, iNBLfromEthpacket, NDIS_DEFAULT_PORT_NUMBER, 1, flag);
		}
		else {
			ExFreePoolWithTag(ethMDLSpace, ETH_FRAME_POOL_ALLOC_TAG);
			NdisFreeMdl(ethMDL);
			break;
		}

	} while (FALSE);

	return status;
}

VOID WPTFreeNBL(NET_BUFFER_LIST* NetBufferLists) {

	NET_BUFFER_LIST* pNetBufList = NetBufferLists;
	NET_BUFFER* currentBuffer;
	MDL* pMDL;
	VOID* npBuffer;

	currentBuffer = NET_BUFFER_LIST_FIRST_NB(pNetBufList);
	while (currentBuffer != NULL)
	{
		pMDL = NET_BUFFER_FIRST_MDL(currentBuffer);
		npBuffer = MmGetSystemAddressForMdlSafe(pMDL, HighPagePriority | MdlMappingNoExecute);
		if (npBuffer != NULL) {
			ExFreePoolWithTag(npBuffer, ETH_FRAME_POOL_ALLOC_TAG);
		}
		NdisFreeMdl(pMDL); //Free MDL
		currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
	}

	NdisFreeNetBufferList(pNetBufList);

}

VOID WriteNBLIntoRingBuffer(RING_BUFFER* ringBuffer ,NET_BUFFER_LIST*  netBufferLists, TRANSFER_DIRECION direction,ULONG ifIndex) {

	NET_BUFFER_LIST* currentNBL = netBufferLists;

	//Get all netbuffer structure in NBLs
	while (currentNBL != NULL) {

		for (NET_BUFFER* netbuffer = NET_BUFFER_LIST_FIRST_NB(currentNBL); netbuffer != NULL; netbuffer = NET_BUFFER_NEXT_NB(netbuffer)) {

			if (netbuffer->DataLength >= (RING_BUFFER_BLOCK_SIZE - 12)) {
				//The frame is TOO LARGE
				continue;
			}

			VOID* freeRingBufferBlock = ExAllocatePoolWithTag(NonPagedPool, RING_BUFFER_BLOCK_SIZE, TEMP_POOL_ALLOC_TAG);

			if (freeRingBufferBlock == NULL) {
				//Memory ALLOC FAILED
				continue;
			}

			NdisZeroMemory(freeRingBufferBlock, RING_BUFFER_BLOCK_SIZE);


			VOID* ethDataPtr = NdisGetDataBuffer(netbuffer, netbuffer->DataLength, (BYTE*)freeRingBufferBlock + 12, 1, 0);
			//The data in NBL is contiguous system will not auto copy the data, we shouled copy manually
			if (ethDataPtr != ((BYTE*)freeRingBufferBlock + 12)) {
				NdisMoveMemory((BYTE*)freeRingBufferBlock + 12, ethDataPtr, netbuffer->DataLength);
			}


			//Ring buffer block structure:
			//direction 4 byte; ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending ....
			*((ULONG*)((BYTE*)freeRingBufferBlock + 0)) = (ULONG)direction;
			*((ULONG*)((BYTE*)freeRingBufferBlock + 4)) = (ULONG)ifIndex;
			*((ULONG*)((BYTE*)freeRingBufferBlock + 8)) = (ULONG)netbuffer->DataLength;


			WriteBlockToRingBuffer(ringBuffer, freeRingBufferBlock);

			ExFreePoolWithTag(freeRingBufferBlock, TEMP_POOL_ALLOC_TAG);
			freeRingBufferBlock = NULL;
			ethDataPtr = NULL;


		}
		currentNBL = NET_BUFFER_LIST_NEXT_NBL(currentNBL);
	}

}

FILTER_CONTEXT* GetFilterContextByMiniportInterfaceIndex(ULONG index) {

	for (LIST_ENTRY* p = filterModuleList.Flink; p != &filterModuleList; p = p->Flink) {

		FILTER_CONTEXT* context = CONTAINING_RECORD(p, FILTER_CONTEXT, filterModuleLink);

		if (context->miniportIfIndex == index) {

			return context;
		}

	}

	return NULL;
}


NDIS_STATUS WPTFilterSetOptions(NDIS_HANDLE  ndisFilterDriverHandle, NDIS_HANDLE  filterDriverContext) {

	if ((ndisFilterDriverHandle != (NDIS_HANDLE)filterDriverHandle) || (filterDriverContext != (NDIS_HANDLE)filterDriverObject))
	{
		return NDIS_STATUS_INVALID_PARAMETER;
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS WPTFilterSetModuleOptions(NDIS_HANDLE filterModuleContext) {

	NDIS_STATUS status = NDIS_STATUS_SUCCESS;

	return status;
}



NDIS_STATUS WPTFilterAttach(NDIS_HANDLE ndisfilterHandle, NDIS_HANDLE filterDriverContext, NDIS_FILTER_ATTACH_PARAMETERS* attachParameters) {

	DbgPrint("WPTFilterAttach\n");

	FILTER_CONTEXT* filterContext = NULL;
	NDIS_STATUS status = NDIS_STATUS_SUCCESS;
	NDIS_FILTER_ATTRIBUTES filterAttributes;
	ULONG size;

	do
	{
		if (filterDriverContext != (NDIS_HANDLE)filterDriverObject) {
			status = NDIS_STATUS_INVALID_PARAMETER;
			break;
		}

		// Verify the media type is supported.  This is a last resort; the
		// the filter should never have been bound to an unsupported miniport
		// to begin with.  If this driver is marked as a Mandatory filter (which
		// is the default for this sample; see the INF file), failing to attach
		// here will leave the network adapter in an unusable state.
		//
		// Your setup/install code should not bind the filter to unsupported
		// media types.

		if ((attachParameters->MiniportMediaType != NdisMedium802_3)
			&& (attachParameters->MiniportMediaType != NdisMediumWan)
			&& (attachParameters->MiniportMediaType != NdisMediumWirelessWan)) {

			status = NDIS_STATUS_INVALID_PARAMETER;
			break;

		}

		size = sizeof(FILTER_CONTEXT) +
			attachParameters->FilterModuleGuidName->Length +
			attachParameters->BaseMiniportInstanceName->Length +
			attachParameters->BaseMiniportName->Length;

		filterContext = (FILTER_CONTEXT*)NdisAllocateMemoryWithTagPriority(ndisfilterHandle, size, FILTER_ALLOC_TAG, NormalPoolPriority);
		if (filterContext == NULL) {
			status = NDIS_STATUS_RESOURCES;
			break;
		}

		NdisZeroMemory(filterContext, sizeof(FILTER_CONTEXT));

		filterContext->filterModuleName.Length = filterContext->filterModuleName.MaximumLength = attachParameters->FilterModuleGuidName->Length;
		filterContext->filterModuleName.Buffer = (PWSTR)((PUCHAR)filterContext + sizeof(FILTER_CONTEXT));
		NdisMoveMemory(filterContext->filterModuleName.Buffer,
			attachParameters->FilterModuleGuidName->Buffer,
			filterContext->filterModuleName.Length);

		NdisMoveMemory(filterContext->currentMacAddress, attachParameters->CurrentMacAddress, NDIS_MAX_PHYS_ADDRESS_LENGTH);
		filterContext->macLength = (BYTE)(attachParameters->MacAddressLength & 0xFF);



		filterContext->miniportFriendlyName.Length = filterContext->miniportFriendlyName.MaximumLength = attachParameters->BaseMiniportInstanceName->Length;
		filterContext->miniportFriendlyName.Buffer = (PWSTR)((PUCHAR)filterContext->filterModuleName.Buffer + filterContext->filterModuleName.Length);
		NdisMoveMemory(filterContext->miniportFriendlyName.Buffer,
			attachParameters->BaseMiniportInstanceName->Buffer,
			filterContext->miniportFriendlyName.Length);


		filterContext->miniportName.Length = filterContext->miniportName.MaximumLength = attachParameters->BaseMiniportName->Length;
		filterContext->miniportName.Buffer = (PWSTR)((PUCHAR)filterContext->miniportFriendlyName.Buffer + filterContext->miniportFriendlyName.Length);
		NdisMoveMemory(filterContext->miniportName.Buffer, attachParameters->BaseMiniportName->Buffer, filterContext->miniportName.Length);

		filterContext->miniportIfIndex = attachParameters->BaseMiniportIfIndex;

		// The filter should initialize TrackReceives and TrackSends properly. For this
		// driver, since its default characteristic has both a send and a receive handler,
		// these fields are initialized to TRUE.

		filterContext->filterHandle = ndisfilterHandle;


		NET_BUFFER_LIST_POOL_PARAMETERS PoolParameters;
		NdisZeroMemory(&PoolParameters, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));
		PoolParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
		PoolParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		PoolParameters.Header.Size = sizeof(PoolParameters);
		PoolParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
		PoolParameters.ContextSize = MEMORY_ALLOCATION_ALIGNMENT;
		PoolParameters.fAllocateNetBuffer = TRUE;
		PoolParameters.PoolTag = SEND_POOL_ALLOC_TAG;
		filterContext->sendNetBufferListPool = NdisAllocateNetBufferListPool(filterContext->filterHandle, &PoolParameters);

		//TODO: disable the offloads converient for future process

		/*
		NDIS_OFFLOAD_PARAMETERS disableAllOffload;
		NdisZeroMemory(&disableAllOffload, sizeof(NDIS_OFFLOAD_PARAMETERS));
		disableAllOffload.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
		disableAllOffload.Header.Revision = NDIS_OFFLOAD_PARAMETERS_REVISION_2;
		disableAllOffload.Header.Size = sizeof(disableAllOffload);
		disableAllOffload.IPv4Checksum = NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED;
		disableAllOffload.TCPIPv4Checksum = NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED;
		disableAllOffload.UDPIPv4Checksum = NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED;
		disableAllOffload.TCPIPv6Checksum = NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED;
		disableAllOffload.UDPIPv6Checksum = NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED;
		disableAllOffload.TcpConnectionIPv4 = NDIS_OFFLOAD_PARAMETERS_NO_CHANGE;
		disableAllOffload.TcpConnectionIPv6 = NDIS_OFFLOAD_PARAMETERS_NO_CHANGE;
		*/


		if (filterContext->sendNetBufferListPool == NULL) {
			status = NDIS_STATUS_BAD_CHARACTERISTICS;
			break;
		}

		NdisZeroMemory(&filterAttributes, sizeof(NDIS_FILTER_ATTRIBUTES));
		filterAttributes.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
		filterAttributes.Header.Size = sizeof(NDIS_FILTER_ATTRIBUTES);
		filterAttributes.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
		filterAttributes.Flags = 0;


		NDIS_DECLARE_FILTER_MODULE_CONTEXT(FILTER_CONTEXT);
		status = NdisFSetAttributes(ndisfilterHandle,
			filterContext,
			&filterAttributes);
		if (status != NDIS_STATUS_SUCCESS) {
			break;
		}

		filterContext->filterState = FilterPaused;

		NDIS_ACQUIRE_LOCK(&filterListLock, FALSE);
		InsertHeadList(&filterModuleList, &filterContext->filterModuleLink);
		NDIS_RELEASE_LOCK(&filterListLock, FALSE);


	} while (FALSE);

	if (status != NDIS_STATUS_SUCCESS) {
		if (filterContext != NULL) {
			NdisFreeMemory(filterContext, 0, 0);
			filterContext = NULL;
		}
	}

	return status;
}


NDIS_STATUS WPTFilterPause(NDIS_HANDLE filterModuleContext, NDIS_FILTER_PAUSE_PARAMETERS* pauseParameters) {

	DbgPrint("WPTFilterPause\n");

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)(filterModuleContext);
	NDIS_STATUS status;

	// Set the flag that the filter is going to pause
	NDIS_ACQUIRE_LOCK(&filterContext->filterLock, FALSE);
	filterContext->filterState = FilterPausing;
	NDIS_RELEASE_LOCK(&filterContext->filterLock, FALSE);

	status = NDIS_STATUS_SUCCESS;

	filterContext->filterState = FilterPaused;

	return status;
}


NDIS_STATUS WPTFilterRestart(NDIS_HANDLE filterModuleContext, NDIS_FILTER_RESTART_PARAMETERS* RestartParameters) {

	DbgPrint("WPTFilterRestart\n");

	NDIS_STATUS status;
	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;


	NDIS_RESTART_GENERAL_ATTRIBUTES* ndisGeneralAttributes;
	NDIS_RESTART_ATTRIBUTES* ndisRestartAttributes;

	ndisRestartAttributes = RestartParameters->RestartAttributes;


	// If NdisRestartAttributes is not NULL, then the filter can modify generic
	// attributes and add new media specific info attributes at the end.
	// Otherwise, if NdisRestartAttributes is NULL, the filter should not try to
	// modify/add attributes.
	if (ndisRestartAttributes != NULL)
	{
		NDIS_RESTART_ATTRIBUTES* NextAttributes;

		ndisGeneralAttributes = (PNDIS_RESTART_GENERAL_ATTRIBUTES)ndisRestartAttributes->Data;


		// Check to see if we need to change any attributes. For example, the
		// driver can change the current MAC address here. Or the driver can add
		// media specific info attributes.
		ndisGeneralAttributes->LookaheadSize = 128;


		// Check each attribute to see whether the filter needs to modify it.
		NextAttributes = ndisRestartAttributes->Next;

		while (NextAttributes != NULL)
		{
			NextAttributes = NextAttributes->Next;
		}
	}


	// If everything is OK, set the filter in running state.
	filterContext->filterState = FilterRunning; // when successful


	status = NDIS_STATUS_SUCCESS;


	// Ensure the state is Paused if restart failed.
	if (status != NDIS_STATUS_SUCCESS)
	{
		filterContext->filterState = FilterPaused;
	}

	return status;
}


VOID WPTFilterDetach(NDIS_HANDLE filterModuleContext) {

	DbgPrint("WPTFilterDetach\n");

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;


	// Detach must not fail, so do not put any code here that can possibly fail.

	// Free filter instance name if allocated.
	if (filterContext->filterName.Buffer != NULL)
	{
		NdisFreeMemory(filterContext->filterName.Buffer, 0, 0);
		filterContext->filterName.Buffer = NULL;
	}

	// Free filter NetBufferListPool if allocated.
	if (filterContext->sendNetBufferListPool != NULL)
	{
		NdisFreeNetBufferListPool(filterContext->sendNetBufferListPool);

		filterContext->sendNetBufferListPool = NULL;
	}


	NDIS_ACQUIRE_LOCK(&filterListLock, FALSE);
	RemoveEntryList(&filterContext->filterModuleLink);
	NDIS_RELEASE_LOCK(&filterListLock, FALSE);


	// Free the memory allocated
	NdisFreeMemory(filterContext, 0, 0);
	filterContext = NULL;
	return;
}


VOID WPTReceivedFromNIC(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG numberOfNetBufferLists, ULONG receiveFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;
	BOOLEAN dispatchLevel;
	ULONG returnFlags;

	do
	{
		dispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(receiveFlags);

		NDIS_ACQUIRE_LOCK(&filterContext->filterLock, dispatchLevel);

		returnFlags = 0;
		if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(receiveFlags))
		{
			NDIS_SET_RETURN_FLAG(returnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
		}

		if (filterContext->filterState != FilterRunning)
		{
			NDIS_RELEASE_LOCK(&filterContext->filterLock, dispatchLevel);

			if (NDIS_TEST_RECEIVE_CAN_PEND(receiveFlags))
			{

				NdisFReturnNetBufferLists(filterContext->filterHandle, netBufferLists, returnFlags);

			}
			break;
		}
		NDIS_RELEASE_LOCK(&filterContext->filterLock, dispatchLevel);

		//No NBLs
		if (numberOfNetBufferLists < 1) {
			break;
		}

		
		WriteNBLIntoRingBuffer(&kernel2userRingBuffer,netBufferLists, NICToFilter, filterContext->miniportIfIndex);

		
		//NdisFReturnNetBufferLists(filterContext->filterHandle, netBufferLists, receiveFlags);
		NdisFIndicateReceiveNetBufferLists(filterContext->filterHandle, netBufferLists, portNumber, numberOfNetBufferLists, receiveFlags);

	} while (FALSE);

}

VOID WPTSendToUpperFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG returnFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;

	NET_BUFFER_LIST* pNetBufList;
	NET_BUFFER_LIST* pNextNetBufList;

	BOOLEAN dispatchLevel;

	dispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(returnFlags);


	pNetBufList = netBufferLists;

	while (pNetBufList != NULL)
	{
		pNextNetBufList = NET_BUFFER_LIST_NEXT_NBL(pNetBufList);
		NET_BUFFER_LIST_NEXT_NBL(pNetBufList) = NULL;

		if (pNetBufList->SourceHandle == filterContext->filterHandle)
		{
			//If the NBL has our tag, FREE it.
			WPTFreeNBL(pNetBufList);

		}
		else
		{
			//Not our NBLs. Return the NBLs. 
			NdisFReturnNetBufferLists(filterContext->filterHandle, pNetBufList, returnFlags);
		}

		pNetBufList = pNextNetBufList;
	}


}


VOID WPTReceivedFromUpper(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG sendFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;

	
	BOOLEAN dispatchLevel;

	do
	{

		dispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(sendFlags);

		// We should never get packets to send if we are not in running state
		// If the filter is not in running state, fail the send
		if (filterContext->filterState != FilterRunning)
		{
			NET_BUFFER_LIST* currentNBL = netBufferLists;
			while (currentNBL != NULL)
			{
				NET_BUFFER_LIST_STATUS(currentNBL) = NDIS_STATUS_PAUSED;
				currentNBL = NET_BUFFER_LIST_NEXT_NBL(currentNBL);
			}
			NdisFSendNetBufferListsComplete(filterContext->filterHandle,
				netBufferLists,
				dispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0);

			break;
		}


		WriteNBLIntoRingBuffer(&kernel2userRingBuffer, netBufferLists, UpperToFilter, filterContext->miniportIfIndex);


		//NdisFSendNetBufferListsComplete(filterContext->filterHandle, netBufferLists ,sendFlags);
		NdisFSendNetBufferLists(filterContext->filterHandle, netBufferLists, portNumber, sendFlags);

	} while (FALSE);

}


VOID WPTSendToNICFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG sendCompleteFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;

	NET_BUFFER_LIST* pNetBufList;
	NET_BUFFER_LIST* pNextNetBufList;

	BOOLEAN dispatchLevel;

	dispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(sendCompleteFlags);


	pNetBufList = netBufferLists;

	while (pNetBufList != NULL)
	{
		pNextNetBufList = NET_BUFFER_LIST_NEXT_NBL(pNetBufList);
		NET_BUFFER_LIST_NEXT_NBL(pNetBufList) = NULL;

		if (pNetBufList->SourceHandle == filterContext->filterHandle) {
			//If the NBL has our tag, FREE it.
			WPTFreeNBL(pNetBufList);
		}
		else {
			//Not our NBLs. Send complete the NBLs. 
			NdisFSendNetBufferListsComplete(filterContext->filterHandle, pNetBufList, sendCompleteFlags);
		}

		pNetBufList = pNextNetBufList;
	}

}
