#include <types.h>
#include <stdarg.h>

// Prototype of print function
typedef void (*putcf) (void*, char);

/*
 * Prints a character to a buffer.
 */
static void putcp(void* p, char c) {
	*(*((char**)p))++ = c;
}

/*
 * Prints an unsigned long in the base specified by base to *bf.
 */
static void kprintf_ulong2ascii(unsigned long int num, unsigned int base, bool uc, char* bf) {
	int n = 0;
	unsigned int d = 1;
	while (num/d >= base)
		d *= base;
	
	while (d != 0) {
		int dgt = num / d;
		num %= d;
		d /= base;
		if (n || dgt > 0|| d == 0) {
			*bf++ = dgt + (dgt < 10 ? '0' : (uc ? 'A' : 'a') - 10);
			++n;
		}
	}
	
	*bf = 0;
}

/*
 * Prints a signed long in the base specified by base to *bf.
 */
static void kprintf_long2ascii(long num, char* bf) {
	if (num < 0) {
		num =- num;
		*bf++ = '-';
	}

	kprintf_ulong2ascii(num, 10, 0, bf);
}

/*
 * Prints an unsigned integer in the base specified by base to *bf.
 */
static void kprintf_uint2ascii(unsigned int num, unsigned int base, bool uc, char* bf) {
	int n=0;
	unsigned int d=1;
	while (num/d >= base)
		d *= base;        
	while (d!=0) {
		int dgt = num / d;
		num%= d;
		d/=base;
		if (n || dgt>0 || d==0) {
			*bf++ = dgt + (dgt < 10 ? '0' : (uc ? 'A' : 'a') - 10);
			++n;
		}
	}
	*bf=0;
}

/*
 * Prints a signed integer in the base specified by base to *bf.
 */
static void kprintf_int2ascii(int num, char* bf) {
	if (num < 0) {
		num =- num;
		*bf++ = '-';
	}

	kprintf_uint2ascii(num, 10, 0, bf);
}

/*
 * Turns an ASCII character into an integer.
 */
static int kprintf_char2int(char ch) {
	if (ch >= '0' && ch <= '9') {
		return ch-'0';
	} else if (ch >= 'a' && ch <= 'f') {
		return ch-'a'+10;
	} else if (ch >= 'A' && ch <= 'F') {
		return ch-'A'+10;
	} else { 
		return -1;
	}
}

/*
 * Turns an ASCII string into an integer.
 */
static char kprintf_ascii2int(char ch, char** src, int base, int* nump) {
	char* p= *src;
	int num=0;
	int digit;
	
	while ((digit = kprintf_char2int(ch)) >= 0) {
		if (digit > base) break;
		num = num * base + digit;
		ch =* p++;
	}

	*src = p;
	*nump = num;
	return ch;
}

/*
 * Puts a string of characters into the buffer.
 */
static void kprintf_putchw(void* putp, putcf putf, int n, char z, char* bf) {
	char fc = z? '0' : ' ';
	char ch;
	char* p = bf;
	while (*p++ && n > 0)
		n--;
	while (n-- > 0) 
		putf(putp, fc);
	while ((ch = *bf++))
		putf(putp, ch);
}

/*
 * This function does the actual formatting
 */
void kprintf_format(void* putp, putcf putf, char *fmt, va_list va) {
	char bf[12];
	
	char ch;

	while ((ch =* (fmt++))) {
		if (ch != '%') 
			putf(putp, ch);
		else {
			char lz = 0;
			char lng = 0;
			int w = 0;
			ch =* (fmt++);
			
			if (ch =='0') {
				ch =* (fmt++);
				lz = 1;
			}
			
			if (ch >= '0' && ch <= '9') {
				ch = kprintf_ascii2int(ch, &fmt, 10, &w);
			}
			
			// If the specifier is prefixed with l, it will read a longword, or
			// 64 bits, instead of 32 bit.
			if (ch =='l') {
				ch =* (fmt++);
				lng = 1;
			}

			switch (ch) {
				case 0: // end of string
					goto abort;

				case 'u': case 'i': // unsigned int/long
					if (lng) {
						kprintf_ulong2ascii(va_arg(va, unsigned long int), 10, false, bf);
					} else {
						kprintf_uint2ascii(va_arg(va, unsigned int), 10, false, bf);
					}

					kprintf_putchw(putp, putf, w, lz, bf);
					break;
					
				case 'd': // signed int/long
					if (lng) {
						kprintf_long2ascii(va_arg(va, unsigned long int), bf);
					} else {
						kprintf_int2ascii(va_arg(va, int), bf);
					}

					kprintf_putchw(putp, putf, w, lz, bf);
					break;

				case 'x': case 'X': // print as hexadecimal
					if (lng) {
						kprintf_ulong2ascii(va_arg(va, unsigned long int), 16, (ch == 'X') ? true : false, bf);
					} else {
						kprintf_uint2ascii(va_arg(va, unsigned int), 16, (ch == 'X') ? true : false, bf);
					}

					kprintf_putchw(putp, putf, w, lz, bf);
					break;

				case 'c': // character
					putf(putp, (char) (va_arg(va, int)));
					break;

				case 's': // string
					kprintf_putchw(putp, putf, w, 0, va_arg(va, char*));
					break;

				case '%': // escaped percent sign
					putf(putp, ch);

				default: // unknown
					putf(putp, ch);
					break;
				}
			}
		}

	abort: ;
}

/*
 * Formats the string fmt, then stores it in the memory pointed to by s instead
 * of stdio.
 */
void sprintf(char* s, char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	kprintf_format(&s, putcp, fmt, va);
	putcp(&s, 0);
	va_end(va);
}