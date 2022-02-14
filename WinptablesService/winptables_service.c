/*
 * File Name:		winptables_service.c
 * Description:		winptables service entry
 * Date:			2022.2.11
 * Author:			HBSnail
 */

#include "global.h"
#include "ring_buffer_r3.h"


HANDLE commDevice;
HANDLE refreshRouteTableHandle;

RING_BUFFER_R3 kernel2userRingBuffer_INBOUND;
RING_BUFFER_R3 kernel2userRingBuffer_OUTBOUND;
RING_BUFFER_R3 user2kernelRingBuffer_INBOUND;
RING_BUFFER_R3 user2kernelRingBuffer_OUTBOUND;

typedef struct _ROUTE_INFO {
	BOOLEAN canRoute;
	BOOLEAN isLocal;
	ULONG outIfIndex;
}ROUTE_INFO;


VOID RefreshRouteTable(VOID* must_null_ptr, PMIB_IPFORWARD_ROW2 Row, MIB_NOTIFICATION_TYPE NotificationType) {
	
}

ROUTE_INFO GetRoute(BYTE* ethFrame) {
	ROUTE_INFO retInfo;
	//EthTypeCode is in big endian,convert to little endian!!!
	USHORT etherFrameType = *((BYTE*)ethFrame + 12);
	etherFrameType = (etherFrameType << 8) | (*((BYTE*)ethFrame + 13));
	//Now we get the ethtype in little endian (x86-64)

	switch (etherFrameType)
	{
	case 0x0800: //IPv4

		break;
	case 0x86DD: //IPv6

		break;
	default:

		break;
	}

	/*SOCKADDR_INET destIP;
	MIB_IPFORWARD_ROW2 forwardInfo;
	SOCKADDR_INET bestSrcIP;
	memset(&destIP, 0, sizeof(destIP));
	destIP.si_family = 2;
	GetBestRoute2(NULL, *(ULONG*)dataBuffer, NULL, &destIP, 0, &forwardInfo, &bestSrcIP);*/

}
VOID TestingRoutine1(VOID* must_null_ptr) {
	VOID* dataBuffer = malloc(RING_BUFFER_BLOCK_SIZE);
	while (1) {

		if (dataBuffer == NULL) {
			return;
		}

		do {

			BOOLEAN success = ReadBlockFromRingBufferR3(&kernel2userRingBuffer_INBOUND,dataBuffer);


			if (!success) {
				break;
			}


			//Ring buffer block structure:
			//ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			WriteBlockToRingBufferR3(&user2kernelRingBuffer_INBOUND,dataBuffer);

		} while (FALSE);

	}


	if (dataBuffer != NULL) {
		free(dataBuffer);
	}


}

VOID TestingRoutine2(VOID* must_null_ptr) {
	VOID* dataBuffer = malloc(RING_BUFFER_BLOCK_SIZE);

	while (1) {

		if (dataBuffer == NULL) {
			return;
		}
		do {

			BOOLEAN success = ReadBlockFromRingBufferR3(&kernel2userRingBuffer_OUTBOUND,dataBuffer);


			//EthTypeCode is in big endian,convert to little endian!!!
			USHORT etherFrameType = *((BYTE*)dataBuffer + 20);
			etherFrameType = (etherFrameType << 8) | (*((BYTE*)dataBuffer + 21));
			//Now we get the ethtype in little endian (x86-64)

			SOCKADDR_INET destIP;
			MIB_IPFORWARD_ROW2 forwardInfo;
			SOCKADDR_INET bestSrcIP;
			memset(&destIP, 0, sizeof(destIP));
			destIP.si_family = 2;
			GetBestRoute2(NULL, *(ULONG*)dataBuffer, NULL, &destIP, 0, &forwardInfo, &bestSrcIP);


			if (!success) {
				break;
			}


			//Ring buffer block structure:
			//direction 4 byte; ifIndex 4 byte;  ethLeng 4Byte; ethdata... ;pending 0000....

			WriteBlockToRingBufferR3(&user2kernelRingBuffer_OUTBOUND,dataBuffer);

		} while (FALSE);

	}

	if (dataBuffer != NULL) {
		free(dataBuffer);
	}

}



