/*
 * File Name:		global.h
 * Date:			2022.1.14
 * Author:			HBSnail
 */

#pragma once
#ifndef _GLOBAL_H_

    #define _GLOBAL_H_

    #include <ntifs.h>
    #include <ndis.h>
    #include <netioapi.h>

    #define NDIS_MAJOR_VERSION 6
    #define NDIS_MINOR_VERSION 20
    #define DRIVER_MAJOR_VERSION 1
    #define DRIVER_MINOR_VERSION 0

    #define or ||
    #define and &&

    #define NO_FLAG 0

    #define FILTER_SERVICE_NAME  L"winptables"
    #define FILTER_GUID  L"{1b184974-b5ce-ea6e-bb38-c227ed17c203}"
    #define FILTER_FRIENDLY_NAME  L"Winptables NDIS Driver"

    #define WINPTABLES_COMMUNICATION_DEVICE_NAME L"\\Device\\WinptablesCommunicationDevice"
    #define WINPTABLES_COMMUNICATION_DEVICE_LINK L"\\??\\winptables_comm"

    #define FILTER_ALLOC_TAG 'FTAT'
    #define RING_BUFFER_ALLOC_TAG 'RBAT'
    #define SEND_POOL_ALLOC_TAG 'SPAT'
    #define TEMP_POOL_ALLOC_TAG 'TPAT'
    #define ETH_FRAME_POOL_ALLOC_TAG 'EFPT'

    typedef UCHAR BYTE;

    typedef enum _FILTER_STATE {
        FilterStateUnspecified,
        FilterInitialized,
        FilterPausing,
        FilterPaused,
        FilterRunning,
        FilterRestarting,
        FilterDetaching
    } FILTER_STATE;

    typedef enum _TRANSFER_DIRECION {
        NICToFilter,
        FilterToUpper,
        UpperToFilter,
        FilterToNIC
    } TRANSFER_DIRECION;


    typedef struct _FILTER_CONTEXT {

        LIST_ENTRY filterModuleLink;
       
        //Reference to this filter
        NDIS_HANDLE filterHandle;
        NDIS_STRING filterModuleName;
        NDIS_STRING miniportFriendlyName;
        NDIS_STRING miniportName;
        NET_IFINDEX miniportIfIndex;

        BYTE currentMacAddress[NDIS_MAX_PHYS_ADDRESS_LENGTH];
        BYTE macLength;

        NDIS_SPIN_LOCK filterLock;    // filterLock for protection of state and outstanding sends and recvs

        FILTER_STATE filterState;   // Which state the filter is in

        NDIS_HANDLE sendNetBufferListPool;

        NDIS_STRING filterName;

    }FILTER_CONTEXT;



    #define NDIS_INIT_LOCK(_pLock)      NdisAllocateSpinLock(_pLock)

    #define NDIS_FREE_LOCK(_pLock)      NdisFreeSpinLock(_pLock)

    #define NDIS_ACQUIRE_LOCK(_pLock, DispatchLevel)              \
        {                                                           \
            if (DispatchLevel)                                      \
            {                                                       \
                NdisDprAcquireSpinLock(_pLock);                     \
            }                                                       \
            else                                                    \
            {                                                       \
                NdisAcquireSpinLock(_pLock);                        \
            }                                                       \
        }


    #define NDIS_RELEASE_LOCK(_pLock, DispatchLevel)              \
        {                                                           \
            if (DispatchLevel)                                      \
            {                                                       \
                NdisDprReleaseSpinLock(_pLock);                     \
            }                                                       \
            else                                                    \
            {                                                       \
                NdisReleaseSpinLock(_pLock);                        \
            }                                                       \
        }

#endif
