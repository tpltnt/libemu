/********************************************************************************
 *                               libemu
 *
 *                    - x86 shellcode emulation -
 *
 *
 * Copyright (C) 2007  Paul Baecher & Markus Koetter
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * 
 *             contact nepenthesdev@users.sourceforge.net  
 *
 *******************************************************************************/


#include "../config.h"

#define HAVE_GETOPT_H
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif


#include <stdint.h>

#define HAVE_UNISTD
#ifdef HAVE_UNISTD
# include <unistd.h>
#endif
#include <stdio.h>

#include <stdarg.h>


#include <errno.h>
#include <sys/select.h>

#include <sys/wait.h>

#ifdef HAVE_LIBCARGOS
#include <cargos-lib.h>
#endif


#include <sys/types.h>
#include <sys/socket.h>  

#include "emu/emu.h"
#include "emu/emu_memory.h"
#include "emu/emu_cpu.h"
#include "emu/emu_log.h"
#include "emu/emu_cpu_data.h"
#include "emu/emu_cpu_stack.h"
#include "emu/environment/emu_profile.h"
#include "emu/environment/emu_env.h"
#include "emu/environment/win32/emu_env_w32.h"
#include "emu/environment/win32/emu_env_w32_dll.h"
#include "emu/environment/win32/emu_env_w32_dll_export.h"
#include "emu/environment/win32/env_w32_dll_export_kernel32_hooks.h"
#include "emu/environment/linux/emu_env_linux.h"
#include "emu/emu_getpc.h"
#include "emu/emu_graph.h"
#include "emu/emu_string.h"
#include "emu/emu_hashtable.h"

#include "emu/emu_shellcode.h"


#include "userhooks.h"
#include "options.h"

#include <stdint.h>
#include <stdarg.h>


uint32_t user_hook_ExitProcess(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

/*
VOID WINAPI ExitProcess(
  UINT uExitCode
);
*/

	va_list vl;
	va_start(vl, hook);
	int exitcode = va_arg(vl,  int);
	va_end(vl);

	printf("%s(%i)\n", hook->hook.win->fnname, exitcode);


	opts.steps = 0;
	return 0;
}


uint32_t user_hook_ExitThread(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

/*
VOID ExitThread(
  DWORD dwExitCode
);
*/

	va_list vl;
	va_start(vl, hook);
	int exitcode = va_arg(vl,  int);
	va_end(vl);

	printf("%s(%i)\n", hook->hook.win->fnname, exitcode);

	opts.steps = 0;
	return 0;

}

uint32_t user_hook_CreateProcess(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);

	/* char *pszImageName				  = */ (void)va_arg(vl, char *);
	char *pszCmdLine                      = va_arg(vl, char *);               
	/* void *psaProcess, 				  = */ (void)va_arg(vl, void *);
	/* void *psaThread,  				  = */ (void)va_arg(vl, void *);
	/* bool fInheritHandles,              = */ (void)va_arg(vl, char *);
	/* uint32_t fdwCreate,                = */ (void)va_arg(vl, uint32_t);
	/* void *pvEnvironment             	  = */ (void)va_arg(vl, void *);
	/* char *pszCurDir                 	  = */ (void)va_arg(vl, char *);
	STARTUPINFO *psiStartInfo       	  = va_arg(vl, STARTUPINFO *);
	PROCESS_INFORMATION *pProcInfo  	  = va_arg(vl, PROCESS_INFORMATION *); 

	va_end(vl);
	printf("CreateProcess(%s)\n",pszCmdLine);

	if ( pszCmdLine != NULL && strncasecmp(pszCmdLine, "cmd", 3) == 0 )
	{
		pid_t pid;
		if ( (pid = fork()) == 0 )
		{ // child

			dup2(psiStartInfo->hStdInput,  fileno(stdin));
			dup2(psiStartInfo->hStdOutput, fileno(stdout));
			dup2(psiStartInfo->hStdError,  fileno(stderr));

			struct emu_hashtable_item *ehi = emu_hashtable_search(opts.override.commands.commands, "cmd");
			if (ehi != NULL)
				system((char *)ehi->value);
			else
				system("/bin/sh -c \"cd ~/.wine/drive_c/; wine 'c:\\windows\\system32\\cmd_orig.exe' \"");
			
			exit(EXIT_SUCCESS);
		}
		else
		{ // parent 
			pProcInfo->hProcess = pid;
			
		}
	}



	return 1;
}

