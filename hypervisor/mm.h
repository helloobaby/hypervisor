#pragma once
#include<ntifs.h>
#include<ntddk.h>
#include<wdm.h>

#include "type_define.h"


uint64_t get_physical(void* const virt_addr);