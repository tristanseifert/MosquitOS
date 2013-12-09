#include <types.h>
#include "mod_acpi.h"
#include "sys/paging.h"
#include "sys/sched.h"

#include <acpi.h>

void console_putc(char c);

extern char* printf_buffer;
extern page_directory_t *kernel_directory;

/*
 * Initialises ACPI.
 */
static int acpi_init(void) {
	ACPI_STATUS status;

	// ACPICA subsystem
	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status)) {
		kprintf("ACPICA init failed (%i)\n", status);
		return status;
	}

	// ACPICA table manager and get all tables
	status = AcpiInitializeTables(NULL, 16, FALSE);
	if (ACPI_FAILURE(status)) {
		kprintf("Table manager initialisation failed (%i)\n", status);
		return status;
	}

	// Create namespace from tables
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status)) {
		kprintf("Namespace initialisation failed (%i)\n", status);
		return status;
	}

	// If we had local handlers, we would install them here.

	// Initialise the ACPI subsystem
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		kprintf("ACPI subsystem enable failed (%i)\n", status);
		return status;
	}

	// Complete namespace initialisation.
	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		kprintf("Object initialisation failed (%i)\n", status);
		return status;
	}
	return 0;
}

module_early_init(acpi_init);

/*
 * Called during ACPICA initialisation/shutdown. Do nothing.
 */
ACPI_STATUS AcpiOsInitialize() {
	return AE_OK;
}

ACPI_STATUS AcpiOsTerminate() {
	return AE_OK;
}

/*
 * Find root pointer of the RSDP.
 */
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
	ACPI_SIZE ret;
	AcpiFindRootPointer(&ret);
	return ret;
}

/*
 * Functions to allow us to override a system table. Currently does nothing.
 */
ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue) {
	NewValue = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
	NewTable = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength) {
	NewAddress = NULL;
	return AE_OK;
}

/*
 * Maps Length bytes, starting at PhysicalAddress, somewhere in virtual memory.
 * Note that ACPICA expects the returned pointer to not be the start of the
 * pages mapped, but the offset into the first page, i.e. PhysicalAddress
 * logical ANDed with 0x00000FFF.
 */
void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
	// Below 0x00100000 is identity mapped
	if(PhysicalAddress < 0x00100000) {
		return (void *) PhysicalAddress;
	}

	uint32_t location = paging_map_section(PhysicalAddress, (uint32_t) Length, kernel_directory, kMemorySectionHardware);
	//kprintf("ACPI: Mapped 0x%X to virtual 0x%X (len = 0x%X)\n", PhysicalAddress, location, Length);
	paging_flush_tlb(location);

	return (void *) location;
}

/*
 * Unmaps a memory segment at virtual address where of length Length.
 */
void AcpiOsUnmapMemory(void *where, ACPI_SIZE length) {
	// Below 0x00100000 is identity mapped and the kernel needs it, don't unmap
	if((uint32_t) where < 0x00100000) {
		return;
	}

	paging_unmap_section((uint32_t) where, length, kernel_directory);
}

/*
 * Looks up the physical address that the logical address in LogicalAddress
 * represents, then places it in PhysicalAddress.
 */
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {
	if(!LogicalAddress || !PhysicalAddress) return AE_BAD_PARAMETER;

	// Try to get the page at the specified virtual address
	page_t* page = paging_get_page((uint32_t) (((uint32_t) LogicalAddress) & 0xFFFFF000), false, kernel_directory);

	if(!page) {
		return AE_ERROR;
	} else {
		PhysicalAddress = (void *) ((page->frame << 12) | (((uint32_t) LogicalAddress) & 0x00000FFF));
		return AE_OK;
	}
}

/*
 * Allocate size bytes of memory.
 */
inline void *AcpiOsAllocate(ACPI_SIZE size) {
	void* ptr = (void *) kmalloc(size);
	memclr(ptr, size);
	return ptr;
}

/*
 * Deallocate memory that was previously allocated through AcpiOsAllocate.
 */
