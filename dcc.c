/* $Id: dcc.c,v 1.69 2005/05/22 08:41:54 wojtekka Exp $ */

/*
 *  (C) Copyright 2001-2002 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Tomasz Chili�ski <chilek@chilan.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef sun
#  include <sys/filio.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "compat.h"
#include "libgadu.h"

#ifndef GG_DEBUG_DISABLE
/*
 * gg_dcc_debug_data() // funkcja wewn�trzna
 *
 * wy�wietla zrzut pakietu w hexie.
 * 
 *  - prefix - prefiks zrzutu pakietu
 *  - fd - deskryptor gniazda
 *  - buf - bufor z danymi
 *  - size - rozmiar danych
 */
static void gg_dcc_debug_data(const char *prefix, int fd, const void *buf, unsigned int size)
{
	unsigned int i;
	
	gg_debug(GG_DEBUG_MISC, "++ gg_dcc %s (fd=%d,len=%d)", prefix, fd, size);
	
	for (i = 0; i < size; i++)
		gg_debug(GG_DEBUG_MISC, " %.2x", ((unsigned char*) buf)[i]);
	
	gg_debug(GG_DEBUG_MISC, "\n");
}
#else
#define gg_dcc_debug_data(a,b,c,d) do { } while (0)
#endif

/*
 * gg_dcc_request()
 *
 * wysy�a informacj� o tym, �e dany klient powinien si� z nami po��czy�.
 * wykorzystywane, kiedy druga strona, kt�rej chcemy co� wys�a� jest za
 * maskarad�.
 *
 *  - sess - struktura opisuj�ca sesj� GG
 *  - uin - numerek odbiorcy
 *
 * patrz gg_send_message_ctcp().
 */
int gg_dcc_request(struct gg_session *sess, uin_t uin)
{
	return gg_send_message_ctcp(sess, GG_CLASS_CTCP, uin, "\002", 1);
}

/* 
 * gg_dcc_fill_filetime()  // funkcja wewn�trzna
 *
 * zamienia czas w postaci unixowej na windowsowy.
 *
 *  - unix - czas w postaci unixowej
 *  - filetime - czas w postaci windowsowej
 */
void gg_dcc_fill_filetime(uint32_t ut, uint32_t *ft)
{
#ifdef __GG_LIBGADU_HAVE_LONG_LONG
	unsigned long long tmp;

	tmp = ut;
	tmp += 11644473600LL;
	tmp *= 10000000LL;

#ifndef __GG_LIBGADU_BIGENDIAN
	ft[0] = (uint32_t) tmp;
	ft[1] = (uint32_t) (tmp >> 32);
#else
	ft[0] = gg_fix32((uint32_t) (tmp >> 32));
	ft[1] = gg_fix32((uint32_t) tmp);
#endif

#endif
}

/*
 * gg_dcc_fill_file_info()
 *
 * wype�nia pola struct gg_dcc niezb�dne do wys�ania pliku.
 *
 *  - d - struktura opisuj�ca po��czenie DCC
 *  - filename - nazwa pliku
 *
 * 0, -1.
 */
int gg_dcc_fill_file_info(struct gg_dcc *d, const char *filename)
{
	return gg_dcc_fill_file_info2(d, filename, filename);
}

/*
 * gg_dcc_fill_file_info2()
 *
 * wype�nia pola struct gg_dcc niezb�dne do wys�ania pliku.
 *
 *  - d - struktura opisuj�ca po��czenie DCC
 *  - filename - nazwa pliku
 *  - local_filename - nazwa na lokalnym systemie plik�w
 *
 * 0, -1.
 */
