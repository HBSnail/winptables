/*
 * File Name:		filter_subroutines.h
 * Date:			2022.1.14
 * Author:			HBSnail
 */

#pragma once
#ifndef _FILTER_SUBROUTINES_H_		

	#define _FILTER_SUBROUTINES_H_

	#include "ring_buffer.h"

	NTSTATUS TransmitEthPacket(FILTER_CONTEXT* filterContext, ULONG length, MDL* dataMDL, TRANSFER_DIRECION direction, ULONG flag);

	VOID WPTFreeNBL(NET_BUFFER_LIST* NetBufferLists);

	FILTER_CONTEXT* GetFilterContextByMiniportInterfaceIndex(ULONG index);

	NDIS_STATUS WPTFilterSetOptions(NDIS_HANDLE  ndisFilterDriverHandle, NDIS_HANDLE  filterDriverContext);

	NDIS_STATUS WPTFilterSetModuleOptions(NDIS_HANDLE filterModuleContext);

	NDIS_STATUS WPTFilterAttach(NDIS_HANDLE ndisfilterHandle, NDIS_HANDLE filterDriverContext, NDIS_FILTER_ATTACH_PARAMETERS* attachParameters);
	
	NDIS_STATUS WPTFilterPause(NDIS_HANDLE filterModuleContext, NDIS_FILTER_PAUSE_PARAMETERS* pauseParameters);
	
	NDIS_STATUS WPTFilterRestart(NDIS_HANDLE filterModuleContext, NDIS_FILTER_RESTART_PARAMETERS* RestartParameters);
	
	VOID WPTFilterDetach(NDIS_HANDLE filterModuleContext);
	
	VOID WPTReceivedFromNIC(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG numberOfNetBufferLists, ULONG receiveFlags);
	
	VOID WPTSendToUpperFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG returnFlags);
	
	VOID WPTReceivedFromUpper(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG sendFlags);
	
	VOID WPTSendToNICFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG sendCompleteFlags);

#endif

