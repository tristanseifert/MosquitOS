#ifndef CPUINFO_H
#define CPUINFO_H

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

	cpu_manufacturer_t manufacturer;

	bool temp_diode;

	// These strings can be populated by sys_set_cpuid_strings
	char *str_type;
	char *str_family;
	char *str_model;
	char *str_brand;
} cpu_info_t;

cpu_info_t* cpuid_detect();
cpu_info_t* cpuid_detect_intel();
cpu_info_t* cpuid_detect_amd();
void cpuid_set_strings(cpu_info_t* in);

#endif