/*
 * File Name:		global.h
 * Date:			2022.2.11
 * Author:			HBSnail
 */

#pragma once
#ifndef _GLOBAL_H
	#define _GLOBAL_H_


	#include <winsock2.h>
	#include <Windows.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <netioapi.h>
	#include <iphlpapi.h>


	#define WINPTABLES_DEVICE_NAME L"\\\\.\\winptables_comm"

	#define WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_INBOUND L"Global\\winptables_ke_k2u_in"
	#define WINPTABLES_KERNEL_EVENT_KERNEL_TO_USER_OUTBOUND L"Global\\winptables_ke_k2u_out"
	#define WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_INBOUND L"Global\\winptables_ke_u2k_in"
	#define WINPTABLES_KERNEL_EVENT_USER_TO_KERNEL_OUTBOUND L"Global\\winptables_ke_u2k_out"

#endif