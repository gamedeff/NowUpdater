//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================
// Module name:         util.cpp
// Module date:         23.7.2009 - 16:29
// Module author:       Fedor Gavrilov aka Dorfe
// Module description:  
//===================================================================================
// Precompiled header:
//-----------------------------------------------------------------------------------
//#include "precompiled.h"
//#pragma hdrstop
//-----------------------------------------------------------------------------------
#include "util.h"
//-----------------------------------------------------------------------------------
#include <assert.h>
#include <locale.h>
//-----------------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------------
const int WCHAR_MAX_COUNT = 16;                                 // max number of string buffers
const int WCHAR_MAX_LENGTH = 1024;                              // max string length per buffer
//-----------------------------------------------------------------------------------
static wchar_t wchar_wideStr[WCHAR_MAX_COUNT][WCHAR_MAX_LENGTH];// circular buffer for wchar_t*
static char wchar_str[WCHAR_MAX_COUNT][WCHAR_MAX_LENGTH];       // circular buffer for char*
static int wchar_indexWchar = 0;                                // current index of circular buffer
static int wchar_indexChar = 0;                                 // current index of circular buffer
//-----------------------------------------------------------------------------------
// Convert char* string to wchar_t* string
//-----------------------------------------------------------------------------------
GAMEWARE_API const wchar_t* gwToWChar(const char *src)
{
	wchar_indexWchar = (++wchar_indexWchar) % WCHAR_MAX_COUNT;  // circulate index

	setlocale(LC_ALL, "");

	mbstowcs(wchar_wideStr[wchar_indexWchar], src, WCHAR_MAX_LENGTH); // copy string as wide char
	wchar_wideStr[wchar_indexWchar][WCHAR_MAX_LENGTH-1] = L'\0';      // in case when source exceeded max length

	return wchar_wideStr[wchar_indexWchar];                     // return string as wide char
}
//-----------------------------------------------------------------------------------
// Convert wchar_t* string to char* string
//-----------------------------------------------------------------------------------
GAMEWARE_API const char* gwToChar(const wchar_t* src)
{
	wchar_indexChar = (++wchar_indexChar) % WCHAR_MAX_COUNT;    // circulate index

	wcstombs(wchar_str[wchar_indexChar], src, WCHAR_MAX_LENGTH);// copy string as char
	wchar_str[wchar_indexChar][WCHAR_MAX_LENGTH-1] = '\0';      // in case when source exceeded max length

	return wchar_str[wchar_indexChar];                          // return string as char
}
//-----------------------------------------------------------------------------------
//char_t _FS_buffer[GW_MAX_STACK_FS_COUNT][GW_MAX_STACK_ERROR_STR_LEN];
//uint32_t _FS_index = 0;
//-----------------------------------------------------------------------------------
//GAMEWARE_API char_t *_FS(const char_t *format, ...)
//{
//	_FS_index = (++_FS_index) % GW_MAX_STACK_FS_COUNT;  // circulate index
//
//	va_list args;
//	int len;
//
//	va_start(args, format);
//		len = _vsctprintf(format, args) + 1; // _vsctprintf doesn't count terminating '\0'
//		assert(len <= GW_MAX_STACK_ERROR_STR_LEN);
//		_vstprintf(_FS_buffer[_FS_index], format, args);
//	va_end(args);
//
//	return _FS_buffer[_FS_index];
//}
//-----------------------------------------------------------------------------------
char _FS_buffer_narrow[GW_MAX_STACK_FS_COUNT][GW_MAX_STACK_ERROR_STR_LEN];
uint32_t _FS_index_narrow = 0;
//-----------------------------------------------------------------------------------
GAMEWARE_API char *_FS_narrow(const char *format, ...)
{
	_FS_index_narrow = (++_FS_index_narrow) % GW_MAX_STACK_FS_COUNT;  // circulate index

	va_list args;
	int len;

	va_start(args, format);
		len = _vscprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
		assert(len <= GW_MAX_STACK_ERROR_STR_LEN);
		vsprintf(_FS_buffer_narrow[_FS_index_narrow], format, args);
	va_end(args);

	return _FS_buffer_narrow[_FS_index_narrow];
}
//-----------------------------------------------------------------------------------
wchar_t _FS_buffer_wide[GW_MAX_STACK_FS_COUNT][GW_MAX_STACK_ERROR_STR_LEN];
uint32_t _FS_index_wide = 0;
//-----------------------------------------------------------------------------------
GAMEWARE_API wchar_t *_FS_wide(const wchar_t *format, ...)
{
	_FS_index_wide = (++_FS_index_wide) % GW_MAX_STACK_FS_COUNT;  // circulate index

	va_list args;
	int len;

	va_start(args, format);
		len = _vscwprintf(format, args) + 1; // _vscwprintf doesn't count terminating L'\0'
		assert(len <= GW_MAX_STACK_ERROR_STR_LEN);
		_vswprintf(_FS_buffer_wide[_FS_index_wide], format, args);
	va_end(args);

	return _FS_buffer_wide[_FS_index_wide];
}
//-----------------------------------------------------------------------------------
GAMEWARE_API char_t *string_replace(const char_t *s, const char_t *orig, char_t *rep)
{
	static char_t str[GW_MAX_STACK_FORMAT_STR_LEN];

	_tcscpy(str, s);

	char_t *ch;

	while((ch = (char_t *) _tcsstr(str, orig)) != NULL)
	{
		char_t buffer1[GW_MAX_STACK_FORMAT_STR_LEN];
		char_t buffer2[GW_MAX_STACK_FORMAT_STR_LEN];

		_tcsncpy(buffer1, str, ch - str);
		_tcsncpy(buffer2, str, ch - str + _tcslen(orig));

		buffer1[ch - str]                 = _T('\0');
		buffer2[ch - str + _tcslen(orig)] = _T('\0');

		if(_tcslen(orig) >= _tcslen(rep))
		{
			_stprintf(buffer1 + (ch - str), _T("%s%s"), rep, ch + _tcslen(orig));
		}
		else
		{
			_stprintf(buffer2 + (ch - str), _T("%s%s"), rep, ch + _tcslen(orig));
			_tcscpy(buffer1, buffer2/*, _tcslen(buffer1)*/);
		}

		_tcscpy(str, buffer1);
	}

	return str;
}
//-----------------------------------------------------------------------------------
#if 0