int main() {
	NotifyRouteChange2(AF_UNSPEC, (PIPFORWARD_CHANGE_CALLBACK)RefreshRouteTable, NULL, TRUE, &refreshRouteTableHandle);

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

		 kernel2userRingBuffer_INBOUND.bufferAddress = *outBuffer;
		 kernel2userRingBuffer_OUTBOUND.bufferAddress = *(outBuffer + 1);
		 user2kernelRingBuffer_INBOUND.bufferAddress = *(outBuffer + 2);
		 user2kernelRingBuffer_OUTBOUND.bufferAddress = *(outBuffer + 3);

		 kernel2userRingBuffer_INBOUND.sharedStructure = *(outBuffer + 4);
		 kernel2userRingBuffer_OUTBOUND.sharedStructure = *(outBuffer + 5);
		 user2kernelRingBuffer_INBOUND.sharedStructure = *(outBuffer + 6);
		 user2kernelRingBuffer_OUTBOUND.sharedStructure = *(outBuffer + 7);


		printf("[OK] Get kernel mapped address success. \n	\
			kernel2userRingBuffer_INBOUND:0x%p\n	\
			kernel2userRingBuffer_OUTBOUND:0x%p\n	\
			user2kernelRingBuffer_INBOUND:0x%p\n	\
			user2kernelRingBuffer_OUTBOUND:0x%p\n	\
			kernel2userRingBuffer_INBOUND.sharedStructure:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n	\
			kernel2userRingBuffer_OUTBOUND.sharedStructure:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n	\
			user2kernelRingBuffer_INBOUND.sharedStructure:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n	\
			user2kernelRingBuffer_OUTBOUND.sharedStructure:0x%p\n	\
				Info: Head:%d Tail:%d Size:%d ModFactor:%d\n",
			kernel2userRingBuffer_INBOUND.bufferAddress, 
			kernel2userRingBuffer_OUTBOUND.bufferAddress, 
			user2kernelRingBuffer_INBOUND.bufferAddress, 
			user2kernelRingBuffer_OUTBOUND.bufferAddress,
			kernel2userRingBuffer_INBOUND.sharedStructure,
			kernel2userRingBuffer_INBOUND.sharedStructure->head, kernel2userRingBuffer_INBOUND.sharedStructure->tail, kernel2userRingBuffer_INBOUND.sharedStructure->bufferSize, kernel2userRingBuffer_INBOUND.sharedStructure->modFactor,
			kernel2userRingBuffer_OUTBOUND.sharedStructure,
			kernel2userRingBuffer_OUTBOUND.sharedStructure->head, kernel2userRingBuffer_OUTBOUND.sharedStructure->tail, kernel2userRingBuffer_OUTBOUND.sharedStructure->bufferSize, kernel2userRingBuffer_OUTBOUND.sharedStructure->modFactor,
			user2kernelRingBuffer_INBOUND.sharedStructure,
			user2kernelRingBuffer_INBOUND.sharedStructure->head, user2kernelRingBuffer_INBOUND.sharedStructure->tail, user2kernelRingBuffer_INBOUND.sharedStructure->bufferSize, user2kernelRingBuffer_INBOUND.sharedStructure->modFactor,
			user2kernelRingBuffer_OUTBOUND.sharedStructure,
			user2kernelRingBuffer_OUTBOUND.sharedStructure->head, user2kernelRingBuffer_OUTBOUND.sharedStructure->tail, user2kernelRingBuffer_OUTBOUND.sharedStructure->bufferSize, user2kernelRingBuffer_OUTBOUND.sharedStructure->modFactor
		);

		kernel2userRingBuffer_INBOUND.dataBlockWriteEventHandle = OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_INBOUND);
		if (kernel2userRingBuffer_INBOUND.dataBlockWriteEventHandle == INVALID_HANDLE_VALUE || kernel2userRingBuffer_INBOUND.dataBlockWriteEventHandle == NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (k2u_in) open failed. HANDLE_VALUE: 0x%p\n", kernel2userRingBuffer_INBOUND.dataBlockWriteEventHandle);

			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (k2u_in) open success. HANDLE_VALUE: 0x%p\n", kernel2userRingBuffer_INBOUND.dataBlockWriteEventHandle);

		kernel2userRingBuffer_OUTBOUND.dataBlockWriteEventHandle = OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_OUTBOUND);
		if (kernel2userRingBuffer_OUTBOUND.dataBlockWriteEventHandle == INVALID_HANDLE_VALUE || kernel2userRingBuffer_OUTBOUND.dataBlockWriteEventHandle == NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (k2u_out) open failed. HANDLE_VALUE: 0x%p\n", kernel2userRingBuffer_OUTBOUND.dataBlockWriteEventHandle);
			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (k2u_out) open success. HANDLE_VALUE: 0x%p\n", kernel2userRingBuffer_OUTBOUND.dataBlockWriteEventHandle);

		user2kernelRingBuffer_INBOUND.dataBlockWriteEventHandle = OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_INBOUND);
		if (user2kernelRingBuffer_INBOUND.dataBlockWriteEventHandle == INVALID_HANDLE_VALUE || user2kernelRingBuffer_INBOUND.dataBlockWriteEventHandle == NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (u2k_in) open failed. HANDLE_VALUE: 0x%p\n", user2kernelRingBuffer_INBOUND.dataBlockWriteEventHandle);
			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (u2k_in) open success. HANDLE_VALUE: 0x%p\n", user2kernelRingBuffer_INBOUND.dataBlockWriteEventHandle);

		user2kernelRingBuffer_OUTBOUND.dataBlockWriteEventHandle =  OpenEvent(EVENT_ALL_ACCESS, FALSE, WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_OUTBOUND);
		if (user2kernelRingBuffer_OUTBOUND.dataBlockWriteEventHandle == INVALID_HANDLE_VALUE || user2kernelRingBuffer_OUTBOUND.dataBlockWriteEventHandle ==NULL) {
			printf("[ERROR] winptables kernel-user syncheonize event (u2k_out) open failed. HANDLE_VALUE: 0x%p\n", user2kernelRingBuffer_OUTBOUND.dataBlockWriteEventHandle);
			break;
		}
		printf("[OK] winptables kernel-user syncheonize event (u2k_out) open success. HANDLE_VALUE: 0x%p\n", user2kernelRingBuffer_OUTBOUND.dataBlockWriteEventHandle);

		
		
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TestingRoutine1, NULL, 0, NULL);

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TestingRoutine2, NULL, 0, NULL);
		
		/*memset(kernel2userRingBuffer_INBOUND.sharedStructure.bufferAddress, 0, 32);
		memset(kernel2userRingBuffer_OUTBOUND.sharedStructure.bufferAddress, 0, 32);
		memset(kernel2userRingBuffer_INBOUND.bufferAddress, 0, 32);
		memset(user2kernelRingBuffer_INBOUND.bufferAddress, 0, 32);*/
		
		

	} while (FALSE);
	int t;
	scanf_s("%d", &t);
	return 0;
}