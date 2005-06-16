
/*
 *  LibGadu dla Win32 jako DLL  
 *
 *  (C)Copyright 2003 Rafa³ Lindemann <stamina@kliper.eu.org>
 *
 *  O autorach poszczególnych czêœci libgadu przeczytasz w pozosta³ych
 *  plikach Ÿród³owych.
 *                    
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License Version
 *  2.1 as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "compat.h"
#include <sys/ioctl.h>
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

#include "libgadu_dll.h"

    int WSAerrno(void) {
        int e = WSAGetLastError();
        // Zamienia kod blêdu WSA na bardziej "kompatybilny"
		if (e == WSAEWOULDBLOCK) {Sleep(100); return 4 /*EINTR*/;}
        if (e>=10001 && e<=10022 && e!=10015/* && e!=10036 && e!=10035*/)
            e-=10000;
        return e;
    }


const char * WSAstrerror(int err) {
#undef strerror
    if (err <= 42) return strerror(err);

    switch (err) {
    case WSAEINTR: return "WSA Interrupted function call";
    case WSAEACCES: return "WSA Permission denied";
    case WSAEFAULT: return "WSA Bad address";
    case WSAEINVAL: return "WSA Invalid argument";
    case WSAEMFILE: return "WSA Too many open files";
    case WSAEWOULDBLOCK: return "WSA Resource temporarily unavailable";
    case WSAEINPROGRESS: return "WSA Operation now in progress";
    case WSAEALREADY: return "WSA Operation already in progress";
    case WSAENOTSOCK: return "WSA Socket operation on nonsocket";
    case WSAEDESTADDRREQ: return "WSA Destination address required";
    case WSAEMSGSIZE: return "WSA Message too long";
    case WSAEADDRINUSE: return "WSA Address already in use";
    case WSAEADDRNOTAVAIL: return "WSA Cannot assign requested address";
    case WSAENETDOWN: return "WSA Network is down";
    case WSAENETUNREACH:return "WSA Network is unreachable";
    case WSAENETRESET: return "WSA Network dropped connection on reset";
    case WSAECONNABORTED: return "WSA Software caused connection abort";
    case WSAECONNRESET: return "WSA Connection reset by peer";
    case WSAENOBUFS: return "WSA No buffer space available"; 
    case WSAEISCONN: return "WSA Socket is already connected";
    case WSAENOTCONN: return "WSA Socket is not connected";
    case WSAESHUTDOWN: return "WSA Cannot send after socket shutdown";
    case WSAETIMEDOUT: return "WSA Connection timed out";
    case WSAECONNREFUSED: return "WSA Connection refused";
    case WSAEHOSTDOWN: return "WSA Host is down";
    case WSAEHOSTUNREACH: return "WSA No route to host";
    case WSAEPROCLIM: return "WSA Too many processes";
    case WSASYSNOTREADY: return "WSA Network subsystem is unavailable";
    case WSANOTINITIALISED: return "WSA Successful WSAStartup not yet performed";
    case WSAEDISCON: return "WSA Graceful shutdown in progress";
    case WSAHOST_NOT_FOUND: return "WSA Host not found";
    case WSANO_RECOVERY: return "WSA This is a nonrecoverable error";
    default: return "WSA Unknown Error";
    }

}


int ioctl(int fd, int request, ...)
{
	va_list ap;
	unsigned long *foo;

	va_start(ap, request);
	foo = va_arg(ap, unsigned long*);
	va_end(ap);
	
	return ioctlsocket(fd, request, foo);

}

