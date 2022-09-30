#include "utils.h"

void* get_segment_base_by_descriptor(segment_descriptor_64* sd) {
	u64 base = 0;
	const auto base_high = sd->base_address_high << (6 * 4);
	const auto base_middle = sd->base_address_middle << (4 * 4);
	const auto base_low = sd->base_address_low;
	base = (base_high | base_middle | base_low) & 0xffffffff/*MAXULONG*/;
	u64 base_upper32 = sd->base_address_upper;
	base |= (base_upper32 << 32);
	return (void*)base;
}