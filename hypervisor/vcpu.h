#pragma once
#include "ia32-doc/out/ia32.hpp"
#include "type_define.h"

#include <intrin.h>

//��ǰcpu�߼�������(Ĭ��û�д�������,Ҳ���Ƕ�����CPU)
extern u32 g_logical_processor;

struct cached_data {
	cpuid_eax_01 cpuid_01;
	cr0 _cr0;
	cr4 _cr4;
};
extern cached_data _cached_data;

//ÿ������Ҫ�����ݽṹ
struct hypervisor {
	alignas(0x1000) vmxon vmxon;
	alignas(0x1000) vmcs vmcs;

};


struct hypervisor_shared {

};

bool check_hypervisor_can_enable();

bool load_vmcs(vmcs& vmcs);
void prepare_vmcs();


void enable_vmx_operation();

void cached_cpu_data();

//vmcs�ṹ
void init_vmcs_control_fields();
void init_vmcs_host_state();
void init_vmcs_guest_state();
//

void stop_hypervisor();

u64 virtualize_everycpu_ipi_routine(u64 Argument);
u64 stop_virtualize_everycpu_ipi_routine(u64 Argument);


void test_vmcs_field();
u64 read_ctrl_field(u64 a, u64 true_a);