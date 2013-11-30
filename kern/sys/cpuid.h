#ifndef CPUINFO_H
#define CPUINFO_H

// %ecx
#define CPUID_R_C_SSE3			(1 << 0)
#define CPUID_R_C_PCLMUL		(1 << 1)
#define CPUID_R_C_SSSE3			(1 << 9)
#define CPUID_R_C_FMA			(1 << 12)
#define CPUID_R_C_CMPXCHG16B	(1 << 13)
#define CPUID_R_C_SSE4_1		(1 << 19)
#define CPUID_R_C_SSE4_2		(1 << 20)
#define CPUID_R_C_MOVBE			(1 << 22)
#define CPUID_R_C_POPCNT		(1 << 23)
#define CPUID_R_C_AES			(1 << 25)
#define CPUID_R_C_XSAVE			(1 << 26)
#define CPUID_R_C_OSXSAVE		(1 << 27)
#define CPUID_R_C_AVX			(1 << 28)
#define CPUID_R_C_F16C			(1 << 29)
#define CPUID_R_C_RDRND			(1 << 30)

// %edx
#define CPUID_R_D_ENHANCEDV86	(1 << 2)
#define CPUID_R_D_CMPXCHG8B		(1 << 8)
#define CPUID_R_D_CMOV			(1 << 15)
#define CPUID_R_D_MMX			(1 << 23)
#define CPUID_R_D_FXSAVE		(1 << 24)
#define CPUID_R_D_SSE			(1 << 25)
#define CPUID_R_D_SSE2			(1 << 26)

// Extended Features
// %ecx
#define CPUID_E_C_LAHF_LM		(1 << 0)
#define CPUID_E_C_ABM			(1 << 5)
#define CPUID_E_C_SSE4a			(1 << 6)
#define CPUID_E_C_XOP			(1 << 11)
#define CPUID_E_C_LWP			(1 << 15)
#define CPUID_E_C_FMA4			(1 << 16)
#define CPUID_E_C_TBM			(1 << 21)

// %edx
#define CPUID_E_D_MMXEXT		(1 << 22)
#define CPUID_E_D_LM			(1 << 29)
#define CPUID_E_D_3DNOWP		(1 << 30)
#define CPUID_E_D_3DNOW			(1 << 31)

#define cpuid(in, a, b, c, d) __asm__ volatile("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in));

typedef enum sys_cpu_manufacturer {
	kCPUManufacturerIntel,
	kCPUManufacturerAMD
} cpu_manufacturer_t;

typedef struct sys_cpu_info {
	uint32_t extended;
	
	char *detected_name;

	uint32_t family;
	uint32_t extended_family;
	uint32_t model;
	uint32_t stepping;
	uint32_t reserved;

	uint32_t type;
	uint32_t brand;

	uint32_t signature;

	uint32_t cpuid_eax, cpuid_ebx, cpuid_ecx, cpuid_edx;

	cpu_manufacturer_t manufacturer;

	bool temp_diode;

	// These strings can be populated by sys_set_cpuid_strings
	char *str_type;
	char *str_family;
	char *str_model;
	char *str_brand;
} cpu_info_t;

// Define equates for bit positions for various features in CPUID.
enum {
	CPUID_FEAT_ECX_SSE3			= 1 << 0, 
	CPUID_FEAT_ECX_PCLMUL		= 1 << 1,
	CPUID_FEAT_ECX_DTES64		= 1 << 2,
	CPUID_FEAT_ECX_MONITOR		= 1 << 3,  
	CPUID_FEAT_ECX_DS_CPL		= 1 << 4,  
	CPUID_FEAT_ECX_VMX			= 1 << 5,  
	CPUID_FEAT_ECX_SMX			= 1 << 6,  
	CPUID_FEAT_ECX_EST			= 1 << 7,  
	CPUID_FEAT_ECX_TM2			= 1 << 8,  
	CPUID_FEAT_ECX_SSSE3		= 1 << 9,  
	CPUID_FEAT_ECX_CID			= 1 << 10,
	CPUID_FEAT_ECX_FMA			= 1 << 12,
	CPUID_FEAT_ECX_CX16			= 1 << 13, 
	CPUID_FEAT_ECX_ETPRD		= 1 << 14, 
	CPUID_FEAT_ECX_PDCM			= 1 << 15, 
	CPUID_FEAT_ECX_DCA			= 1 << 18, 
	CPUID_FEAT_ECX_SSE4_1		= 1 << 19, 
	CPUID_FEAT_ECX_SSE4_2		= 1 << 20, 
	CPUID_FEAT_ECX_x2APIC		= 1 << 21, 
	CPUID_FEAT_ECX_MOVBE		= 1 << 22, 
	CPUID_FEAT_ECX_POPCNT		= 1 << 23, 
	CPUID_FEAT_ECX_AES			= 1 << 25, 
	CPUID_FEAT_ECX_XSAVE		= 1 << 26, 
	CPUID_FEAT_ECX_OSXSAVE		= 1 << 27, 
	CPUID_FEAT_ECX_AVX			= 1 << 28,
 
	CPUID_FEAT_EDX_FPU			= 1 << 0,  
	CPUID_FEAT_EDX_VME			= 1 << 1,  
	CPUID_FEAT_EDX_DE			= 1 << 2,  
	CPUID_FEAT_EDX_PSE			= 1 << 3,  
	CPUID_FEAT_EDX_TSC			= 1 << 4,  
	CPUID_FEAT_EDX_MSR			= 1 << 5,  
	CPUID_FEAT_EDX_PAE			= 1 << 6,  
	CPUID_FEAT_EDX_MCE			= 1 << 7,  
	CPUID_FEAT_EDX_CX8			= 1 << 8,  
	CPUID_FEAT_EDX_APIC			= 1 << 9,  
	CPUID_FEAT_EDX_SEP			= 1 << 11, 
	CPUID_FEAT_EDX_MTRR			= 1 << 12, 
	CPUID_FEAT_EDX_PGE			= 1 << 13, 
	CPUID_FEAT_EDX_MCA			= 1 << 14, 
	CPUID_FEAT_EDX_CMOV			= 1 << 15, 
	CPUID_FEAT_EDX_PAT			= 1 << 16, 
	CPUID_FEAT_EDX_PSE36		= 1 << 17, 
	CPUID_FEAT_EDX_PSN			= 1 << 18, 
	CPUID_FEAT_EDX_CLF			= 1 << 19, 
	CPUID_FEAT_EDX_DTES			= 1 << 21, 
	CPUID_FEAT_EDX_ACPI			= 1 << 22, 
	CPUID_FEAT_EDX_MMX			= 1 << 23, 
	CPUID_FEAT_EDX_FXSR			= 1 << 24, 
	CPUID_FEAT_EDX_SSE			= 1 << 25, 
	CPUID_FEAT_EDX_SSE2			= 1 << 26, 
	CPUID_FEAT_EDX_SS			= 1 << 27, 
	CPUID_FEAT_EDX_HTT			= 1 << 28, 
	CPUID_FEAT_EDX_TM1			= 1 << 29, 
	CPUID_FEAT_EDX_IA64			= 1 << 30,
	CPUID_FEAT_EDX_PBE			= 1 << 31
};

cpu_info_t* cpuid_detect();
cpu_info_t* cpuid_detect_intel();
cpu_info_t* cpuid_detect_amd();
void cpuid_set_strings(cpu_info_t* in);

#endif