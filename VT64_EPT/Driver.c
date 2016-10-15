#pragma once
#include "intel_vt_x64.h"

#define MY_DVC_BUFFERED_CODE (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,	FILE_ANY_ACCESS)
#define VT_64_OPEN 0x80010

NTSTATUS MyDeviceIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS Status;
	HANDLE hThread = NULL;
	PIO_STACK_LOCATION irpSp; //��ǰIRP����ջ�ռ�   
	ULONG code;               //���ܺ�   
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG inBufLength;        //���뻺��������   
	ULONG outBufLength;       //�������������   
	PCHAR inBuf;              //���뻺����   
	PCHAR outBuf;             //���������   
	PCHAR outData = "[VTF] 111"; //Ҫ��Ӧ�ò��������Ϣ   
	ULONG outDataLen = strlen(outData) + 1;  //��Ϣ���Ⱥ���βһ��NULL   

	DbgPrint("[VTF] GetIoControl\n");

	irpSp = IoGetCurrentIrpStackLocation(Irp);               //��õ�ǰIRP����ջ�ռ�   
	code = irpSp->Parameters.DeviceIoControl.IoControlCode;  //�õ����ܺţ���������   
	inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;   //�õ����뻺��������   
	outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength; //�õ��������������   
	inBuf = Irp->AssociatedIrp.SystemBuffer;  //���뻺����   
	outBuf = Irp->AssociatedIrp.SystemBuffer; //���������   

	if (code == VT_64_OPEN)  //�����Զ���Ŀ�����   
	{
		DbgPrint("[VTF] inBuf: %s\n", inBuf);      //��ӡ��Ӧ�ò㴫�������   

		RtlCopyBytes(outBuf, outData, outBufLength); //��������Ҫ��������ݵ����������   

		Irp->IoStatus.Information = (outBufLength < outDataLen ? outBufLength : outDataLen);
		Status = PsCreateSystemThread(&hThread,
			THREAD_ALL_ACCESS,
			NULL,
			NULL,
			NULL,
			(PKSTART_ROUTINE)VtStart, NULL);
		if (!NT_SUCCESS(Status))
		{
			Irp->IoStatus.Status = STATUS_SUCCESS;
		}
		Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	}
	else
	{
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	}
	IoCompleteRequest(Irp, IO_NO_INCREMENT); //����IRP����   

	DbgPrint("[VTF] MyDeviceIoControl Over\n");
	return Irp->IoStatus.Status;
}
NTSTATUS MyCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	DbgPrint("[VTF] MyCreateClose\n");
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}

NTSTATUS DriverUnload(PDRIVER_OBJECT DriverObject)
{
	CCHAR i;
	KIRQL OldIrql;
	KAFFINITY OldAffinity;

	KeInitializeMutex(&g_mutex, 0);
	KeWaitForSingleObject(&g_mutex, Executive, KernelMode, FALSE, NULL);

	for (i = 0; i < KeNumberProcessors; i++)
	{
		OldAffinity = KeSetSystemAffinityThreadEx((KAFFINITY)(1 << i));
		OldIrql = KeRaiseIrqlToDpcLevel();
		_StopVirtualization();
		KeLowerIrql(OldIrql);
		KeRevertToUserAffinityThreadEx(OldAffinity);
	}

	KeReleaseMutex(&g_mutex, FALSE);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status = STATUS_SUCCESS;
	HANDLE hThread = NULL;
	FGP_VT_KDPRINT(("Dirver is Start"));
	DriverObject->DriverUnload = DriverUnload;

	Status = PsCreateSystemThread(&hThread,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		(PKSTART_ROUTINE)VtStart, NULL);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	FGP_VT_KDPRINT(("Dirver is Start"));
	//Status = VtStart(NULL);
	return Status;
	//NTSTATUS ntStatus = STATUS_SUCCESS;
	//PDEVICE_OBJECT Device;
	//UNICODE_STRING DeviceName, DeviceLink;  //�豸��������������   

	//DbgPrint("[VTF] DriverEntry\n");

	//RtlInitUnicodeString(&DeviceName, L"\\Device\\VT_64");         //��ʼ���豸��  
	//RtlInitUnicodeString(&DeviceLink, L"\\DosDevices\\VT_64");  //��ʼ������������  
	////mm_init(DriverObject);
	////p2m_init();
	//EptPml4TablePointer = InitEptIdentityMap();
	//pagingInitMappingOperations(&memContext, 1024);
	///* IoCreateDevice �����豸���� */
	//ntStatus = IoCreateDevice(DriverObject,         //�����豸����������   
	//	0,                    //�豸��չ���ڴ��С   
	//	&DeviceName,          //�豸����/Device/Aliwy   
	//	FILE_DEVICE_UNKNOWN,  //�豸����   
	//	0,                    //��д0����   
	//	FALSE,                //����ΪFALSE   
	//	&Device);             //�豸����ָ�뷵�ص�DeviceObject��   
	//if (!NT_SUCCESS(ntStatus))
	//{
	//	DbgPrint("[VTF] IoCreateDevice FALSE: %.8X\n", ntStatus);
	//	return ntStatus;  //����ʧ�ܾͷ���   
	//}
	//else
	//	DbgPrint("[VTF] IoCreateDevice SUCCESS\n");

	///* IoCreateSymbolicLink ���ɷ������� */
	//ntStatus = IoCreateSymbolicLink(&DeviceLink, &DeviceName);
	//if (!NT_SUCCESS(ntStatus))
	//{
	//	DbgPrint("[VTF] IoCreateSymbolicLink FALSE: %.8X\n", ntStatus);
	//	IoDeleteDevice(Device);  //ɾ���豸   
	//	return ntStatus;
	//}
	//else
	//	DbgPrint("[VTF] IoCreateSymbolicLink SUCCESS\n");

	//Device->Flags &= ~DO_DEVICE_INITIALIZING;  //�豸��ʼ����ɱ��   

	//DriverObject->DriverUnload = DriverUnload;

	///*�豸�������󣬶�ӦRing3 DeviceIoControl*/
	//DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDeviceIoControl;
	///*�豸�����󣬶�ӦRing3 CreateFile*/                      //   
	//DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreateClose; //  Ҫ��Ӧ�ò�ͨ�ţ�   
	///*�豸�ر����󣬶�ӦRing3 CloseHandle*/                     //  �����д򿪡��ر�����   
	//DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyCreateClose;  //    
	//PsSetCreateProcessNotifyRoutine(&processCreationMonitor, FALSE);

	//return ntStatus;
}
