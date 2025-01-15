#include "devctrl.h"
#include "Normal.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symbolicLink;

	// Initialize symbolic link string for cleanup
	RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\Leviathan");

	// Remove symbolic link first
	NTSTATUS status = IoDeleteSymbolicLink(&symbolicLink);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to delete symbolic link (0x%08X)\n", status);
	}

	// Delete the device object if it exists
	if (DriverObject->DeviceObject != NULL) {
		IoDeleteDevice(DriverObject->DeviceObject);
		DriverObject->DeviceObject = NULL;
	}

	DbgPrint("Driver unloaded successfully\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("Driver initialization started\n");

	__try {
		// Initialize dispatch routines for all supported IRPs
		for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
			DriverObject->MajorFunction[i] = devctrl_dispatch;
		}

		// Initialize device control interface
		status = devctrl_ioInit(DriverObject);
		if (!NT_SUCCESS(status)) {
			DbgPrint("Failed to initialize device control (0x%08X)\n", status);
			__leave;
		}

		// Set driver unload routine
		DriverObject->DriverUnload = DriverUnload;

		DbgPrint("Driver initialized successfully\n");
	}
	__finally {
		if (!NT_SUCCESS(status)) {
			// Cleanup if initialization fails
			if (DriverObject->DriverUnload) {
				DriverObject->DriverUnload(DriverObject);
			}
		}
	}

	return status;
}