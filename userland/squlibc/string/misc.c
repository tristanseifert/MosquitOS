#include "string_internal.h"

int isupper(int ch) {
	return (ch >= 'A' && ch <= 'Z');
}

int isdigit(int ch) {
	return (ch >= '0' && ch <= '9');
}

int isspace(int ch) {
	return (ch == ' ');
}

int isalpha(int ch) {
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}