uint32_t user_hook_WaitForSingleObject(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);


	/*
	DWORD WINAPI WaitForSingleObject(
	  HANDLE hHandle,
	  DWORD dwMilliseconds
	);
	*/

	va_list vl;
	va_start(vl, hook);

	int32_t hHandle = va_arg(vl, int32_t);
	/*int32_t dwMilliseconds = */ (void)va_arg(vl, int32_t);
	va_end(vl);

	int status;
	while(1)
	{
		if (waitpid(hHandle, &status, WNOHANG) != 0)
			break;
		sleep(1);
	}

	return 0;
}


uint32_t user_hook_exit(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);
	int code = va_arg(vl,  int);
	va_end(vl);

	printf("exit(%i)\n",code);
	opts.steps = 0;
	return 0;
}

uint32_t user_hook_accept(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);

	int s 					= va_arg(vl,  int);
	struct sockaddr* addr 	= va_arg(vl,  struct sockaddr *);
	socklen_t* addrlen 			= va_arg(vl,  socklen_t *);
	va_end(vl);

    return accept(s, addr, addrlen);
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


uint32_t user_hook_bind(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);

	int s 					= va_arg(vl,  int);
	struct sockaddr* addr 	= va_arg(vl,  struct sockaddr *);
	socklen_t addrlen = va_arg(vl,  socklen_t );

	if (opts.override.bind.host != NULL )
	{
		struct sockaddr_in *si = (struct sockaddr_in *)addr;
		si->sin_addr.s_addr = inet_addr(opts.override.bind.host);
	}

	if (opts.override.bind.port > 0)
	{
		struct sockaddr_in *si = (struct sockaddr_in *)addr;;
		si->sin_port = htons(opts.override.bind.port);
	}

	va_end(vl);

    return bind(s, addr, addrlen);
}

uint32_t user_hook_closesocket(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);
	int s 					= va_arg(vl,  int);
	va_end(vl);

    return close(s);
}

uint32_t user_hook_connect(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);

	int s 					= va_arg(vl,  int);
	struct sockaddr* addr 	= va_arg(vl,  struct sockaddr *);

	if (opts.override.connect.host != NULL )
	{
		struct sockaddr_in *si = (struct sockaddr_in *)addr;
		si->sin_addr.s_addr = inet_addr(opts.override.connect.host);
	}

	if (opts.override.connect.port > 0)
	{
		struct sockaddr_in *si = (struct sockaddr_in *)addr;;
		si->sin_port = htons(opts.override.connect.port);
	}


	socklen_t addrlen = va_arg(vl,  socklen_t);

	if (addrlen != sizeof(struct sockaddr))
	{
		addrlen = sizeof(struct sockaddr);
	}

	va_end(vl);

    return connect(s, addr, addrlen);
}

uint32_t user_hook_fclose(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);
	return 0;
}


uint32_t user_hook_fopen(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);
	return 0;
}

uint32_t user_hook_fwrite(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);
	return 0;
}



uint32_t user_hook_listen(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);

	int s 					= va_arg(vl,  int);
	int backlog			 	= va_arg(vl,  int);
	va_end(vl);

    return listen(s, backlog);
}

uint32_t user_hook_recv(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);

	int s = va_arg(vl,  int);
	char* buf = va_arg(vl,  char *);
	int len = va_arg(vl,  int);
	int flags = va_arg(vl,  int);
	va_end(vl);

	return recv(s, buf, len,  flags);
}

uint32_t user_hook_send(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);

	int s = va_arg(vl,  int);
	char* buf = va_arg(vl,  char *);
	int len = va_arg(vl,  int);
	int flags = va_arg(vl,  int);
	va_end(vl);

	return send(s, buf, len,  flags);
}


uint32_t user_hook_socket(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);
	/* int socket(int domain, int type, int protocol); */
	int domain = va_arg(vl,  int);
	int type = va_arg(vl,  int);
	int protocol = va_arg(vl, int);
	va_end(vl);

	printf("socket(%i, %i, %i)\n",domain, type, protocol);

	return socket(domain, type, protocol);
}

uint32_t user_hook_WSASocket(struct emu_env *env, struct emu_env_hook *hook, ...)
{
	printf("Hook me Captain Cook!\n");
	printf("%s:%i %s\n",__FILE__,__LINE__,__FUNCTION__);

	va_list vl;
	va_start(vl, hook);
	/* int socket(int domain, int type, int protocol); */
	int domain = va_arg(vl,  int);
	int type = va_arg(vl,  int);
	int protocol = va_arg(vl, int);
	(void)va_arg(vl, int);
	(void)va_arg(vl, int);
	(void)va_arg(vl, int);

	va_end(vl);

	printf("WSASocket(%i, %i, %i)\n",domain, type, protocol);

	return socket(domain, type, protocol);
}
