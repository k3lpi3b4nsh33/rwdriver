#pragma once

#include <ntddk.h>
#include <stdlib.h>

#ifndef _DEVCTRL_H_
#define _DEVCTRL_H_

#define CTL_DEVCTRL_RWMEM \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0xAE86, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MEMORY_OPERATION {
    void* SourceAddress;
    void* DestinationAddress;
    size_t Size;
} MEMORY_OPERATION, * PMEMORY_OPERATION;

static UNICODE_STRING g_devicename;
static UNICODE_STRING g_devicesyslink;
static PDEVICE_OBJECT g_deviceControl;

NTSTATUS devctrl_dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);
NTSTATUS devctrl_RwMemory(PDEVICE_OBJECT DeviceObject, PIRP irp, PIO_STACK_LOCATION irpSp);
NTSTATUS devctrl_ioInit(PDRIVER_OBJECT DriverObject);

#endif // !_DEVCTRL_H_


