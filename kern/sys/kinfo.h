#ifndef KINFO_H
#define KINFO_H

#define KERN_BDATE __DATE__
#define KERN_BTIME __TIME__

#if (defined(__GNUC__) || defined(__GNUG__)) && !(defined(__clang__) || defined(__INTEL_COMPILER))
	#define KERN_COMPILER "GNU GCC " __VERSION__
#elif (defined(__ICC))
	#define KERN_COMPILER "Intel Compiler " __VERSION__
#elif (defined(__clang__))
	#define KERN_COMPILER "clang " __VERSION__
#endif

#endif