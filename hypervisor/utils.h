#pragma once
#include "ia32-doc/out/ia32.hpp"
#include "type_define.h"

#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>

extern "C" {
	UCHAR* PsGetProcessImageFileName(PEPROCESS Process);
	u32 KeGetEffectiveIrql();
}

#if 0
template<typename... types>
void print(types... args)
{
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, args...);
}
#endif

void* get_segment_base_by_descriptor(segment_descriptor_64* sd);

bool is_invalid_msr_access(u32 msr_index);

char* get_current_process_name();