int gg_dcc_fill_file_info2(struct gg_dcc *d, const char *filename, const char *local_filename)
{
	struct stat st;
	const char *name, *ext, *p;
	unsigned char *q;
	int i, j;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_fill_file_info2(%p, \"%s\", \"%s\");\n", d, filename, local_filename);

	if (!d || d->type != GG_SESSION_DCC_SEND) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info2() invalid arguments\n");
		errno = EINVAL;
		return -1;
	}

	if (stat(local_filename, &st) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info2() stat() failed (%s)\n", strerror(errno));
		return -1;
	}

	if ((st.st_mode & S_IFDIR)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info2() that's a directory\n");
		errno = EINVAL;
		return -1;
	}

	if ((d->file_fd = open(local_filename, O_RDONLY)) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info2() open() failed (%s)\n", strerror(errno));
		return -1;
	}

	memset(&d->file_info, 0, sizeof(d->file_info));

	if (!(st.st_mode & S_IWUSR))
		d->file_info.mode |= gg_fix32(GG_DCC_FILEATTR_READONLY);

	gg_dcc_fill_filetime(st.st_atime, d->file_info.atime);
	gg_dcc_fill_filetime(st.st_mtime, d->file_info.mtime);
	gg_dcc_fill_filetime(st.st_ctime, d->file_info.ctime);

	d->file_info.size = gg_fix32(st.st_size);
	d->file_info.mode = gg_fix32(0x20);	/* FILE_ATTRIBUTE_ARCHIVE */

	if (!(name = strrchr(filename, '/')))
		name = filename;
	else
		name++;

	if (!(ext = strrchr(name, '.')))
		ext = name + strlen(name);

	for (i = 0, p = name; i < 8 && p < ext; i++, p++)
		d->file_info.short_filename[i] = toupper(name[i]);

	if (i == 8 && p < ext) {
		d->file_info.short_filename[6] = '~';
		d->file_info.short_filename[7] = '1';
	}

	if (strlen(ext) > 0) {
		for (j = 0; *ext && j < 4; j++, p++)
			d->file_info.short_filename[i + j] = toupper(ext[j]);
	}

	for (q = d->file_info.short_filename; *q; q++) {
		if (*q == 185) {
			*q = 165;
		} else if (*q == 230) {
			*q = 198;
		} else if (*q == 234) {
			*q = 202;
		} else if (*q == 179) {
			*q = 163;
		} else if (*q == 241) {
			*q = 209;
		} else if (*q == 243) {
			*q = 211;
		} else if (*q == 156) {
			*q = 140;
		} else if (*q == 159) {
			*q = 143;
		} else if (*q == 191) {
			*q = 175;
		}
	}
	
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info2() short name \"%s\", dos name \"%s\"\n", name, d->file_info.short_filename);
	strncpy(d->file_info.filename, name, sizeof(d->file_info.filename) - 1);

	return 0;
}

/*
 * gg_dcc_transfer() // funkcja wewn�trzna
 * 
 * inicjuje proces wymiany pliku z danym klientem.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w�asny numer
 *  - peer_uin - numer obiorcy
 *  - type - rodzaj wymiany (GG_SESSION_DCC_SEND lub GG_SESSION_DCC_GET)
 *
 * zaalokowana struct gg_dcc lub NULL je�li wyst�pi� b��d.
 */
static struct gg_dcc *gg_dcc_transfer(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin, int type)
{
	struct gg_dcc *d = NULL;
	struct in_addr addr;

	addr.s_addr = ip;
	
	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_transfer(%s, %d, %ld, %ld, %s);\n", inet_ntoa(addr), port, my_uin, peer_uin, (type == GG_SESSION_DCC_SEND) ? "SEND" : "GET");
	