#include <windows.h>

typedef long (NTAPI *_NtQueryInformationProcess)(
	HANDLE ProcessHandle,
	DWORD ProcessInformationClass,
	PVOID ProcessInformation,
	DWORD ProcessInformationLength,
	PDWORD ReturnLength
	);

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PROCESS_BASIC_INFORMATION
{
	LONG ExitStatus;
	PVOID PebBaseAddress;
	ULONG_PTR AffinityMask;
	LONG BasePriority;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR ParentProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

PVOID GetPebAddress(HANDLE ProcessHandle)
{
	_NtQueryInformationProcess NtQueryInformationProcess =
		(_NtQueryInformationProcess)GetProcAddress(
		GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
	PROCESS_BASIC_INFORMATION pbi;

	NtQueryInformationProcess(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);

	return pbi.PebBaseAddress;
}

int GetProcessCommandLine(int pid)
{
	HANDLE processHandle;
	PVOID pebAddress;
	PVOID rtlUserProcParamsAddress;
	UNICODE_STRING commandLine;
	WCHAR *commandLineContents;

	if ((processHandle = OpenProcess(
		PROCESS_QUERY_INFORMATION | /* required for NtQueryInformationProcess */
		PROCESS_VM_READ, /* required for ReadProcessMemory */
		FALSE, pid)) == 0)
	{
		printf("Could not open process!\n");
		return GetLastError();
	}

	pebAddress = GetPebAddress(processHandle);

	/* get the address of ProcessParameters */
	if (!ReadProcessMemory(processHandle, (PCHAR)pebAddress + 0x10,
		&rtlUserProcParamsAddress, sizeof(PVOID), NULL))
	{
		printf("Could not read the address of ProcessParameters!\n");
		return GetLastError();
	}

	/* read the CommandLine UNICODE_STRING structure */
	if (!ReadProcessMemory(processHandle, (PCHAR)rtlUserProcParamsAddress + 0x40,
		&commandLine, sizeof(commandLine), NULL))
	{
		printf("Could not read CommandLine!\n");
		return GetLastError();
	}

	/* allocate memory to hold the command line */
	commandLineContents = (WCHAR *)malloc(commandLine.Length);

	/* read the command line */
	if (!ReadProcessMemory(processHandle, commandLine.Buffer,
		commandLineContents, commandLine.Length, NULL))
	{
		printf("Could not read the command line string!\n");
		return GetLastError();
	}

	/* print it */
	/* the length specifier is in characters, but commandLine.Length is in bytes */
	/* a WCHAR is 2 bytes */
	printf("%.*S\n", commandLine.Length / 2, commandLineContents);
	CloseHandle(processHandle);
	free(commandLineContents);

	return 1;
}

#include <tchar.h>
#include <tlhelp32.h>

//  Forward declarations:
BOOL GetProcessList();
BOOL ListProcessModules( DWORD dwPID );
BOOL ListProcessThreads( DWORD dwOwnerPID );
void printError( TCHAR* msg );

BOOL GetProcessList()
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		printError( TEXT("CreateToolhelp32Snapshot (of processes)") );
		return( FALSE );
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32First( hProcessSnap, &pe32 ) )
	{
		printError( TEXT("Process32First") ); // show cause of failure
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return( FALSE );
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		_tprintf( TEXT("\n\n=====================================================" ));
		_tprintf( TEXT("\nPROCESS NAME:  %s"), pe32.szExeFile );
		_tprintf( TEXT("\n-------------------------------------------------------" ));

		// Retrieve the priority class.
		dwPriorityClass = 0;
		hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
		if( hProcess == NULL )
			printError( TEXT("OpenProcess") );
		else
		{
			dwPriorityClass = GetPriorityClass( hProcess );
			if( !dwPriorityClass )
				printError( TEXT("GetPriorityClass") );
			CloseHandle( hProcess );
		}

		_tprintf( TEXT("\n  Process ID        = 0x%08X"), pe32.th32ProcessID );
		_tprintf( TEXT("\n  Thread count      = %d"),   pe32.cntThreads );
		_tprintf( TEXT("\n  Parent process ID = 0x%08X"), pe32.th32ParentProcessID );
		_tprintf( TEXT("\n  Priority base     = %d"), pe32.pcPriClassBase );
		if( dwPriorityClass )
			_tprintf( TEXT("\n  Priority class    = %d"), dwPriorityClass );

		// List the modules and threads associated with this process
		ListProcessModules( pe32.th32ProcessID );
		//ListProcessThreads( pe32.th32ProcessID );
		GetProcessCommandLine(pe32.th32ProcessID);

	} while( Process32Next( hProcessSnap, &pe32 ) );

	CloseHandle( hProcessSnap );
	return( TRUE );
}


