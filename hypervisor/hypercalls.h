/*
* 
* ¸úvmcallÓÐ¹Ø
* 
* 
*/

#include"ia32-doc/out/ia32.hpp"

enum hypercall_code : uint32_t {
    unload,             //Ð¶ÔØhypervisor
};

// hypercall input
struct hypercall_input {    
    // rax
    struct {
        hypercall_code code : 8;
        uint64_t       key : 56;
    };

    // rcx, rdx, r8, r9, r10, r11
    uint64_t args[6];
};