#include <dbg.h>

#include "antidebug.h"
#include "xorstr.h"

#ifdef _WIN32
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#endif

namespace AntiDebug
{
	typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);

	bool Check()
	{
	#ifdef _WIN32
		if ( IsDebuggerPresent() )
		{
			return true;
		}

		DWORD pNtGlobalFlag = NULL;
		PPEB pPeb = (PPEB)__readfsdword(0x30);
		pNtGlobalFlag = *(PDWORD)((PBYTE)pPeb + 0x68);

		if ( pNtGlobalFlag & 0x70 )
		{
			return true;
		}

		BOOL IsDbgPresent = FALSE;
		CheckRemoteDebuggerPresent(GetCurrentProcess(), &IsDbgPresent);
		
		if ( IsDbgPresent )
		{
			return true;
		}

		pNtQueryInformationProcess NtQueryInfoProcess = (pNtQueryInformationProcess)GetProcAddress(LoadLibrary(xs("ntdll.dll")), xs("NtQueryInformationProcess"));

		unsigned long DbgPort = 0;

		NTSTATUS status = NtQueryInfoProcess(GetCurrentProcess(), 7, &DbgPort, sizeof(DbgPort), NULL);

		if ( NT_SUCCESS(status) && DbgPort != 0 )
		{
			return true;
		}

	#define ProcessDebugObjectHandle 0x1E

		HANDLE hDebugObject;

		status = NtQueryInfoProcess(GetCurrentProcess(), ProcessDebugObjectHandle, &hDebugObject, sizeof(hDebugObject), NULL);

		if ( NT_SUCCESS(status) && hDebugObject )
		{
			return true;
		}

	#define ProcessDebugFlags 0x1F
		DWORD processDebugFlags = 0;

		status = NtQueryInfoProcess(GetCurrentProcess(), ProcessDebugFlags, &processDebugFlags, sizeof(processDebugFlags), NULL);

		if ( NT_SUCCESS(status) && processDebugFlags == 0 )
		{
			return true;
		}

		PROCESS_BASIC_INFORMATION baseInf;

		NtQueryInfoProcess(GetCurrentProcess(), ProcessBasicInformation, &baseInf, sizeof(baseInf), NULL);

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		
		if ( hSnapshot )
		{
			PROCESSENTRY32 pe32;
			pe32.dwSize = sizeof(PROCESSENTRY32);

			if ( Process32First(hSnapshot, &pe32) )
			{
				do
				{
					if ( pe32.th32ProcessID == baseInf.UniqueProcessId )
					{
						//Warning("%s\n", pe32.szExeFile);
						break;
					}
				}
				while ( Process32Next(hSnapshot, &pe32) );
			}

			CloseHandle(hSnapshot);
		}

		CONTEXT context = {};
		context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

		GetThreadContext(GetCurrentThread(), &context);

		if ( context.Dr0 != 0 || context.Dr1 != 0 || context.Dr2 != 0 || context.Dr3 != 0 )
		{
			return true;
		}
	#endif

		return false;
	}
}