#ifndef MODULE_H
#define MODULE_H

typedef int (*module_initcall_t)(void);
typedef void (*module_exitcall_t)(void);

#define module_init(fn)	__define_initcall(fn, 1)
#define module_early_init(fn)	__define_initcall(fn, 0)
#define module_exit(fn)	__exitcall(fn)

// Plops pointers to a module's init function into the appropraite section
#define __define_initcall(fn, id) \
	static module_initcall_t __initcall_##fn##id __used \
	__attribute__((__section__(".initcall" #id ".init"))) = fn

#define __exitcall(fn) \
	static module_exitcall_t __exitcall_##fn __used \
	__attribute__((__section__(".exitcall.exit"))) = fn

void modules_load();

#endif