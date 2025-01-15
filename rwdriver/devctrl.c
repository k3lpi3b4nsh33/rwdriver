#include "devctrl.h"
#include "Normal.h"


NTSTATUS devctrl_ioInit(PDRIVER_OBJECT DriverObject) {
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN deviceCreated = FALSE;

    // Initialize device name strings
    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLink;
    RtlInitUnicodeString(&deviceName, L"\\Device\\Leviathan");
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\Leviathan");

    __try {
        // Create device object
        status = IoCreateDevice(
            DriverObject,             // Our driver object
            0,                        // No extra bytes needed
            &deviceName,              // Device name
            FILE_DEVICE_UNKNOWN,      // Device type
            FILE_DEVICE_SECURE_OPEN,  // Device characteristics
            FALSE,                    // Not exclusive
            &g_deviceControl);        // Returned device object

        if (!NT_SUCCESS(status)) {
            DbgPrint("Failed to create device object (0x%08X)\n", status);
            __leave;
        }
        deviceCreated = TRUE;

        // Create symbolic link for user-mode access
        status = IoCreateSymbolicLink(&symbolicLink, &deviceName);
        if (!NT_SUCCESS(status)) {
            DbgPrint("Failed to create symbolic link (0x%08X)\n", status);
            __leave;
        }

        // Configure device flags
        g_deviceControl->Flags |= DO_DIRECT_IO;             // Use direct I/O
        g_deviceControl->Flags &= ~DO_DEVICE_INITIALIZING;  // Mark initialization complete
    }
    __finally {
        if (!NT_SUCCESS(status)) {
            // Clean up on failure
            if (deviceCreated) {
                IoDeleteDevice(g_deviceControl);
                g_deviceControl = NULL;
            }
        }
    }

    return status;
}

NTSTATUS devctrl_RwMemory(PDEVICE_OBJECT DeviceObject, PIRP irp, PIO_STACK_LOCATION irpSp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	SIZE_T bytesTransferred = 0;

	// Get the system buffer and validate input parameters
	PVOID pBuffer = irp->AssociatedIrp.SystemBuffer;
	ULONG bufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

	if (!pBuffer || bufferLength < sizeof(MEMORY_OPERATION)) {
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	// Cast buffer to our memory operation structure
	PMEMORY_OPERATION memOp = (PMEMORY_OPERATION)pBuffer;
	
	// Validate memory operation parameters
	if (!memOp->SourceAddress || !memOp->DestinationAddress || !memOp->Size) {
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	// Get current process context
	PEPROCESS CurrentProcess = IoGetCurrentProcess();
	
	// Perform the memory copy operation
	status = MmCopyVirtualMemory(
		CurrentProcess,
		memOp->SourceAddress,
		CurrentProcess, 
		memOp->DestinationAddress,
		memOp->Size,
		KernelMode,  // Explicitly specify kernel mode
		&bytesTransferred
	);

Exit:
	// Set IRP completion status
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = NT_SUCCESS(status) ? bufferLength : 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	
	return status;
}

NTSTATUS devctrl_dispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    switch (irpSp->MajorFunction) {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            // Allow create/close operations
            break;

        case IRP_MJ_DEVICE_CONTROL:
            // Handle device control requests
            return devctrl_RwMemory(DeviceObject, Irp, irpSp);

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    // Complete the IRP
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}