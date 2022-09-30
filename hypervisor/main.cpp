//https://nixhacker.com/developing-hypervisior-from-scratch-part-1/
//https://developer.apple.com/documentation/hypervisor/

#include<ntifs.h>
#include<ntddk.h>
#include<wdm.h>

#include "utils.h"
#include "vcpu.h"

void DriverUnload(PDRIVER_OBJECT DriverObject);


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegPath) {
	
	DriverObject->DriverUnload = DriverUnload;

	//缓存一些数据
	cached_cpu_data();


	g_logical_processor = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
	

	print("[+] g_logical_processor %d\n", g_logical_processor);

	bool b;
	b = check_hypervisor_can_enable();
	if (!b) {
		print("[+] check_hypervisor_can_enable return false\n");
		return STATUS_SUCCESS;
	}

	//通过ipi call虚拟化每个核
	KeIpiGenericCall(virtualize_everycpu_ipi_routine,NULL);

	return STATUS_SUCCESS;
}


void DriverUnload(PDRIVER_OBJECT DriverObject) {

	stop_hypervisor();


	return;
}