inline void AcpiOsFree(void *memory) {
	//kfree(memory);
}

/*
 * Checks if the memory pointed to by Memory is readable.
 */
inline BOOLEAN AcpiOsReadable(void *memory, ACPI_SIZE Length) {
	return true;
}

/*
 * Checks if the memory at Memory is writeable.
 */
inline BOOLEAN AcpiOsWritable(void *memory, ACPI_SIZE Length) {
	return true;
}

/*
 * Returns the unique ID of the currently running thread/process.
 */
ACPI_THREAD_ID AcpiOsGetThreadId() {
	uint32_t pid = ((i386_task_t *) sched_curr_task())->pid;

	if(pid == 0) {
		pid = 0xDEADBEEF;
	}

	return pid;
}

/*
 * Spawns a new thread or process with the entry point Function, with Context
 * pushed on the stack as the parameter. The invocation by the scheduler should
 * function exactly as doing the following:
 *
 * Function(Context);
 */
ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context) {
	return AE_ERROR;
}

/*
 * Sleeps the current thread for Milliseconds.
 */
void AcpiOsSleep(UINT64 Milliseconds) {

}

/*
 * Stalls the current thread for the specified number of microseconds. Note that
 * this does not cause the thread to be put into the sleep queue: it will spin
 * until either the time expires or the OS pre-empts it.
 */
void AcpiOsStall(UINT32 Microseconds) {

}

/*
 * Waits for all threads/processes started by ACPICA to exit.
 */
void AcpiOsWaitEventsComplete(void) {

}

/*
 * Creates a new semaphore whose counter is initialised to InitialUnits, and
 * whose address is put into OutHandle. MaxUnits is ignored.
 */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
	return AE_OK;
}

/*
 * Deletes a previously created semaphore.
 */
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
	return AE_OK;
}

/*
 * Waits for Units amount of units on the semaphore. Timeout is the same as in
 * AcpiOsAcquireMutex.
 */
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
	return AE_OK;
}

/*
 * Signals Units number of units on the specified semaphore.
 */
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
	return AE_OK;
}

/*
 * Creates a new spinlock and places its address in OutHandle. Note that a
 * spinlock disables scheduling and interrupts on the current CPU.
 */
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
	return AE_OK;
}

/*
 * Deletes a spinlock.
 */
void AcpiOsDeleteLock(ACPI_HANDLE Handle) {

}

/*
 * Acquire a spinlock, and return a set of flags that can be used to restore
 * machine state in AcpiOsReleaseLock.
 */
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
	return -1;
}

/*
 * Releases a spinlock. Flags is the value returned by AcpiOsAcquireLock.
 */
void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
	
}

/*
 * Registers Handler as an IRQ handler for IRQ InterruptLevel, passing Context
 * to it when called.
 */
ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context) {
	kprintf("ACPI: Register IRQ handler for level %i at 0x%X\n", InterruptLevel, Handler);
	return AE_OK;
}

/*
 * Removes Handler as a handler for IRQ level InterruptLevel.
 */
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler) {
	return AE_OK;
}

/*
 * Interface to the operating system's printf()-like output facilities.
 */
void AcpiOsPrintf(const char *format, ...) {
	memclr(printf_buffer, 0x1000);

	char* buffer = (char *) printf_buffer;
	va_list ap;
	va_start(ap, format);
	int n = vsprintf(buffer, format, ap);
	va_end(ap);
	while(*buffer != 0x00) console_putc(*buffer++);
}

void AcpiOsVprintf(const char *format, va_list va) {
	memclr(printf_buffer, 0x1000);

	char* buffer = (char *) printf_buffer;
	int n = vsprintf(printf_buffer, format, va);
	while(*buffer != 0x00) console_putc(*buffer++);
}

