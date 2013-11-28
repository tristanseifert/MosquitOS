/*
 * SQULibC - String Functions
 */
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
char* strsep(char **stringp, const char *delim);
char* strtok(char *s, const char *delim);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);