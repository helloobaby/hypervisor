#include<ntifs.h>
#include<ntddk.h>
#include<wdm.h>

#include "utils.h"
#include "vcpu.h"

void DriverUnload(PDRIVER_OBJECT DriverObject);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegPath) {
	
	DriverObject->DriverUnload = DriverUnload;

	g_logical_processor = KeGetCurrentProcessorNumber();
	

	print("[+] g_logical_processor %d\n", g_logical_processor);

	bool b;
	b = check_hypervisor_can_enable();
	if (!b) {
		print("[+] check_hypervisor_can_enable return false\n");
		return STATUS_SUCCESS;
	}

	return STATUS_SUCCESS;
}


void DriverUnload(PDRIVER_OBJECT DriverObject) {
	return;
}