#include "mm.h"

uint64_t get_physical(void* const virt_addr) {
	return MmGetPhysicalAddress(virt_addr).QuadPart;
}