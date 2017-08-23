// stacktrace.h (c) 2008, Timo Bingmann from http://idlebox.net/
// published under the WTFPL v2.0

// Downloaded from http://panthema.net/2008/0901-stacktrace-demangled/
// and modified for C++ and FC by Steemit, Inc.

#include <fc/stacktrace.hpp>

#ifdef __GNUC__

#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace fc {

void print_stacktrace(std::ostream& out, unsigned int max_frames /* = 63 */, void* caller_overwrite_hack /* = nullptr */ )
{
    out << "stack trace:" << std::endl;

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
	out << "  <empty, possibly corrupt>" << std::endl;
	return;
    }

    if( caller_overwrite_hack != nullptr )
        addrlist[2] = caller_overwrite_hack;

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++)
    {
	char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

	// find parentheses and +address offset surrounding the mangled name:
	// ./module(function+0x15c) [0x8048a6d]
	for (char *p = symbollist[i]; *p; ++p)
	{
	    if (*p == '(')
		begin_name = p;
	    else if (*p == '+')
		begin_offset = p;
	    else if (*p == ')' && begin_offset)
            {
		end_offset = p;
		break;
	    }
	}

        out << "  0x" << std::setfill('0') << std::setw(16) << std::hex << std::noshowbase << uint64_t(addrlist[i]);

	if (begin_name && begin_offset && end_offset
	    && begin_name < begin_offset)
	{
	    *begin_name++ = '\0';
	    *begin_offset++ = '\0';
	    *end_offset = '\0';

	    // mangled name is now in [begin_name, begin_offset) and caller
	    // offset in [begin_offset, end_offset). now apply
	    // __cxa_demangle():

	    int status;
	    char* ret = abi::__cxa_demangle(begin_name,
					    funcname, &funcnamesize, &status);
	    if (status == 0)
            {
		funcname = ret; // use possibly realloc()-ed string
                out << " " << symbollist[i] << " : " << funcname << "+" << begin_offset << std::endl;
	    }
	    else
            {
		// demangling failed. Output function name as a C function with
		// no arguments.
                out << " " << symbollist[i] << " : " << begin_name << "+" << begin_offset << std::endl;
	    }
	}
	else
	{
	    // couldn't parse the line? print the whole line.
            out << " " << symbollist[i] << std::endl;
	}
    }

    free(funcname);
    free(symbollist);
}

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext
{
   unsigned long     uc_flags;
   struct ucontext*  uc_link;
   stack_t           uc_stack;
   struct sigcontext uc_mcontext;
   sigset_t          uc_sigmask;
} sig_ucontext_t;

// This function is based on https://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
void segfault_handler(int sig_num, siginfo_t * info, void * ucontext)
{
   void*              caller_address;
   sig_ucontext_t*    uc;

   uc = (sig_ucontext_t *)ucontext;

   /* Get the address at the time the signal was raised */
#if defined(__i386__) // gcc specific
   caller_address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific
#elif defined(__x86_64__) // gcc specific
   caller_address = (void *) uc->uc_mcontext.rip; // RIP: x86_64 specific
#else
#error Unsupported architecture. // TODO: Add support for other arch.
#endif

   std::cerr << "without overwrite:" << std::endl;
   print_stacktrace( std::cerr, 128, nullptr );
   std::cerr << "with overwrite:" << std::endl;
   print_stacktrace( std::cerr, 128, caller_address );
   std::exit(EXIT_FAILURE);
}

void print_stacktrace_on_segfault()
{
   struct sigaction sigact;

   sigact.sa_sigaction = segfault_handler;
   sigact.sa_flags = SA_RESTART | SA_SIGINFO;

   if( sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0 )
   {
      std::cerr << "Error setting signal handler" << std::endl;
      std::exit(EXIT_FAILURE);
   }
}

}

#else

namespace fc {

void print_stacktrace(std::ostream& out, unsigned int max_frames /* = 63 */ )
{
    out << "stack trace not supported on this compiler" << std::endl;
}

void print_stacktrace_on_segfault() {}

}

#endif
