#include <types.h>
#include "cpuid.h"
#include "cpuid_data.h"

#define cpuid(in, a, b, c, d) __asm__ volatile("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in));

/*
 * Detects the kind of CPU in the system.
 */
cpu_info_t* cpuid_detect() {
	uint32_t ebx, unused;
	cpuid(0, unused, ebx, unused, unused);

	switch(ebx) {
		case 0x756e6547: // Intel Magic Code
			return cpuid_detect_intel();
			break;
		
		case 0x68747541: // AMD Magic Code
			return cpuid_detect_amd();
			break;
		
		default:
			return 0;
			break;
	}
	
	return 0;	
}

/*
 * Sets the string variables to point to char buffers.
 */
void cpuid_set_strings(cpu_info_t* in) {
	if(in->manufacturer == kCPUManufacturerIntel) {
		switch(in->type) {
			case 0:
				in->str_type = "Original OEM";
				break;
			case 1:
				in->str_type = "Overdrive";
				break;
			case 2:
				in->str_type = "Dual-capable";
				break;
			case 3:
				in->str_type = "Reserved";
				break;
		}

		switch(in->family) {
			case 3:
				in->str_family = "i386";
				break;
			case 4:
				in->str_family = "i486";
				break;
			case 5:
				in->str_family = "Pentium";
				break;
			case 6:
				in->str_family = "Pentium Pro";
				break;
			case 15:
				in->str_family = "Pentium 4";
				break;
		}

		switch(in->family) {
			case 3:
				break;
			case 4:
				switch(in->model) {
					case 0:
					case 1:
						in->str_model = "DX";
						break;
					case 2:
						in->str_model = "SX";
						break;
					case 3:
						in->str_model = "487/DX2";
						break;
					case 4:
						in->str_model = "SL";
						break;
					case 5:
						in->str_model = "SX2";
						break;
					case 7:
						in->str_model = "Write-back enhanced DX2";
						break;
					case 8:
						in->str_model = "DX4";
						break;
				}
				break;
			case 5:
				switch(in->model) {
					case 1:
						in->str_model = "60/66";
						break;
					case 2:
						in->str_model = "75-200";
						break;
					case 3:
						in->str_model = "for 486 system";
						break;
					case 4:
						in->str_model = "MMX";
						break;
				}
				break;
			case 6:
				switch(in->model) {
					case 1:
						in->str_model = "Pentium Pro";
						break;
					case 3:
						in->str_model = "Pentium II Model 3";
						break;
					case 5:
						in->str_model = "Pentium II Model 5/Xeon/Celeron";
						break;
					case 6:
						in->str_model = "Celeron";
						break;
					case 7:
						in->str_model = "Pentium III/Pentium III Xeon - external L2 cache";
						break;
					case 8:
						in->str_model = "Pentium III/Pentium III Xeon - internal L2 cache";
						break;
				}
				break;
			case 15:
				break;
		}

		if(in->brand < 0x18) {
			if(in->signature == 0x000006B1 || in->signature == 0x00000F13) {
				in->str_brand = cpuid_intel_brands_other[in->brand];
			} else {
				in->str_brand = cpuid_intel_brands[in->brand];
			}
		} else {
			in->str_brand = "Reserved";
		}
	} else if(in->manufacturer == kCPUManufacturerAMD) {
		switch(in->family) {
			case 4:
				in->str_model = "486 Model %d";
				break;
			case 5:
				switch(in->model) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 6:
					case 7:
						in->str_model = "K6 Model %d";
						break;
					case 8:
						in->str_model = "K6-2 Model 8 %d";
						break;
					case 9:
						in->str_model = "K6-III Model 9 %d";
						break;
					default:
						in->str_model = "K5/K6 Model %d";
						break;
				}
				break;
			case 6:
				switch(in->model) {
					case 1:
					case 2:
					case 4:
						in->str_model = "Athlon Model %d";
						break;
					case 3:
						in->str_model = "Duron Model 3";
						break;
					case 6:
						in->str_model = "Athlon MP/Mobile Athlon Model 6";
						break;
					case 7:
						in->str_model = "Mobile Duron Model 7";
						break;
					default:
						in->str_model = "Duron/Athlon Model %d";
						break;
				}
				break;
		}
	}
}

/*
 * Detects Intel-specific stuff.
 */