BOOL ListProcessModules( DWORD dwPID )
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
	if( hModuleSnap == INVALID_HANDLE_VALUE )
	{
		printError( TEXT("CreateToolhelp32Snapshot (of modules)") );
		return( FALSE );
	}

	// Set the size of the structure before using it.
	me32.dwSize = sizeof( MODULEENTRY32 );

	// Retrieve information about the first module,
	// and exit if unsuccessful
	if( !Module32First( hModuleSnap, &me32 ) )
	{
		printError( TEXT("Module32First") );  // show cause of failure
		CloseHandle( hModuleSnap );           // clean the snapshot object
		return( FALSE );
	}

	// Now walk the module list of the process,
	// and display information about each module
	do
	{
		_tprintf( TEXT("\n\n     MODULE NAME:     %s"),   me32.szModule );
		_tprintf( TEXT("\n     Executable     = %s"),     me32.szExePath );
		_tprintf( TEXT("\n     Process ID     = 0x%08X"),         me32.th32ProcessID );
		_tprintf( TEXT("\n     Ref count (g)  = 0x%04X"),     me32.GlblcntUsage );
		_tprintf( TEXT("\n     Ref count (p)  = 0x%04X"),     me32.ProccntUsage );
		_tprintf( TEXT("\n     Base address   = 0x%08X"), (DWORD) me32.modBaseAddr );
		_tprintf( TEXT("\n     Base size      = %d"),             me32.modBaseSize );

	} while( 0 && Module32Next( hModuleSnap, &me32 ) );

	CloseHandle( hModuleSnap );
	return( TRUE );
}

BOOL ListProcessThreads( DWORD dwOwnerPID ) 
{ 
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
	THREADENTRY32 te32; 

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
	if( hThreadSnap == INVALID_HANDLE_VALUE ) 
		return( FALSE ); 

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32); 

	// Retrieve information about the first thread,
	// and exit if unsuccessful
	if( !Thread32First( hThreadSnap, &te32 ) ) 
	{
		printError( TEXT("Thread32First") ); // show cause of failure
		CloseHandle( hThreadSnap );          // clean the snapshot object
		return( FALSE );
	}

	// Now walk the thread list of the system,
	// and display information about each thread
	// associated with the specified process
	do 
	{ 
		if( te32.th32OwnerProcessID == dwOwnerPID )
		{
			_tprintf( TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID ); 
			_tprintf( TEXT("\n     Base priority  = %d"), te32.tpBasePri ); 
			_tprintf( TEXT("\n     Delta priority = %d"), te32.tpDeltaPri ); 
			_tprintf( TEXT("\n"));
		}
	} while( Thread32Next(hThreadSnap, &te32 ) ); 

	CloseHandle( hThreadSnap );
	return( TRUE );
}

void printError( TCHAR* msg )
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError( );
	FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		sysMsg, 256, NULL );

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while( ( *p > 31 ) || ( *p == 9 ) )
		++p;
	do { *p-- = 0; } while( ( p >= sysMsg ) &&
		( ( *p == '.' ) || ( *p < 33 ) ) );

	// Display the message
	_tprintf( TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
}

#endif
//-----------------------------------------------------------------------------------

//===================================================================================
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================