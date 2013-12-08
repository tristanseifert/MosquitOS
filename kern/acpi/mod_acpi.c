#include <types.h>
#include "mod_acpi.h"
#include "sys/paging.h"
#include "sys/sched.h"

#include <acpi.h>

extern page_directory_t *kernel_directory;

/*
 * Initialises ACPI.
 */
static int acpi_init(void) {
	ACPI_STATUS status;

	// ACPICA subsystem
	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status)) {
		kprintf("ACPI: ACPICA init failed (%i)\n", status);
		return status;
	}

	// ACPICA table manager and get all tables
	status = AcpiInitializeTables(NULL, 16, FALSE);
	if (ACPI_FAILURE(status)) {
		kprintf("ACPI: Table manager initialisation failed (%i)\n", status);
		return status;
	}

	// Create namespace from tables
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status)) {
		kprintf("ACPI: Namespace initialisation failed (%i)\n", status);
		return status;
	}

	// If we had local handlers, we would install them here.

	// Initialise the ACPI subsystem
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		kprintf("ACPI: ACPI subsystem enable failed (%i)\n", status);
		return status;
	}

	// Complete namespace initialisation.
	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		kprintf("ACPI: Object initialisation failed (%i)\n", status);
		return status;
	}

	kprintf("ACPI: Initialised.\n");
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

/*
 * Maps Length bytes, starting at PhysicalAddress, somewhere in virtual memory.
 * Note that ACPICA expects the returned pointer to not be the start of the
 * pages mapped, but the offset into the first page, i.e. PhysicalAddress
 * logical ANDed with 0x00000FFF.
 */
void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
	uint32_t location = paging_map_section(PhysicalAddress, (uint32_t) Length, kernel_directory, kMemorySectionHardware);
	return (void *) location;
}

/*
 * Unmaps a memory segment at virtual address where of length Length.
 */
void AcpiOsUnmapMemory(void *where, ACPI_SIZE length) {
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
	return malloc(size);
}

/*
 * Deallocate memory that was previously allocated through AcpiOsAllocate.
 */
inline void AcpiOsFree(void *memory) {
	kfree(memory);
}

/*
 * Checks if the memory pointed to by Memory is readable.
 */
inline BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) {
	return true;
}

/*
 * Checks if the memory at Memory is writeable.
 */
inline BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) {
	return true;
}

/*
 * Returns the unique ID of the currently running thread/process.
 */
ACPI_THREAD_ID AcpiOsGetThreadId() {
	return ((i386_task_t *) sched_curr_task())->pid;
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
 * Creates a new mutex, putting the pointer to its memory into OutHandle.
 */
ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle) {

}

/*
 * Deletes the mutex.
 */
void AcpiOsDeleteMutex(ACPI_MUTEX Handle) {

}

/*
 * Attempt to acquire the specified mutex. If Timeout is 0, this function will
 * return if the mutex is not free. If Timeout is 1 to positive infinity, it
 * will wait at most that many milliseconds for the mutex to become free. If
 * the parameter is -1 (0xFFFF), it will wait until the mutex becomes available,
 * without a timeout.
 */
ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout) {

}

/*
 * Releases a previously created mutex.
 */
void AcpiOsReleaseMutex(ACPI_MUTEX Handle) {

}

/*
 * Creates a new semaphore whose counter is initialised to InitialUnits, and
 * whose address is put into OutHandle. MaxUnits is ignored.
 */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {

}

/*
 * Deletes a previously created semaphore.
 */
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {

}

/*
 * Waits for Units amount of units on the semaphore. Timeout is the same as in
 * AcpiOsAcquireMutex.
 */
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {

}

/*
 * Signals Units number of units on the specified semaphore.
 */
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {

}

/*
 * Creates a new spinlock and places its address in OutHandle. Note that a
 * spinlock disables scheduling and interrupts on the current CPU.
 */
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {

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
	return AE_OK;
}

/*
 * Removes Handler as a handler for IRQ level InterruptLevel.
 */
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler) {
	return AE_OK;
}