	if (!ip || ip == INADDR_NONE || !port || !my_uin || !peer_uin) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_transfer() invalid arguments\n");
		errno = EINVAL;
		return NULL;
	}

	if (!(d = (void*) calloc(1, sizeof(*d)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_transfer() not enough memory\n");
		return NULL;
	}

	d->check = GG_CHECK_WRITE;
	d->state = GG_STATE_CONNECTING;
	d->type = type;
	d->timeout = GG_DEFAULT_TIMEOUT;
	d->file_fd = -1;
	d->active = 1;
	d->fd = -1;
	d->uin = my_uin;
	d->peer_uin = peer_uin;

	if ((d->fd = gg_connect(&addr, port, 1)) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_transfer() connection failed\n");
		free(d);
		return NULL;
	}

	return d;
}

/*
 * gg_dcc_get_file()
 * 
 * inicjuje proces odbierania pliku od danego klienta, gdy ten wys�a� do
 * nas ��danie po��czenia.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w�asny numer
 *  - peer_uin - numer obiorcy
 *
 * zaalokowana struct gg_dcc lub NULL je�li wyst�pi� b��d.
 */
struct gg_dcc *gg_dcc_get_file(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin)
{
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_get_file() handing over to gg_dcc_transfer()\n");

	return gg_dcc_transfer(ip, port, my_uin, peer_uin, GG_SESSION_DCC_GET);
}

/*
 * gg_dcc_send_file()
 * 
 * inicjuje proces wysy�ania pliku do danego klienta.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w�asny numer
 *  - peer_uin - numer obiorcy
 *
 * zaalokowana struct gg_dcc lub NULL je�li wyst�pi� b��d.
 */
struct gg_dcc *gg_dcc_send_file(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin)
{
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_send_file() handing over to gg_dcc_transfer()\n");

	return gg_dcc_transfer(ip, port, my_uin, peer_uin, GG_SESSION_DCC_SEND);
}

/*
 * gg_dcc_voice_chat()
 * 
 * pr�buje nawi�za� po��czenie g�osowe.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w�asny numer
 *  - peer_uin - numer obiorcy
 *
 * zaalokowana struct gg_dcc lub NULL je�li wyst�pi� b��d.
 */
struct gg_dcc *gg_dcc_voice_chat(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin)
{
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_chat() handing over to gg_dcc_transfer()\n");

	return gg_dcc_transfer(ip, port, my_uin, peer_uin, GG_SESSION_DCC_VOICE);
}

/*
 * gg_dcc_set_type()
 *
 * po zdarzeniu GG_EVENT_DCC_CALLBACK nale�y ustawi� typ po��czenia za
 * pomoc� tej funkcji.
 *
 *  - d - struktura opisuj�ca po��czenie
 *  - type - typ po��czenia (GG_SESSION_DCC_SEND lub GG_SESSION_DCC_VOICE)
 */
void gg_dcc_set_type(struct gg_dcc *d, int type)
{
	d->type = type;
	d->state = (type == GG_SESSION_DCC_SEND) ? GG_STATE_SENDING_FILE_INFO : GG_STATE_SENDING_VOICE_REQUEST;
}
	
/*
 * gg_dcc_callback() // funkcja wewn�trzna
 *
 * wywo�ywana z struct gg_dcc->callback, odpala gg_dcc_watch_fd i umieszcza
 * rezultat w struct gg_dcc->event.
 *
 *  - d - structura opisuj�ca po��czenie
 *
 * 0, -1.
 */
static int gg_dcc_callback(struct gg_dcc *d)
{
	struct gg_event *e = gg_dcc_watch_fd(d);

	d->event = e;

	return (e != NULL) ? 0 : -1;
}

/*
 * gg_dcc_socket_create()
 *
 * tworzy gniazdo dla bezpo�redniej komunikacji mi�dzy klientami.
 *
 *  - uin - w�asny numer
 *  - port - preferowany port, je�li r�wny 0 lub -1, pr�buje domy�lnego
 *
 * zaalokowana struct gg_dcc, kt�r� po�niej nale�y zwolni� funkcj�
 * gg_dcc_free(), albo NULL je�li wyst�pi� b��d.
 */
struct gg_dcc *gg_dcc_socket_create(uin_t uin, uint16_t port)
{
	struct gg_dcc *c;
	struct sockaddr_in sin;
	int sock, bound = 0, errno2;
	
	gg_debug(GG_DEBUG_FUNCTION, "** gg_create_dcc_socket(%d, %d);\n", uin, port);
	
	if (!uin) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() invalid arguments\n");
		errno = EINVAL;
		return NULL;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() can't create socket (%s)\n", strerror(errno));
		return NULL;
	}

	if (!port)
		port = GG_DEFAULT_DCC_PORT;
	
	while (!bound) {
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
	
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() trying port %d\n", port);
		if (!bind(sock, (struct sockaddr*) &sin, sizeof(sin)))
			bound = 1;
		else {
			if (++port == 65535) {
				gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() no free port found\n");
				close(sock);
				return NULL;
			}
		}
	}

	if (listen(sock, 10)) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() unable to listen (%s)\n", strerror(errno));
		errno2 = errno;
		close(sock);
		errno = errno2;
		return NULL;
	}
	
	gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() bound to port %d\n", port);

	if (!(c = malloc(sizeof(*c)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() not enough memory for struct\n");
		close(sock);
		return NULL;
	}
	memset(c, 0, sizeof(*c));

	c->port = c->id = port;
	c->fd = sock;
	c->type = GG_SESSION_DCC_SOCKET;
	c->uin = uin;
	c->timeout = -1;
	c->state = GG_STATE_LISTENING;
	c->check = GG_CHECK_READ;
	c->callback = gg_dcc_callback;
	c->destroy = gg_dcc_free;
	
	return c;
}

/*
 * gg_dcc_voice_send()
 *
 * wysy�a ramk� danych dla rozmowy g�osowej.
 *
 *  - d - struktura opisuj�ca po��czenie dcc
 *  - buf - bufor z danymi
 *  - length - rozmiar ramki
 *
 * 0, -1.
 */
int gg_dcc_voice_send(struct gg_dcc *d, char *buf, int length)
{
	struct packet_s {
		uint8_t type;
		uint32_t length;
	} GG_PACKED;
	struct packet_s packet;

	gg_debug(GG_DEBUG_FUNCTION, "++ gg_dcc_voice_send(%p, %p, %d);\n", d, buf, length);
	if (!d || !buf || length < 0 || d->type != GG_SESSION_DCC_VOICE) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_send() invalid argument\n");
		errno = EINVAL;
		return -1;
	}

	packet.type = 0x03; /* XXX */
	packet.length = gg_fix32(length);

	if (write(d->fd, &packet, sizeof(packet)) < (signed)sizeof(packet)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_send() write() failed\n");
		return -1;
	}
	gg_dcc_debug_data("write", d->fd, &packet, sizeof(packet));

	if (write(d->fd, buf, length) < length) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_send() write() failed\n");
		return -1;
	}
	gg_dcc_debug_data("write", d->fd, buf, length);

	return 0;
}

