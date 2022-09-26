#pragma once
#include "ia32-doc/out/ia32.hpp"
#include "type_define.h"

#include <intrin.h>

//当前cpu逻辑核数量(默认没有处理器组,也就是多物理CPU)
extern u32 g_logical_processor;

struct hypervisor {
	
	

};


struct hypervisor_shared {

};

bool check_hypervisor_can_enable();

bool prepare_vmcs(vmcs& vmcs);