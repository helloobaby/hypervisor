#pragma once
#include "ia32-doc/out/ia32.hpp"
#include "type_define.h"

#include <intrin.h>

//��ǰcpu�߼�������(Ĭ��û�д�������,Ҳ���Ƕ�����CPU)
extern u32 g_logical_processor;

struct hypervisor {
	
	

};


struct hypervisor_shared {

};

bool check_hypervisor_can_enable();

bool prepare_vmcs(vmcs& vmcs);