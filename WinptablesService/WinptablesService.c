#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>


#define WINPTABLES_DEVICE_NAME L"\\\\.\\winptables_comm"

HANDLE commDevice;

typedef struct _RING_BUFFER_SHARED_VARIABLES
{
	ULONG head;
	ULONG tail;
	ULONG bufferSize;
	ULONG modFactor;
}RING_BUFFER_SHARED_VARIABLES;

int main() {

	do {
		commDevice = CreateFile(WINPTABLES_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (commDevice == INVALID_HANDLE_VALUE) {
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

		BOOL getMDLOk = DeviceIoControl(commDevice, CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_GENERIC_WRITE), NULL, 0, outBuffer, 8 * sizeof(VOID*), &outLeng, NULL);

		if (!getMDLOk) {
			printf("[ERROR] Get kernel mapped address failed.");
			break;
		}

		VOID* kernel2userRingBuffer_INBOUND_UserAddr = *outBuffer;
		VOID* kernel2userRingBuffer_OUTBOUND_UserAddr = *(outBuffer + 1);
		VOID* user2kernelRingBuffer_INBOUND_UserAddr = *(outBuffer + 2);
		VOID* user2kernelRingBuffer_OUTBOUND_UserAddr = *(outBuffer + 3);

		RING_BUFFER_SHARED_VARIABLES* kernel2userRingBuffer_INBOUND_STRUCTURE = *(outBuffer + 4);
		RING_BUFFER_SHARED_VARIABLES* kernel2userRingBuffer_OUTBOUND_STRUCTURE = *(outBuffer + 5);
		RING_BUFFER_SHARED_VARIABLES* user2kernelRingBuffer_INBOUND_STRUCTURE = *(outBuffer + 6);
		RING_BUFFER_SHARED_VARIABLES* user2kernelRingBuffer_OUTBOUND_STRUCTURE = *(outBuffer + 7);


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


		/*memset(kernel2userRingBuffer_INBOUND_STRUCTURE_UserAddr, 0, 32);
		memset(kernel2userRingBuffer_OUTBOUND_STRUCTURE_UserAddr, 0, 32);
		memset(kernel2userRingBuffer_INBOUND_UserAddr, 0, 32);
		memset(user2kernelRingBuffer_INBOUND_UserAddr, 0, 32);*/

	} while (FALSE);


	return 0;
}