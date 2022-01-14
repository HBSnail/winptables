/*
 * File Name:		winptables_comm_device.h
 * Date:			2022.1.14
 * Author:			HBSnail
 */


#pragma once
#ifndef _WINPTABLES_COMM_DEVICE_H_		

	#define _WINPTABLES_COMM_DEVICE_H_

	#include "global.h"

	NTSTATUS WPTCommDeviceCreate(DEVICE_OBJECT* deviceObject,IRP* irp);

	NTSTATUS WPTCommDeviceClose(DEVICE_OBJECT* deviceObject, IRP* irp);

	NTSTATUS WPTCommDeviceClean(DEVICE_OBJECT* deviceObject, IRP* irp);

	NTSTATUS WPTCommDeviceIOCtl(DEVICE_OBJECT* deviceObject, IRP* irp);

	NTSTATUS WPTCommDeviceRead(DEVICE_OBJECT* deviceObject, IRP* irp);

	NTSTATUS WPTCommDeviceWrite(DEVICE_OBJECT* deviceObject, IRP* irp);



#endif