cpu_info_t* cpuid_detect_intel() {
	cpu_info_t* cpu_info = (cpu_info_t *) kmalloc(sizeof(cpu_info_t));
	memset(cpu_info, 0x00, sizeof(cpu_info_t));

	uint32_t eax, ebx, ecx, edx, max_eax, signature, unused;
	uint32_t model, family, type, brand, stepping, reserved;
	uint32_t extended_family = -1;
	
	cpuid(1, eax, ebx, unused, unused);
	
	model = (eax >> 4) & 0xf;
	family = (eax >> 8) & 0xf;
	type = (eax >> 12) & 0x3;
	brand = ebx & 0xff;
	stepping = eax & 0xf;
	reserved = eax >> 14;
	signature = eax;
	
	cpu_info->model = model;
	cpu_info->family = family;
	cpu_info->type = type;
	cpu_info->brand = brand;
	cpu_info->stepping = stepping;
	cpu_info->reserved = reserved;
	cpu_info->signature = signature;
	cpu_info->manufacturer = kCPUManufacturerIntel;

	if(family == 15) {
		extended_family = (eax >> 20) & 0xFF;
		cpu_info->extended_family = extended_family;
	}

	cpuid(0x80000000, max_eax, unused, unused, unused);

	/* Quok said: If the max extended eax value is high enough to support the processor brand string
	(values 0x80000002 to 0x80000004), then we'll use that information to return the brand information. 
	Otherwise, we'll refer back to the brand tables above for backwards compatibility with older processors. 
	According to the Sept. 2006 Intel Arch Software Developer's Guide, if extended eax values are supported, 
	then all 3 values for the processor brand string are supported, but we'll test just to make sure and be safe. */
	if(max_eax >= 0x80000004) {	
		char *det_name = (char *) kmalloc(sizeof(char) * 65);
		memset(det_name, 0x00, sizeof(char) * 65);
		uint8_t str_offset = 0x00;

		if(max_eax >= 0x80000002) {
			cpuid(0x80000002, eax, ebx, ecx, edx);
			for(int w = 0; w < 4; w++) {
				det_name[w + str_offset] = eax >> (8 * w);
				det_name[w + 4 + str_offset] = ebx >> (8 * w);
				det_name[w + 8 + str_offset] = ecx >> (8 * w);
				det_name[w + 12 + str_offset] = edx >> (8 * w);
			}

			str_offset += 0x10;
		}

		if(max_eax >= 0x80000003) {
			cpuid(0x80000003, eax, ebx, ecx, edx);
			for(int w = 0; w < 4; w++) {
				det_name[w + str_offset] = eax >> (8 * w);
				det_name[w + 4 + str_offset] = ebx >> (8 * w);
				det_name[w + 8 + str_offset] = ecx >> (8 * w);
				det_name[w + 12 + str_offset] = edx >> (8 * w);
			}

			str_offset += 0x10;
		}

		if(max_eax >= 0x80000004) {
			cpuid(0x80000004, eax, ebx, ecx, edx);
			for(int w = 0; w < 4; w++) {
				det_name[w + str_offset] = eax >> (8 * w);
				det_name[w + 4 + str_offset] = ebx >> (8 * w);
				det_name[w + 8 + str_offset] = ecx >> (8 * w);
				det_name[w + 12 + str_offset] = edx >> (8 * w);
			}

			str_offset += 0x10;
		}

		cpu_info->detected_name = det_name;
	}

	return cpu_info;
}

/*
 * Detects AMD-specific stuff
 */
cpu_info_t* cpuid_detect_amd() {
	cpu_info_t* cpu_info = (cpu_info_t *) kmalloc(sizeof(cpu_info_t));
	memset(cpu_info, 0x00, sizeof(cpu_info_t));

	uint32_t extended, eax, ebx, ecx, edx, unused;
	uint32_t family, model, stepping, reserved;
	
	cpuid(1, eax, unused, unused, unused);
	
	model = (eax >> 4) & 0x0F;
	family = (eax >> 8) & 0x0F;
	stepping = eax & 0x0F;
	reserved = eax >> 12;
	
	cpu_info->family = family;
	cpu_info->stepping = stepping;
	cpu_info->model = model;
	cpu_info->reserved = reserved;
	cpu_info->manufacturer = kCPUManufacturerAMD;
	
	cpuid(0x80000000, extended, unused, unused, unused);
	
	if(extended == 0) {
		return cpu_info;
	}

	cpu_info->extended = extended;

	if(extended >= 0x80000002) {
		uint32_t j;
		char *det_name = (char *) kmalloc(sizeof(char) * 65);
		memset(det_name, 0x00, sizeof(char) * 65);

		uint8_t str_offset = 0x00;

		for(j = 0x80000002; j <= 0x80000004; j++) {
			cpuid(j, eax, ebx, ecx, edx);

			for(int w = 0; w < 4; w++) {
				det_name[w + str_offset] = eax >> (8 * w);
				det_name[w + 4 + str_offset] = ebx >> (8 * w);
				det_name[w + 8 + str_offset] = ecx >> (8 * w);
				det_name[w + 12 + str_offset] = edx >> (8 * w);
			}

			str_offset += 0x10;
		}

		cpu_info->detected_name = det_name;
	}

	if(extended >= 0x80000007) {
		cpuid(0x80000007, unused, unused, unused, edx);
		if(edx & 1) {
			cpu_info->temp_diode = true;
		}
	}
	
	return cpu_info;
}
