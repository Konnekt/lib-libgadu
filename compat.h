/* $Id: compat.h,v 1.3 2004/10/29 18:20:41 wojtekka Exp $ */

/*
 *  (C) Copyright 2001-2002 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Robert J. Wo¼ny <speedy@ziew.org>
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#ifndef __COMPAT_H
#define __COMPAT_H

#ifdef sun
#  define INADDR_NONE   ((in_addr_t) 0xffffffff)
#endif

#ifdef _WIN32
    /*RL*/
    /* Wylaczylem cale gg_resolve - nie mozna wiec uzywac
    polaczen asynchronicznych...
    Na windowsie z reszta, najlepiej zrobic osobny watek
    i w nim pracowac na synchronicznie.
        */
    //#  include <winsock2.h>
    #include <sys/socket.h>

    #pragma comment(lib , "Ws2_32.lib")

    #define strcasecmp _stricmp
    #define snprintf _snprintf

    #define ASSIGN_SOCKETS_TO_THREADS /* gg_connect bedzie zapisywal nr socketa na liscie,
                                        tak zeby mozna go bylo zamknac w polaczeniach synchronicznych (z innego watku) */
    /* Chociaz teoretycznie powinno dzialac na write/read - to nie dziala ...
    Borland na szczescie pozwala na taka "podmiane" funkcji , nie wiem jak inne */
    #  define write(handle,buf,len) send(handle , (const char*)buf , len , 0)
    #  define read(handle,buf,len) recv(handle , (char*)buf , len , 0)
    #ifdef ASSIGN_SOCKETS_TO_THREADS
        #define close(handle) gg_thread_socket(-1,handle)
    #else
        #define close(handle) closesocket(handle)
    #endif
    #  define socket(af , type , protocol) WSASocket(af , type , protocol , 0 , 0 , WSA_FLAG_OVERLAPPED)

	#define ECONNRESET 54


// Na Win32 f-cje socketowe nie ustawiaj¹ errno. raz wywo³any s_errno ustawi numer b³êdu
    #define s_errno (errno=WSAerrno())
// Podmieniamy caly strerror na wersje, ktora potrafi drukowac komunikaty z WSA
    #define strerror(e) WSAstrerror(e)
#else // WIN32
    #define s_errno errno
#endif    // WIN32

#endif
