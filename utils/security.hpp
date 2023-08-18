#ifndef SECURITY
#define SECURITY

#define SECURITY_CHECKS ( 1 )
#define SECURITY_CHECKS_DEBUG ( 1 )

#include <platform.h>
#include <dbg.h>

#include <Windows.h>
#include <Winternl.h>
#include <string>

#include "xorstr.h"

//-----------------------------------------------------------------------------
// Main namespace for security
//-----------------------------------------------------------------------------

namespace security 
{
	namespace debug 
	{
		namespace global_flags
		{
			extern int whoami;

			extern bool byobfuscatedexit;
		}

		int __cdecl vm_handler(EXCEPTION_RECORD* p_rec, void* est, unsigned char* p_context, void* disp);
		void to_lower(unsigned char* input);
		LPCSTR get_string(int index);

		//dynamically resolved functions
		typedef NTSTATUS(__stdcall* _NtQueryInformationProcess)(_In_ HANDLE, _In_  unsigned int, _Out_ PVOID, _In_ ULONG, _Out_ PULONG);
		typedef NTSTATUS(__stdcall* _NtSetInformationThread)(_In_ HANDLE, _In_ THREAD_INFORMATION_CLASS, _In_ PVOID, _In_ ULONG);

		//enum for the results of the antidebugger
		enum results
		{
			//nothing was caught, value = 0
			none = 0x0000,

			//something caught in memory (0x1000 - 0x1009)
			being_debugged_peb = 0x1000,
			remote_debugger_present = 0x1001,
			debugger_is_present = 0x1002,
			dbg_global_flag = 0x1003,
			nt_query_information_process = 0x0004,
			find_window = 0x1005,
			output_debug_string = 0x1006,
			nt_set_information_thread = 0x1007,
			debug_active_process = 0x1008,
			write_buffer = 0x1009,

			//something caught in exceptions (0x2000 - 0x2005)
			close_handle_exception = 0x2000,
			single_step = 0x2001,
			int_3_cc = 0x2002,
			int_2 = 0x2003,
			prefix_hop = 0x2004,
			debug_string = 0x2005,
			multibyte_int_3_cd = 0x2006,
			int_2c = 0x2007,

			//something caught with timings (0x3000 - 0x3002)
			rdtsc = 0x3000,
			query_performance_counter = 0x3001,
			get_tick_count = 0x3002,

			//something caught in cpu (0x4000 - 0x4001)
			hardware_debug_registers = 0x4000,
			mov_ss = 0x4001,

			//virtualization (0x5000 - 0x5003)
			check_cpuid = 0x5000,
			check_registry = 0x5001,
			vm = 0x5002,
		};

		namespace memory 
		{
			int being_debugged_peb();
			int remote_debugger_present();
			int check_window_name();
			int is_debugger_present();
			int nt_global_flag_peb();
			int nt_query_information_process();
			int nt_set_information_thread();
			int debug_active_process();
			int write_buffer();
		}

		namespace exceptions 
		{
			int close_handle_exception();
			int single_step_exception();
			int multibyte_int3();
			int int_3();
			int int_2c();
			int int_2d();
			int prefix_hop();
			int debug_string();
		}

		namespace timing {
			int rdtsc();
			int query_performance_counter();
			int get_tick_count();
		}

		namespace cpu {
			int hardware_debug_registers();
			int mov_ss();
		}

		namespace virtualization {
			int check_cpuid();
			int check_registry();
			int vm();
		}

		debug::results check();

		extern unsigned int randompick;

		extern int (*funcs[])();

		extern int (*picked)();

		extern void dispatch();

		extern int decoy();
	}

	namespace utils
	{
		extern void obfuscate_exit_antidebug();
		extern void obfuscate_entry_antidebug(void *ptr);

		typedef void ( *AntiDebugPtr )( );
		typedef void ( *ExitPtr )( int );

		extern unsigned int randomize();
	}
}

//-----------------------------------------------------------------------------
// Debugging countermeasures
//-----------------------------------------------------------------------------

// Throws every check
FORCEINLINE void AntiDebug()
{
#if SECURITY_CHECKS
	auto check_result = security::debug::check();

	if ( check_result != security::debug::results::none )
	{
	#if SECURITY_CHECKS_DEBUG
		FILE *file = fopen( xs( "security_check_result.txt" ), xs( "w" ) );

		if ( file != NULL )
		{
			fprintf( file, xs( "0x%X\n" ), (int)check_result );
			fclose( file );
		}
	#endif

		security::utils::obfuscate_exit_antidebug();
	}
#endif
}

// Checks with a random method
FORCEINLINE void EasyAntiDebug() 
{
#if SECURITY_CHECKS
	security::debug::dispatch();

	//Msg( "%u\n", security::debug::randompick );

	auto check_result = security::debug::picked();

	if ( check_result != security::debug::results::none )
	{
		security::debug::picked = security::debug::decoy;

	#if SECURITY_CHECKS_DEBUG

		FILE* file = fopen( xs( "security_check_result_2.txt" ), xs( "w" ) );

		if ( file != NULL )
		{
			fprintf( file, xs( "0x%X\n" ), (int)check_result );
			fprintf( file, xs( "%u\n" ), (int)security::debug::randompick );
			fclose( file );
		}
	#endif

		security::utils::obfuscate_exit_antidebug();
	}
	security::debug::picked = security::debug::decoy;
#endif
}

FORCEINLINE void GetHash(void* pointer)
{
	// TODO
}

FORCEINLINE void HashCmp()
{
	// TODO
}

#endif