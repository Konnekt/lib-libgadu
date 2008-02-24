#ifndef __COMPAT_W32_H
#define __COMPAT_W32_H

#ifdef _WIN32

    /* Wylaczylem cale gg_resolve - nie mozna wiec uzywac
    polaczen asynchronicznych...
    Na windowsie z reszta, najlepiej zrobic osobny watek
    i w nim pracowac na synchronicznie.
        */
    //#  include <winsock2.h>
    #include <sys/socket.h>

    #pragma comment(lib , "Ws2_32.lib")

    #define strcasecmp _stricmp
	#define strncasecmp strncmp
    #define snprintf _snprintf
	#define gg_thread_socket gg_win32_thread_socket

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


// Na Win32 f-cje socketowe nie ustawiaj¹ errno. raz wywo³any s_errno ustawi numer b³êdu
    #define s_errno (errno=WSAerrno())
// Podmieniamy caly strerror na wersje, ktora potrafi drukowac komunikaty z WSA
    #define strerror(e) WSAstrerror(e)
#else // WIN32
    #define s_errno errno
#endif    // WIN32

#endif
