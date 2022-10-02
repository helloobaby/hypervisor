//https://nixhacker.com/developing-hypervisior-from-scratch-part-1/
//https://developer.apple.com/documentation/hypervisor/

#include<ntifs.h>
#include<ntddk.h>
#include<wdm.h>

#include "wmi_trace.h"
#include "main.tmh"

#include "vcpu.h"
#include "hypercalls.h"

#include "utils.h"





void DriverUnload(PDRIVER_OBJECT DriverObject);


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegPath) {
	
	DriverObject->DriverUnload = DriverUnload;

	// 缓存一些数据
	cached_cpu_data();

	// 初始化wmi trace
	WPP_INIT_TRACING(DriverObject, RegPath);

	g_logical_processor = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
	
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[+] g_logical_processor %d\n", g_logical_processor);

	bool b;
	b = check_hypervisor_can_enable();
	if (!b) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,"[+] check_hypervisor_can_enable return false\n");
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