#define gg_read(fd, buf, size) \
{ \
	int tmp = read(fd, buf, size); \
	\
	if (tmp < (int) size) { \
		if (tmp == -1) { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed (errno=%d, %s)\n", errno, strerror(errno)); \
		} else if (tmp == 0) { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed, connection broken\n"); \
		} else { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed (%d bytes, %d needed)\n", tmp, size); \
		} \
		e->type = GG_EVENT_DCC_ERROR; \
		e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE; \
		return e; \
	} \
	gg_dcc_debug_data("read", fd, buf, size); \
} 

#define gg_write(fd, buf, size) \
{ \
	int tmp; \
	gg_dcc_debug_data("write", fd, buf, size); \
	tmp = write(fd, buf, size); \
	if (tmp < (int) size) { \
		if (tmp == -1) { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (errno=%d, %s)\n", errno, strerror(errno)); \
		} else { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (%d needed, %d done)\n", size, tmp); \
		} \
		e->type = GG_EVENT_DCC_ERROR; \
		e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE; \
		return e; \
	} \
}

/*
 * gg_dcc_watch_fd()
 *
 * funkcja, kt�r� nale�y wywo�a�, gdy co� si� zmieni na gg_dcc->fd.
 *
 *  - h - struktura zwr�cona przez gg_create_dcc_socket()
 *
 * zaalokowana struct gg_event lub NULL, je�li zabrak�o pami�ci na ni�.
 */
struct gg_event *gg_dcc_watch_fd(struct gg_dcc *h)
{
	struct gg_event *e;
	int foo;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_watch_fd(%p);\n", h);
	
	if (!h || (h->type != GG_SESSION_DCC && h->type != GG_SESSION_DCC_SOCKET && h->type != GG_SESSION_DCC_SEND && h->type != GG_SESSION_DCC_GET && h->type != GG_SESSION_DCC_VOICE)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() invalid argument\n");
		errno = EINVAL;
		return NULL;
	}

	if (!(e = (void*) calloc(1, sizeof(*e)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() not enough memory\n");
		return NULL;
	}

	e->type = GG_EVENT_NONE;

	if (h->type == GG_SESSION_DCC_SOCKET) {
		struct sockaddr_in sin;
		struct gg_dcc *c;
		int fd, sin_len = sizeof(sin), one = 1;
		
		if ((fd = accept(h->fd, (struct sockaddr*) &sin, &sin_len)) == -1) {
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() can't accept() new connection (errno=%d, %s)\n", errno, strerror(errno));
			return e;
		}

		gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() new direct connection from %s:%d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

#ifdef FIONBIO
		if (ioctl(fd, FIONBIO, &one) == -1) {
#else
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
#endif
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() can't set nonblocking (errno=%d, %s)\n", errno, strerror(errno));
			close(fd);
			e->type = GG_EVENT_DCC_ERROR;
			e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
			return e;
		}

		if (!(c = (void*) calloc(1, sizeof(*c)))) {
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() not enough memory for client data\n");

			free(e);
			close(fd);
			return NULL;
		}

		c->fd = fd;
		c->check = GG_CHECK_READ;
		c->state = GG_STATE_READING_UIN_1;
		c->type = GG_SESSION_DCC;
		c->timeout = GG_DEFAULT_TIMEOUT;
		c->file_fd = -1;
		c->remote_addr = sin.sin_addr.s_addr;
		c->remote_port = ntohs(sin.sin_port);
		
		e->type = GG_EVENT_DCC_NEW;
		e->event.dcc_new = c;

		return e;
	} else {
		struct gg_dcc_tiny_packet tiny;
		struct gg_dcc_small_packet small;
		struct gg_dcc_big_packet big;
		int size, tmp, res, res_size = sizeof(res);
		unsigned int utmp;
		char buf[1024], ack[] = "UDAG";

		struct gg_dcc_file_info_packet {
			struct gg_dcc_big_packet big;
			struct gg_file_info file_info;
		} GG_PACKED;
		struct gg_dcc_file_info_packet file_info_packet;

		switch (h->state) {
			case GG_STATE_READING_UIN_1:
			case GG_STATE_READING_UIN_2:
			{
				uin_t uin;

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_READING_UIN_%d\n", (h->state == GG_STATE_READING_UIN_1) ? 1 : 2);
				
				gg_read(h->fd, &uin, sizeof(uin));

				if (h->state == GG_STATE_READING_UIN_1) {
					h->state = GG_STATE_READING_UIN_2;
					h->check = GG_CHECK_READ;
					h->timeout = GG_DEFAULT_TIMEOUT;
					h->peer_uin = gg_fix32(uin);
				} else {
					h->state = GG_STATE_SENDING_ACK;
					h->check = GG_CHECK_WRITE;
					h->timeout = GG_DEFAULT_TIMEOUT;
					h->uin = gg_fix32(uin);
					e->type = GG_EVENT_DCC_CLIENT_ACCEPT;
				}

				return e;
			}

			case GG_STATE_SENDING_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_SENDING_ACK\n");

				gg_write(h->fd, ack, 4);

				h->state = GG_STATE_READING_TYPE;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				return e;

			case GG_STATE_READING_TYPE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_TYPE\n");
				
				gg_read(h->fd, &small, sizeof(small));

				small.type = gg_fix32(small.type);

				switch (small.type) {
					case 0x0003:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() callback\n");
						h->type = GG_SESSION_DCC_SEND;
						h->state = GG_STATE_SENDING_FILE_INFO;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DEFAULT_TIMEOUT;

						e->type = GG_EVENT_DCC_CALLBACK;
			
						break;

					case 0x0002:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() dialin\n");
						h->type = GG_SESSION_DCC_GET;
						h->state = GG_STATE_READING_REQUEST;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						h->incoming = 1;

						break;

					default:
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() unknown dcc type (%.4x) from %ld\n", small.type, h->peer_uin);
						e->type = GG_EVENT_DCC_ERROR;
						e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
				}

				return e;

			case GG_STATE_READING_REQUEST:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_REQUEST\n");
				
				gg_read(h->fd, &small, sizeof(small));

				small.type = gg_fix32(small.type);

				switch (small.type) {
					case 0x0001:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() file transfer request\n");
						h->state = GG_STATE_READING_FILE_INFO;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						break;
						
					case 0x0003:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() voice chat request\n");
						h->state = GG_STATE_SENDING_VOICE_ACK;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DCC_TIMEOUT_VOICE_ACK;
						h->type = GG_SESSION_DCC_VOICE;
						e->type = GG_EVENT_DCC_NEED_VOICE_ACK;

						break;
						
					default:
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() unknown dcc request (%.4x) from %ld\n", small.type, h->peer_uin);
						e->type = GG_EVENT_DCC_ERROR;
						e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
				}
		 	
				return e;

			case GG_STATE_READING_FILE_INFO:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_FILE_INFO\n");
				
				gg_read(h->fd, &file_info_packet, sizeof(file_info_packet));

				memcpy(&h->file_info, &file_info_packet.file_info, sizeof(h->file_info));
		
				h->file_info.mode = gg_fix32(h->file_info.mode);
				h->file_info.size = gg_fix32(h->file_info.size);

				h->state = GG_STATE_SENDING_FILE_ACK;
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DCC_TIMEOUT_FILE_ACK;

				e->type = GG_EVENT_DCC_NEED_FILE_ACK;
				
				return e;

			case GG_STATE_SENDING_FILE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE_ACK\n");
				
				big.type = gg_fix32(0x0006);	/* XXX */
				big.dunno1 = gg_fix32(h->offset);
				big.dunno2 = 0;

				gg_write(h->fd, &big, sizeof(big));

				h->state = GG_STATE_READING_FILE_HEADER;
				h->chunk_size = sizeof(big);
				h->chunk_offset = 0;
				if (!(h->chunk_buf = malloc(sizeof(big)))) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() out of memory\n");
					free(e);
					return NULL;
				}
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				return e;
				
			case GG_STATE_SENDING_VOICE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_VOICE_ACK\n");
				
				tiny.type = 0x01;	/* XXX */

				gg_write(h->fd, &tiny, sizeof(tiny));

				h->state = GG_STATE_READING_VOICE_HEADER;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				h->offset = 0;
				
				return e;
				
			case GG_STATE_READING_FILE_HEADER:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_FILE_HEADER\n");
				
				tmp = read(h->fd, h->chunk_buf + h->chunk_offset, h->chunk_size - h->chunk_offset);

				if (tmp == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_watch_fd() read() failed (errno=%d, %s)\n", errno, strerror(errno));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				gg_dcc_debug_data("read", h->fd, h->chunk_buf + h->chunk_offset, h->chunk_size - h->chunk_offset);
				
				h->chunk_offset += tmp;

				if (h->chunk_offset < h->chunk_size)
					return e;

				memcpy(&big, h->chunk_buf, sizeof(big));
				free(h->chunk_buf);
				h->chunk_buf = NULL;

				big.type = gg_fix32(big.type);
				h->chunk_size = gg_fix32(big.dunno1);
				h->chunk_offset = 0;

				if (big.type == 0x0005)	{ /* XXX */
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() transfer refused\n");
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_REFUSED;
					return e;
				}

				if (h->chunk_size == 0) { 
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() empty chunk, EOF\n");
					e->type = GG_EVENT_DCC_DONE;
					return e;
				}

				h->state = GG_STATE_GETTING_FILE;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				h->established = 1;
			 	
				return e;

			case GG_STATE_READING_VOICE_HEADER:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_HEADER\n");
				
				gg_read(h->fd, &tiny, sizeof(tiny));

				switch (tiny.type) {
					case 0x03:	/* XXX */
						h->state = GG_STATE_READING_VOICE_SIZE;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						h->established = 1;
						break;
					case 0x04:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() peer breaking connection\n");
						/* XXX zwraca� odpowiedni event */
					default:
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() unknown request (%.2x)\n", tiny.type);
						e->type = GG_EVENT_DCC_ERROR;
						e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
				}
			 	
				return e;

			case GG_STATE_READING_VOICE_SIZE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_SIZE\n");
				
				gg_read(h->fd, &small, sizeof(small));

				small.type = gg_fix32(small.type);

				if (small.type < 16 || small.type > sizeof(buf)) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() invalid voice frame size (%d)\n", small.type);
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					
					return e;
				}

				h->chunk_size = small.type;
				h->chunk_offset = 0;

				if (!(h->voice_buf = malloc(h->chunk_size))) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() out of memory for voice frame\n");
					return NULL;
				}

				h->state = GG_STATE_READING_VOICE_DATA;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
			 	
				return e;

			case GG_STATE_READING_VOICE_DATA:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_DATA\n");
				
				tmp = read(h->fd, h->voice_buf + h->chunk_offset, h->chunk_size - h->chunk_offset);
				if (tmp < 1) {
					if (tmp == -1) {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed (errno=%d, %s)\n", errno, strerror(errno));
					} else {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed, connection broken\n");
					}
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				gg_dcc_debug_data("read", h->fd, h->voice_buf + h->chunk_offset, tmp);

				h->chunk_offset += tmp;

				if (h->chunk_offset >= h->chunk_size) {
					e->type = GG_EVENT_DCC_VOICE_DATA;
					e->event.dcc_voice_data.data = h->voice_buf;
					e->event.dcc_voice_data.length = h->chunk_size;
					h->state = GG_STATE_READING_VOICE_HEADER;
					h->voice_buf = NULL;
				}

				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				
				return e;

			case GG_STATE_CONNECTING:
			{
				uin_t uins[2];

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_CONNECTING\n");
				
				res = 0;
				if ((foo = getsockopt(h->fd, SOL_SOCKET, SO_ERROR, &res, &res_size)) || res) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() connection failed (fd=%d,errno=%d(%s),foo=%d,res=%d(%s))\n", h->fd, errno, strerror(errno), foo, res, strerror(res));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
					return e;
				}

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() connected, sending uins\n");
				
				uins[0] = gg_fix32(h->uin);
				uins[1] = gg_fix32(h->peer_uin);

				gg_write(h->fd, uins, sizeof(uins));
				
				h->state = GG_STATE_READING_ACK;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				
				return e;
			}

			case GG_STATE_READING_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_ACK\n");
				
				gg_read(h->fd, buf, 4);

				if (strncmp(buf, ack, 4)) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() did't get ack\n");

					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
					return e;
				}

				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DEFAULT_TIMEOUT;
				h->state = GG_STATE_SENDING_REQUEST;
				
				return e;

			case GG_STATE_SENDING_VOICE_REQUEST:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_VOICE_REQUEST\n");

				small.type = gg_fix32(0x0003);
				
				gg_write(h->fd, &small, sizeof(small));

				h->state = GG_STATE_READING_VOICE_ACK;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				
				return e;
			
			case GG_STATE_SENDING_REQUEST:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_REQUEST\n");

				small.type = (h->type == GG_SESSION_DCC_GET) ? gg_fix32(0x0003) : gg_fix32(0x0002);	/* XXX */
				
				gg_write(h->fd, &small, sizeof(small));
				
				switch (h->type) {
					case GG_SESSION_DCC_GET:
						h->state = GG_STATE_READING_REQUEST;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						break;

					case GG_SESSION_DCC_SEND:
						h->state = GG_STATE_SENDING_FILE_INFO;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DEFAULT_TIMEOUT;

						if (h->file_fd == -1)
							e->type = GG_EVENT_DCC_NEED_FILE_INFO;
						break;
						
					case GG_SESSION_DCC_VOICE:
						h->state = GG_STATE_SENDING_VOICE_REQUEST;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DEFAULT_TIMEOUT;
						break;
				}

				return e;
			
			case GG_STATE_SENDING_FILE_INFO:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE_INFO\n");

				if (h->file_fd == -1) {
					e->type = GG_EVENT_DCC_NEED_FILE_INFO;
					return e;
				}

				small.type = gg_fix32(0x0001);	/* XXX */
				
				gg_write(h->fd, &small, sizeof(small));

				file_info_packet.big.type = gg_fix32(0x0003);	/* XXX */
				file_info_packet.big.dunno1 = 0;
				file_info_packet.big.dunno2 = 0;

				memcpy(&file_info_packet.file_info, &h->file_info, sizeof(h->file_info));

				/* zostaj� teraz u nas, wi�c odwracamy z powrotem */
				h->file_info.size = gg_fix32(h->file_info.size);
				h->file_info.mode = gg_fix32(h->file_info.mode);
				
				gg_write(h->fd, &file_info_packet, sizeof(file_info_packet));

				h->state = GG_STATE_READING_FILE_ACK;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DCC_TIMEOUT_FILE_ACK;

				return e;
				
			case GG_STATE_READING_FILE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_FILE_ACK\n");
				
				gg_read(h->fd, &big, sizeof(big));

				/* XXX sprawdza� wynik */
				h->offset = gg_fix32(big.dunno1);
				
				h->state = GG_STATE_SENDING_FILE_HEADER;
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DEFAULT_TIMEOUT;

				e->type = GG_EVENT_DCC_ACK;

				return e;

			case GG_STATE_READING_VOICE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_ACK\n");
				
				gg_read(h->fd, &tiny, sizeof(tiny));

				if (tiny.type != 0x01) {
					gg_debug(GG_DEBUG_MISC, "// invalid reply (%.2x), connection refused\n", tiny.type);
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_REFUSED;
					return e;
				}

				h->state = GG_STATE_READING_VOICE_HEADER;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				e->type = GG_EVENT_DCC_ACK;
				
				return e;

			case GG_STATE_SENDING_FILE_HEADER:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE_HEADER\n");
				
				h->chunk_offset = 0;
				
				if ((h->chunk_size = h->file_info.size - h->offset) > 4096) {
					h->chunk_size = 4096;
					big.type = gg_fix32(0x0003);  /* XXX */
				} else
					big.type = gg_fix32(0x0002);  /* XXX */

				big.dunno1 = gg_fix32(h->chunk_size);
				big.dunno2 = 0;
				
				gg_write(h->fd, &big, sizeof(big));

				h->state = GG_STATE_SENDING_FILE;
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DEFAULT_TIMEOUT;
				h->established = 1;

				return e;
				
			case GG_STATE_SENDING_FILE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE\n");
				
				if ((utmp = h->chunk_size - h->chunk_offset) > sizeof(buf))
					utmp = sizeof(buf);
				
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() offset=%d, size=%d\n", h->offset, h->file_info.size);

				/* koniec pliku? */
				if (h->file_info.size == 0) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() reached eof on empty file\n");
					e->type = GG_EVENT_DCC_DONE;

					return e;
				}

				lseek(h->file_fd, h->offset, SEEK_SET);

				size = read(h->file_fd, buf, utmp);

				/* b��d */
				if (size == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed. (errno=%d, %s)\n", errno, strerror(errno));

					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_FILE;

					return e;
				}

				/* koniec pliku? */
				if (size == 0) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() reached eof\n");
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_EOF;

					return e;
				}
				
				/* je�li wczytali�my wi�cej, utnijmy. */
				if (h->offset + size > h->file_info.size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() too much (read=%d, ofs=%d, size=%d)\n", size, h->offset, h->file_info.size);
					size = h->file_info.size - h->offset;

					if (size < 1) {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() reached EOF after cutting\n");
						e->type = GG_EVENT_DCC_DONE;
						return e;
					}
				}

				tmp = write(h->fd, buf, size);

				if (tmp == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (%s)\n", strerror(errno));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				h->offset += size;
				
				if (h->offset >= h->file_info.size) {
					e->type = GG_EVENT_DCC_DONE;
					return e;
				}
				
				h->chunk_offset += size;
				
				if (h->chunk_offset >= h->chunk_size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() chunk finished\n");
					h->state = GG_STATE_SENDING_FILE_HEADER;
					h->timeout = GG_DEFAULT_TIMEOUT;
				} else {
					h->state = GG_STATE_SENDING_FILE;
					h->timeout = GG_DCC_TIMEOUT_SEND;
				}
				
				h->check = GG_CHECK_WRITE;

				return e;

			case GG_STATE_GETTING_FILE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_GETTING_FILE\n");
				
				if ((utmp = h->chunk_size - h->chunk_offset) > sizeof(buf))
					utmp = sizeof(buf);
				
				size = read(h->fd, buf, utmp);

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() ofs=%d, size=%d, read()=%d\n", h->offset, h->file_info.size, size);
				
				/* b��d */
				if (size == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed. (errno=%d, %s)\n", errno, strerror(errno));

					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;

					return e;
				}

				/* koniec? */
				if (size == 0) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() reached eof\n");
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_EOF;

					return e;
				}
				
				tmp = write(h->file_fd, buf, size);
				
				if (tmp == -1 || tmp < size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (%d:fd=%d:res=%d:%s)\n", tmp, h->file_fd, size, strerror(errno));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				h->offset += size;
				
				if (h->offset >= h->file_info.size) {
					e->type = GG_EVENT_DCC_DONE;
					return e;
				}

				h->chunk_offset += size;
				
				if (h->chunk_offset >= h->chunk_size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() chunk finished\n");
					h->state = GG_STATE_READING_FILE_HEADER;
					h->timeout = GG_DEFAULT_TIMEOUT;
					h->chunk_offset = 0;
					h->chunk_size = sizeof(big);
					if (!(h->chunk_buf = malloc(sizeof(big)))) {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() out of memory\n");
						free(e);
						return NULL;
					}
				} else {
					h->state = GG_STATE_GETTING_FILE;
					h->timeout = GG_DCC_TIMEOUT_GET;
				}
				
				h->check = GG_CHECK_READ;

				return e;
				
			default:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_???\n");
				e->type = GG_EVENT_DCC_ERROR;
				e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;

				return e;
		}
	}
	
	return e;
}

#undef gg_read
#undef gg_write

/*
 * gg_dcc_free()
 *
 * zwalnia pami�� po strukturze po��czenia dcc.
 *
 *  - d - zwalniana struktura
 */
void gg_dcc_free(struct gg_dcc *d)
{
	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_free(%p);\n", d);
	
	if (!d)
		return;

	if (d->fd != -1)
		close(d->fd);

	if (d->chunk_buf) {
		free(d->chunk_buf);
		d->chunk_buf = NULL;
	}

	free(d);
}

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: notnil
 * End:
 *
 * vim: shiftwidth=8:
 */