/*
 * Reads from virtual memory at Address, and reads Width bits to memory pointed
 * to by Value.
 */
ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
	switch(Width) {
		case 8: {
			uint8_t *read = (uint8_t *) Address;
			*Value = *(read) & 0xFF;
			break;
		}
		case 16: {
			uint16_t *read = (uint16_t *) Address;
			*Value = *(read) & 0xFFFF;
			break;
		}
		case 32: {
			uint32_t *read = (uint32_t *) Address;
			*Value = *(read) & 0xFFFFFFFF;
			break;
		}

		default:
			kprintf("Unhandled ACPI read of %i bits from 0x%X\n", Width, Address);
			return AE_ERROR;
			break;
	}

	return AE_OK;
}

/*
 * Writes Width bits from Value to the virtual address Address.
 */
ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
	switch(Width) {
		case 8: {
			uint8_t *write = (uint8_t *) Address;
			*write = Value & 0xFF;
			break;
		}
		case 16: {
			uint16_t *write = (uint16_t *) Address;
			*write = Value & 0xFFFF;
			break;
		}
		case 32: {
			uint32_t *write = (uint32_t *) Address;
			*write = Value & 0xFFFFFFFF;
			break;
		}

		default:
			kprintf("Unhandled ACPI write of %i bits to 0x%X\n", Width, Address);
			return AE_ERROR;
			break;
	}

	return AE_OK;
}

/*
 * Performs a read from an IO port.
 */
ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
	switch(Width) {
		case 8: {
			uint32_t ret = 0;
			__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"((uint16_t) Address));
			*Value = ret;
			break;
		}
		case 16: {
			uint32_t ret = 0;
			__asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"((uint16_t) Address));
			*Value = ret;
			break;
		}
		case 32: {
			uint32_t ret = 0;
			__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"((uint16_t) Address));
			*Value = ret;
			break;
		}

		default:
			kprintf("Unhandled ACPI IO Read of %i bits to 0x%X\n", Width, Address);
			return AE_ERROR;
			break;
	}

	return AE_OK;
}

/*
 * Writes Value the IO port at Address.
 */
ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
	switch(Width) {
		case 8: {
			__asm__ volatile("outb %0, %1" : : "a"((uint8_t) Value), "Nd"((uint16_t) Address));
			break;
		}
		case 16: {
			__asm__ volatile("outw %0, %1" : : "a"((uint16_t) Value), "Nd"((uint16_t) Address));
			break;
		}
		case 32: {
			__asm__ volatile("outl %0, %1" : : "a"((uint32_t) Value), "Nd"((uint16_t) Address));
			break;
		}

		default:
			kprintf("Unhandled ACPI IO Write of %i bits to 0x%X\n", Width, Address);
			return AE_ERROR;
			break;
	}

	return AE_OK;
}

/*
 * Forms a PCI config space address from the ACPI parameters.
 */
static inline uint32_t AcpiOSGetPCIAddress(ACPI_PCI_ID *PciId, UINT32 Register) {
	return (uint32_t) ((PciId->Segment << 16) | (PciId->Device << 11) | (PciId->Function<< 8) | (Register & 0xFC) | 0x80000000);
}

/*
 * Performs a read from PCI configuration space.
 */
ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Reg, UINT64 *Value, UINT32 Width) {
	AcpiOsWritePort(0xCF8, AcpiOSGetPCIAddress(PciId, Reg), 32);
	return AcpiOsReadPort(0xCFC, (UINT32 *) Value, Width);
}

/*
 * Performs a write to PCI configuration space.
 */
ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Reg, UINT64 Value, UINT32 Width) {
	AcpiOsWritePort(0xCF8, AcpiOSGetPCIAddress(PciId, Reg), 32);
	return AcpiOsWritePort(0xCFC, (UINT32) Value, Width);
}

/*
 * Returns the current system timer, in 100ns granularity.
 */
UINT64 AcpiOsGetTimer(void) {
	return 0;
}

/*
 * Allows ACPICA to signal either a fatal error or breakpoint to the OS. There
 * are triggered in response to AML opcodes.
 */
ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
	if(Function == ACPI_SIGNAL_FATAL) {
		PANIC("ACPI: Fatal opcode encountered");
	} else {
		kprintf("Encountered ACPI Signal %i (0x%X)\n", Function, Info);
	}

	return AE_OK;
}