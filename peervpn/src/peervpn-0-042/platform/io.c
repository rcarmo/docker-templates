/***************************************************************************
 *   Copyright (C) 2015 by Tobias Volk                                     *
 *   mail@tobiasvolk.de                                                    *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/


#ifndef F_IO_C
#define F_IO_C


#if defined(__FreeBSD__)
#define IO_BSD
#elif defined(__APPLE__)
#define IO_BSD
#elif defined(WIN32)
#define IO_WINDOWS
#ifdef WINVER
#if WINVER < 0x0501
#undef WINVER
#endif
#endif
#ifndef WINVER
#define WINVER 0x0501
#endif
#else
#define IO_LINUX
#endif


#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#if defined(IO_LINUX) || defined(IO_BSD)
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#endif

#if defined(IO_LINUX)
#include <linux/if_tun.h>
#endif

#if defined(IO_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winioctl.h>
#endif


#define IO_TYPE_NULL 0
#define IO_TYPE_SOCKET_V6 1
#define IO_TYPE_SOCKET_V4 2
#define IO_TYPE_FILE 3

#define IO_ADDRTYPE_NULL "\x00\x00\x00\x00"
#define IO_ADDRTYPE_UDP6 "\x01\x06\x01\x00"
#define IO_ADDRTYPE_UDP4 "\x01\x04\x01\x00"



// The IO addr structure.
struct s_io_addr {
	unsigned char addr[24];
};


// The IO addrinfo structure.
struct s_io_addrinfo {
	struct s_io_addr item[16];
	int count;
};


// The IO handle structure.
struct s_io_handle {
	int enabled;
	int fd;
	struct sockaddr_storage source_sockaddr;
	struct s_io_addr source_addr;
	int group_id;
	int content_len;
	int type;
	int open;
#if defined(IO_WINDOWS)
	HANDLE fd_h;
	int open_h;
	OVERLAPPED ovlr;
	int ovlr_used;
	OVERLAPPED ovlw;
	int ovlw_used;
#endif
};


// The IO state structure.
struct s_io_state {
	unsigned char *mem;
	struct s_io_handle *handle;
	int bufsize;
	int max;
	int count;
	int timeout;
	int sockmark;
	int nat64clat;
	unsigned char nat64_prefix[12];
	int debug;
};


// Returns length of string.
static int ioStrlen(const char *str, const int max_len) {
	int len;
	len = 0;
	if(str != NULL) {
		while(len < max_len && str[len] != '\0') {
			len++;
		}
	}
	return len;
}


// Resolve name. Returns number of addresses.
static int ioResolveName(struct s_io_addrinfo *iai, const char *hostname, const char *port) {
	int ret;
	struct s_io_addr *iaiaddr;
	struct sockaddr_in6 *saddr6;
	struct sockaddr_in *saddr4;
	struct addrinfo *d;
	struct addrinfo *di;
	struct addrinfo hints;

	ret = 0;
	if(hostname != NULL && port != NULL) {
		memset(&hints,0,sizeof(struct addrinfo));

		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = 0;
		d = NULL;
		if(getaddrinfo(hostname, port, &hints, &d) == 0) {
			di = d;
			while(di != NULL && ret < 12) {
				saddr6 = (struct sockaddr_in6 *)di->ai_addr;
				iaiaddr = &iai->item[ret];
				memcpy(&iaiaddr->addr[0], IO_ADDRTYPE_UDP6, 4); // set address type
				memcpy(&iaiaddr->addr[4], saddr6->sin6_addr.s6_addr, 16); // copy IPv6 address
				memcpy(&iaiaddr->addr[20], &saddr6->sin6_port, 2); // copy port
				memcpy(&iaiaddr->addr[22], "\x00\x00", 2); // empty bytes
				di = di->ai_next;
				ret++;
			}
			freeaddrinfo(d);
		}

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = 0;
		d = NULL;
		if(getaddrinfo(hostname, port, &hints, &d) == 0) {
			di = d;
			while(di != NULL && ret < 16) {
				saddr4 = (struct sockaddr_in *)di->ai_addr;
				iaiaddr = &iai->item[ret];
				memcpy(&iaiaddr->addr[0], IO_ADDRTYPE_UDP4, 4); // set address type
				memcpy(&iaiaddr->addr[4], &saddr4->sin_addr.s_addr, 4); // copy IPv4 address
				memcpy(&iaiaddr->addr[8], &saddr4->sin_port, 2); // copy port
				memcpy(&iaiaddr->addr[10], "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 14); // empty bytes
				di = di->ai_next;
				ret++;
			}
			freeaddrinfo(d);
		}

	}
	iai->count = ret;
	return ret;
}


// Reset handle ID values and buffers.
static void ioResetID(struct s_io_state *iostate, const int id) {
	iostate->handle[id].enabled = 0;
	iostate->handle[id].content_len = 0;
	iostate->handle[id].fd = -1;
	iostate->handle[id].type = IO_TYPE_NULL;
	iostate->handle[id].group_id = 0;
	iostate->handle[id].open = 0;
	memset(&iostate->handle[id].source_addr, 0, sizeof(struct s_io_addr));
	memset(&iostate->handle[id].source_sockaddr, 0, sizeof(struct sockaddr_storage));
	memset(&iostate->mem[id * iostate->bufsize], 0, iostate->bufsize);
#if defined(IO_WINDOWS)
	memset(&iostate->handle[id].fd_h, 0, sizeof(HANDLE));
	iostate->handle[id].open_h = 0;
	memset(&iostate->handle[id].ovlr, 0, sizeof(OVERLAPPED));
	iostate->handle[id].ovlr.hEvent = NULL;
	iostate->handle[id].ovlr_used = 0;
	memset(&iostate->handle[id].ovlw, 0, sizeof(OVERLAPPED));
	iostate->handle[id].ovlw.hEvent = NULL;
	iostate->handle[id].ovlw_used = 0;
#endif
}


// Allocates a handle ID. Returns ID if succesful, or -1 on error.
static int ioAllocID(struct s_io_state *iostate) {
	int i;
	if(iostate->count < iostate->max) {
		for(i=0; i<iostate->max; i++) {
			if(!iostate->handle[i].enabled) {
				iostate->handle[i].enabled = 1;
#if defined(IO_WINDOWS)
				iostate->handle[i].ovlr.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				iostate->handle[i].ovlw.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif
				iostate->count++;
				return i;
			}
		}
	}
	return -1;
}


// Deallocates a handle ID.
static void ioDeallocID(struct s_io_state *iostate, const int id) {
	if((iostate->count > 0) && (id >= 0) && (id < iostate->max)) {
		if(iostate->handle[id].enabled) {
#if defined(IO_WINDOWS)
			CloseHandle(iostate->handle[id].ovlr.hEvent);
			CloseHandle(iostate->handle[id].ovlw.hEvent);
#endif
			ioResetID(iostate, id);
			iostate->count--;
		}
	}
}


// Closes a handle ID.
static void ioClose(struct s_io_state *iostate, const int id) {
	if(id >= 0 && id < iostate->max) {
		if(iostate->handle[id].enabled) {
			if(iostate->handle[id].open) {
				close(iostate->handle[id].fd);
				iostate->handle[id].open = 0;
			}
#if defined(IO_WINDOWS)
			if(iostate->handle[id].open_h) {
				CloseHandle(iostate->handle[id].fd_h);
				iostate->handle[id].open_h = 0;
			}
#endif
		}
		ioDeallocID(iostate, id);
	}
}


// Opens a socket. Returns handle ID if successful, or -1 on error.
static int ioOpenSocket(struct s_io_state *iostate, const int iotype, const char *bindaddress, const char *bindport, const int domain, const int type, const int protocol) {
	int id;
	int sockfd;

	int so;
	int fd;
	const char *zero_c = "0";
	const char *useport;
	const char *useaddr;
	struct addrinfo *d;
	struct addrinfo *di;
	struct addrinfo hints;

#if defined(IO_LINUX) || defined(IO_BSD)

	if((fd = socket(domain, type, 0)) < 0) {
		return -1;
	}
	if((fcntl(fd,F_SETFL,O_NONBLOCK)) < 0) {
		close(fd);
		return -1;
	}

#if defined(AF_INET6) && defined(IPPROTO_IPV6) && defined(IPV6_V6ONLY)
	so = 1;
	if(domain == AF_INET6) {
		setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&so, sizeof(int));
	}
#endif
	so = iostate->sockmark;
	if(so > 0) {
#if defined(SO_MARK)
		if(setsockopt(fd, SOL_SOCKET, SO_MARK, (void *)&so, sizeof(int)) < 0) { 
			close(fd);
			return -1;
		}
#else
		close(fd);
		return -1; // unsupported feature
#endif
	}

#elif defined(IO_WINDOWS)

	if((fd = WSASocket(domain, type, 0, 0, 0, WSA_FLAG_OVERLAPPED)) < 0) {
		return -1;
	}

#else

	#error not implemented
	return -1;

#endif

	if(iostate->debug > 0) {
		so = iostate->debug;
		setsockopt(fd, SOL_SOCKET, SO_DEBUG, (void *)&so, sizeof(int));
	}

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = domain;
	hints.ai_socktype = type;
	hints.ai_protocol = protocol;
	hints.ai_flags = AI_PASSIVE;
	if(ioStrlen(bindaddress, 255) > 0) {
		useaddr = bindaddress;
	}
	else {
		useaddr = NULL;
	}
	if(ioStrlen(bindport, 255) > 0) {
		useport = bindport;
	}
	else {
		useport = zero_c;
	}

	d = NULL;
	di = NULL;
	if(getaddrinfo(useaddr, useport, &hints, &d) != 0) {
		close(fd);
		return -1;
	}

	di = d;
	sockfd = -1;
	while(di != NULL) {
		if(bind(fd, di->ai_addr, di->ai_addrlen) == 0) {
			sockfd = fd;
			break;
		}
		di = di->ai_next;
	}
	freeaddrinfo(d);

	if(sockfd < 0) {
		close(fd);
		return -1;
	}

	if((id = ioAllocID(iostate)) < 0) {
		close(fd);
		return -1;
	}

	iostate->handle[id].fd = sockfd;
	iostate->handle[id].type = iotype;
	iostate->handle[id].open = 1;

	return id;
}


// Opens an IPv6 UDP socket. Returns handle ID if successful, or -1 on error.
static int ioOpenSocketV6(struct s_io_state *iostate, const char *bindaddress, const char *bindport) {
	return ioOpenSocket(iostate, IO_TYPE_SOCKET_V6, bindaddress, bindport, AF_INET6, SOCK_DGRAM, 0);
}


// Opens an IPv4 UDP socket. Returns handle ID if successful, or -1 on error.
static int ioOpenSocketV4(struct s_io_state *iostate, const char *bindaddress, const char *bindport) {
	return ioOpenSocket(iostate, IO_TYPE_SOCKET_V4, bindaddress, bindport, AF_INET, SOCK_DGRAM, 0);
}


// Helper functions for TAP devices on Windows.
#if defined(IO_WINDOWS)
#define IO_TAPWIN_IOCTL(request,method) CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)
#define IO_TAPWIN_IOCTL_SET_MEDIA_STATUS IO_TAPWIN_IOCTL(6, METHOD_BUFFERED)
#define IO_TAPWIN_ADAPTER_KEY "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define IO_TAPWIN_NETWORK_CONNECTIONS_KEY "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define IO_TAPWIN_USERMODEDEVICEDIR "\\\\.\\Global\\"
#define IO_TAPWIN_TAPSUFFIX ".tap"
#define IO_TAPWIN_SEARCH_IF_GUID_FROM_NAME 0
#define IO_TAPWIN_SEARCH_IF_NAME_FROM_GUID 1
static char *ioOpenTAPWINSearch(char *value, char *key, int type) {
	int i = 0;
	LONG status;
	DWORD len;
	HKEY net_conn_key;
	BOOL found = FALSE;
	char guid[256];
	char ifname[256];
	char conn_string[512];
	HKEY conn_key;
	DWORD value_type;
	if (!value || !key) {
		return NULL;
	}
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, IO_TAPWIN_NETWORK_CONNECTIONS_KEY, 0, KEY_READ, &net_conn_key);
	if (status != ERROR_SUCCESS) {
		return NULL;
	}
	while (!found) {
		len = sizeof(guid);
		status = RegEnumKeyEx(net_conn_key, i++, guid, &len, NULL, NULL, NULL, NULL);
		if(status == ERROR_NO_MORE_ITEMS) {
			break;
		}
		else if(status != ERROR_SUCCESS) {
			continue;
		}
		memset(conn_string, 0, 512);
		memcpy(&conn_string[ioStrlen(conn_string, 511)], IO_TAPWIN_NETWORK_CONNECTIONS_KEY, strlen(IO_TAPWIN_NETWORK_CONNECTIONS_KEY));
		memcpy(&conn_string[ioStrlen(conn_string, 511)], "\\", 1);
		memcpy(&conn_string[ioStrlen(conn_string, 511)], guid, ioStrlen(guid, 255));
		memcpy(&conn_string[ioStrlen(conn_string, 511)], "\\Connection", 11);
		status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, conn_string, 0, KEY_READ, &conn_key);
		if(status != ERROR_SUCCESS) {
			continue;
		}
		len = sizeof(ifname);
		status = RegQueryValueEx(conn_key, "Name", NULL, &value_type, (BYTE *)ifname, &len);
		if(status != ERROR_SUCCESS || value_type != REG_SZ) {
			RegCloseKey(conn_key);
			continue;
		}
		switch (type) {
		case IO_TAPWIN_SEARCH_IF_GUID_FROM_NAME:
			if(!strcmp(key, ifname)) {
				strcpy(value, guid);
				found = TRUE;
			}
			break;
		case IO_TAPWIN_SEARCH_IF_NAME_FROM_GUID:
			if(!strcmp(key, guid)) {
				strcpy(value, ifname);
				found = TRUE;
			}
			break;
		default:
			break;
		}
		RegCloseKey(conn_key);
	}
	RegCloseKey(net_conn_key);
	if(found) {
		return value;
	}
	return NULL;
}
static HANDLE ioOpenTAPWINDev(char *guid, char *dev) {
	HANDLE handle;
	ULONG len, status;
	char device_path[512];
	memset(device_path, 0, 512);
	memcpy(&device_path[ioStrlen(device_path, 511)], IO_TAPWIN_USERMODEDEVICEDIR, strlen(IO_TAPWIN_USERMODEDEVICEDIR));
	memcpy(&device_path[ioStrlen(device_path, 511)], guid, ioStrlen(guid, 255));
	memcpy(&device_path[ioStrlen(device_path, 511)], IO_TAPWIN_TAPSUFFIX, strlen(IO_TAPWIN_TAPSUFFIX));
	handle = CreateFile(device_path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
	if (handle == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}
	status = TRUE;
	if(!DeviceIoControl(handle, IO_TAPWIN_IOCTL_SET_MEDIA_STATUS, &status, sizeof(status), &status, sizeof(status), &len, NULL)) {
		return INVALID_HANDLE_VALUE;
	}
	return handle;
}
static HANDLE ioOpenTAPWINHandle(char *tapname, const char *reqname, const int reqname_len) {
	HANDLE handle = INVALID_HANDLE_VALUE;
	HKEY unit_key;
	char guid[256];
	char comp_id[256];
	char enum_name[256];
	char unit_string[512];
	char tmpname[256];
	int tmpname_len;
	BOOL found = FALSE;
	HKEY adapter_key;
	DWORD value_type;
	LONG status;
	DWORD len;
	int i;
	memset(tmpname, 0, 256);
	if(reqname_len > 0) {
		memcpy(tmpname, reqname, reqname_len);
	}
	tmpname_len = ioStrlen(tmpname, 255);
	if(tmpname_len > 0) {
		if(!(ioOpenTAPWINSearch(guid, tmpname, IO_TAPWIN_SEARCH_IF_GUID_FROM_NAME))) {
			return INVALID_HANDLE_VALUE;
		}
		if((handle = (ioOpenTAPWINDev(guid, tmpname))) == INVALID_HANDLE_VALUE) {
			return INVALID_HANDLE_VALUE;
		}
		if(tapname != NULL) {
			memcpy(tapname, tmpname, tmpname_len);
			tapname[tmpname_len] = '\0';
		}
		return handle;
	}
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, IO_TAPWIN_ADAPTER_KEY, 0, KEY_READ, &adapter_key);
	if(status != ERROR_SUCCESS) {
		return INVALID_HANDLE_VALUE;
	}
	i=0;
	while(!found) {
		len = sizeof(enum_name);
		status = RegEnumKeyEx(adapter_key, i++, enum_name, &len, NULL, NULL, NULL, NULL);
		if(status == ERROR_NO_MORE_ITEMS) {
			break;
		}
		else if(status != ERROR_SUCCESS) {
			continue;
		}
		memset(unit_string, 0, 512);
		memcpy(&unit_string[ioStrlen(unit_string, 511)], IO_TAPWIN_ADAPTER_KEY, strlen(IO_TAPWIN_ADAPTER_KEY));
		memcpy(&unit_string[ioStrlen(unit_string, 511)], "\\", 1);
		memcpy(&unit_string[ioStrlen(unit_string, 511)], enum_name, ioStrlen(enum_name, 255));
		status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, unit_string, 0, KEY_READ, &unit_key);
		if(status != ERROR_SUCCESS) {
			continue;
		}
		len = sizeof(comp_id);
		status = RegQueryValueEx(unit_key, "ComponentId", NULL, &value_type, (BYTE *)comp_id, &len);
		if(status != ERROR_SUCCESS || value_type != REG_SZ) {
			RegCloseKey(unit_key);
			continue;
		}
		len = sizeof(guid);
		status = RegQueryValueEx(unit_key, "NetCfgInstanceId", NULL, &value_type, (BYTE *)guid, &len);
		if(status != ERROR_SUCCESS || value_type != REG_SZ) {
			RegCloseKey(unit_key);
			continue;
		}
		ioOpenTAPWINSearch(tmpname, guid, IO_TAPWIN_SEARCH_IF_NAME_FROM_GUID);
		handle = ioOpenTAPWINDev(guid, tmpname);
		if(handle != INVALID_HANDLE_VALUE) {
			found = TRUE;
		}
		RegCloseKey(unit_key);
	}
	RegCloseKey(adapter_key);
	if(handle == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}
	if(tapname != NULL) {
		tapname[0] = '\0';
		tmpname_len = ioStrlen(tmpname, 255);
		if(tmpname_len > 0) {
			memcpy(tapname, tmpname, tmpname_len);
			tapname[tmpname_len] = '\0';
		}
	}
	return handle;
}
#endif


// Opens a TAP device. Returns handle ID if succesful, or -1 on error.
static int ioOpenTAP(struct s_io_state *iostate, char *tapname, const char *reqname) {
	int id;
	int tapfd;
	char filename[512];
	int name_len;
	int req_len;

	req_len = ioStrlen(reqname, 255);
	memset(filename, 0, 512);
	id = -1;
	tapfd = -1;

#if defined(IO_LINUX)

	struct ifreq ifr;

	memcpy(filename, "/dev/net/tun", 12);
	tapfd = open(filename,(O_RDWR | O_NONBLOCK));

	if(tapfd < 0) {
		return -1;
	}

	memset(&ifr,0,sizeof(struct ifreq));
	ifr.ifr_flags = (IFF_TAP | IFF_NO_PI);
	if(req_len > 0) {
		if(req_len < IFNAMSIZ) {
			memcpy(ifr.ifr_name, reqname, req_len);
		}
		else {
			close(tapfd);
			return -1;
		}
	}

	if(ioctl(tapfd,TUNSETIFF,(void *)&ifr) < 0) {
		close(tapfd);
		return -1;
	}

	if((id = ioAllocID(iostate)) < 0) {
		close(tapfd);
		return -1;
	}

	if(tapname != NULL) {
		name_len = ioStrlen(ifr.ifr_name, (IFNAMSIZ-1));
		tapname[0] = '\0';
		if(name_len > 0 && name_len < 255) {
			memcpy(tapname, ifr.ifr_name, name_len);
			tapname[name_len] = '\0';
		}
	}

	iostate->handle[id].open = 1;

#elif defined(IO_BSD)

	int i;

	memcpy(filename, "/dev/", 5);

	if(req_len > 0) {
		memcpy(&filename[5], reqname, 256);
		tapfd = open(filename, O_RDWR);
		if(tapfd < 0) {
			return -1;
		}
		if(fcntl(tapfd, F_SETFL, O_NONBLOCK) < 0) {
			close(tapfd);
			return -1;
		}
	}
	else {
		memcpy(&filename[5], "tap", 3);
		i = 0;
		while((i < 10) && (tapfd < 0)) {
			filename[8] = (i + 48);
			tapfd = open(filename, O_RDWR);
			if(!(tapfd < 0)) {
				if(fcntl(tapfd, F_SETFL, O_NONBLOCK) < 0) {
					close(tapfd);
					tapfd = -1;
				}
			}
			i++;
		}
	}

	if(tapfd < 0) {
		return -1;
	}

	if((id = ioAllocID(iostate)) < 0) {
		close(tapfd);
		return -1;
	}

	if(tapname != NULL) {
		name_len = ioStrlen(&filename[5], 255);
		tapname[0] = '\0';
		if(name_len > 0 && name_len < 255) {
			memcpy(tapname, &filename[5], name_len);
			tapname[name_len] = '\0';
		}
	}

	iostate->handle[id].open = 1;

#elif defined(IO_WINDOWS)

	char tmpname[256];
	HANDLE handle;

	memset(tmpname, 0, 256);
	handle = ioOpenTAPWINHandle(tmpname, reqname, req_len);
	if(handle == INVALID_HANDLE_VALUE) {
		return -1;
	}

	if((id = ioAllocID(iostate)) < 0) {
		CloseHandle(handle);
		return -1;
	}

	if(tapname != NULL) {
		name_len = ioStrlen(tmpname, 255);
		tapname[0] = '\0';
		if(name_len > 0 && name_len < 255) {
			memcpy(tapname, tmpname, name_len);
			tapname[name_len] = '\0';
		}
	}

	iostate->handle[id].fd_h = handle;
	iostate->handle[id].open_h = 1;

#else

	#error not implemented
	return -1;

#endif

	iostate->handle[id].fd = tapfd;
	iostate->handle[id].type = IO_TYPE_FILE;
	return id;
}


// Opens STDIN. Returns handle ID if succesful, or -1 on error.
static int ioOpenSTDIN(struct s_io_state *iostate) {
	int id;

#if defined(IO_LINUX) || defined(IO_BSD)

	if((fcntl(STDIN_FILENO,F_SETFL,O_NONBLOCK)) < 0) {
		return -1;
	}
	
	if((id = ioAllocID(iostate)) < 0) {
		return -1;
	}

	iostate->handle[id].fd = STDIN_FILENO;
	iostate->handle[id].type = IO_TYPE_FILE;

#elif defined(IO_WINDOWS)

	return -1;

#else

	#error not implemented
	return -1;

#endif

	return id;
}


// Receives an UDP packet. Returns length of received message, or 0 if nothing is received.
static int ioHelperRecvFrom(struct s_io_handle *handle, unsigned char *recv_buf, const int recv_buf_size, struct sockaddr *source_sockaddr, socklen_t *source_sockaddr_len) {
	int len;

#if defined(IO_LINUX) || defined(IO_BSD)

	len = recvfrom(handle->fd, recv_buf, recv_buf_size, 0, source_sockaddr, source_sockaddr_len);

#elif defined(IO_WINDOWS)

	WSABUF wsabuf;
	DWORD flags;

	len = 0;
	wsabuf.buf = (char *)recv_buf;
	wsabuf.len = recv_buf_size;
	flags = 0;
	if(!(handle->ovlr_used)) {
		if(WSARecvFrom(handle->fd, &wsabuf, 1, NULL, &flags, source_sockaddr, source_sockaddr_len, &handle->ovlr, NULL) == 0) {
			handle->ovlr_used = 1;
		}
		else {
			if(WSAGetLastError() == WSA_IO_PENDING) {
				handle->ovlr_used = 1;
			}
		}
	}

#else

	#error not implemented
	len = 0;

#endif

	if(len > 0) {
		return len;
	}
	else {
		return 0;
	}
}


#if defined(IO_WINDOWS)
// Finish receiving an UDP packet. Returns amount of bytes read, or 0 if nothing is read.
static int ioHelperFinishRecvFrom(struct s_io_handle *handle) {
	DWORD len;
	DWORD flags;

	len = 0;
	flags = 0;
	if(handle->ovlr_used) {
		if(WSAGetOverlappedResult(handle->fd, &handle->ovlr, &len, FALSE, &flags) == TRUE) {
			handle->ovlr_used = 0;
			ResetEvent(handle->ovlr.hEvent);
			if(len > 0) {
				handle->content_len = len;
			}
		}
		else {
			if(WSAGetLastError() != WSA_IO_INCOMPLETE) {
				handle->ovlr_used = 0;
				ResetEvent(handle->ovlr.hEvent);
			}				
		}
	}

	if(len > 0) {
		return len;
	}
	else {
		return 0;
	}
}
#endif


// Sends an UDP packet. Returns length of sent message.
static int ioHelperSendTo(struct s_io_handle *handle, const unsigned char *send_buf, const int send_buf_size, const struct sockaddr *destination_sockaddr, const socklen_t destination_sockaddr_len) {
	int len;

#if defined(IO_LINUX) || defined(IO_BSD)

	len = sendto(handle->fd, send_buf, send_buf_size, 0, destination_sockaddr, destination_sockaddr_len);

#elif defined(IO_WINDOWS)

	int ovlw_used;
	WSABUF wsabuf;
	DWORD flags;
	DWORD dwlen;

	len = 0;
	wsabuf.buf = (char *)send_buf;
	wsabuf.len = send_buf_size;
	flags = 0;
	ovlw_used = 0;
	if(WSASendTo(handle->fd, &wsabuf, 1, NULL, flags, destination_sockaddr, destination_sockaddr_len, &handle->ovlw, NULL) == 0) {
		ovlw_used = 1;
	}
	else {
		if(WSAGetLastError() == WSA_IO_PENDING) {
			ovlw_used = 1;
		}
	}
	if(ovlw_used) {
		if(WSAGetOverlappedResult(handle->fd, &handle->ovlw, &dwlen, TRUE, &flags) == TRUE) {	
			len = dwlen;
		}
		ResetEvent(handle->ovlw.hEvent);
	}

#else

	#error not implemented
	len = 0;

#endif

	if(len > 0) {
		return len;
	}
	else {
		return 0;
	}
}


// Reads from file. Returns amount of bytes read, or 0 if nothing is read.
static int ioHelperReadFile(struct s_io_handle *handle, unsigned char *read_buf, const int read_buf_size) {
	int len;

#if defined(IO_LINUX) || defined(IO_BSD)

	len = read(handle->fd, read_buf, read_buf_size);

#elif defined(IO_WINDOWS)

	len = 0;
	if(!(handle->ovlr_used)) {
		if(ReadFile(handle->fd_h, read_buf, read_buf_size, NULL, &handle->ovlr) == TRUE) {
			handle->ovlr_used = 1;
		}
		else {
			if(GetLastError() == ERROR_IO_PENDING) {
				handle->ovlr_used = 1;
			}
		}
	}

#else

	#error not implemented
	len = 0;

#endif

	if(len > 0) {
		return len;
	}
	else {
		return 0;
	}
}


#if defined(IO_WINDOWS)
// Finish reading from file. Returns amount of bytes read, or 0 if nothing is read.
static int ioHelperFinishReadFile(struct s_io_handle *handle) {
	DWORD len;

	len = 0;
	if(handle->ovlr_used) {
		if(GetOverlappedResult(handle->fd_h, &handle->ovlr, &len, FALSE) == TRUE) {
			handle->ovlr_used = 0;
			ResetEvent(handle->ovlr.hEvent);
			if(len > 0) {
				handle->content_len = len;
			}
		}
		else {
			if(GetLastError() != ERROR_IO_INCOMPLETE) {
				handle->ovlr_used = 0;
				ResetEvent(handle->ovlr.hEvent);
			}
		}
	}

	if(len > 0) {
		return len;
	}
	else {
		return 0;
	}
}
#endif


// Writes to file. Returns amount of bytes written.
static int ioHelperWriteFile(struct s_io_handle *handle, const unsigned char *write_buf, const int write_buf_size) {
	int len;

#if defined(IO_LINUX) || defined(IO_BSD)

	len = write(handle->fd, write_buf, write_buf_size);

#elif defined(IO_WINDOWS)

	int ovlw_used;
	DWORD dwlen;

	len = 0;
	ovlw_used = 0;
	if(WriteFile(handle->fd_h, write_buf, write_buf_size, NULL, &handle->ovlw) == TRUE) {
		ovlw_used = 1;
	}
	else {
		if(GetLastError() == ERROR_IO_PENDING) {
			ovlw_used = 1;
		}
	}
	if(ovlw_used) {
		if(GetOverlappedResult(handle->fd_h, &handle->ovlw, &dwlen, TRUE) == TRUE) {
			len = dwlen;
		}
		ResetEvent(handle->ovlw.hEvent);
	}

#else

	#error not implemented
	len = 0;

#endif

	if(len > 0) {
		return len;
	}
	else {
		return 0;
	}
}


// Prepares read operation on specified handle ID.
static void ioPreRead(struct s_io_state *iostate, const int id) {
	int ret;
	socklen_t sockaddr_len;
	switch(iostate->handle[id].type) {
		case IO_TYPE_SOCKET_V6:
			sockaddr_len = sizeof(struct sockaddr_in6);
			ret = ioHelperRecvFrom(&iostate->handle[id], &iostate->mem[id * iostate->bufsize], iostate->bufsize, (struct sockaddr *)&iostate->handle[id].source_sockaddr, &sockaddr_len);
			break;
		case IO_TYPE_SOCKET_V4:
			sockaddr_len = sizeof(struct sockaddr_in);
			ret = ioHelperRecvFrom(&iostate->handle[id], &iostate->mem[id * iostate->bufsize], iostate->bufsize, (struct sockaddr *)&iostate->handle[id].source_sockaddr, &sockaddr_len);
			break;
		case IO_TYPE_FILE:
			ret = ioHelperReadFile(&iostate->handle[id], &iostate->mem[id * iostate->bufsize], iostate->bufsize);
			break;
		default:
			ret = 0;
			break;
	}
	iostate->handle[id].content_len = ret;
}


// Reads data on specified handle ID. Returns amount of bytes read, or 0 if nothing is read.
static int ioRead(struct s_io_state *iostate, const int id) {
	int ret;

#if defined(IO_LINUX) || defined(IO_BSD)

	ret = iostate->handle[id].content_len;

#elif defined(IO_WINDOWS)

	switch(iostate->handle[id].type) {
		case IO_TYPE_SOCKET_V6:
			ret = ioHelperFinishRecvFrom(&iostate->handle[id]);
			break;
		case IO_TYPE_SOCKET_V4:
			ret = ioHelperFinishRecvFrom(&iostate->handle[id]);
			break;
		case IO_TYPE_FILE:
			ret = ioHelperFinishReadFile(&iostate->handle[id]);
			break;
		default:
			ret = 0;
			break;
	}

#else

	#error not implemented
	ret = 0;

#endif

	return ret;
}


// Waits for data on any handle and read it. Returns the amount of handles where data have been read.
static int ioReadAll(struct s_io_state *iostate) {
	int ret;
	int i;

#if defined(IO_LINUX) || defined(IO_BSD)

	fd_set fdset;
	struct timeval seltimeout;
	int fd, fdh;

	seltimeout.tv_sec = iostate->timeout;
	seltimeout.tv_usec = 0;

	fdh = 0;
	FD_ZERO(&fdset);
	for(i=0; i<iostate->max; i++) {
		if(iostate->handle[i].enabled) {
			fd = iostate->handle[i].fd;
			FD_SET(fd, &fdset);
			if(fdh < (fd+1)) {
				fdh = (fd+1);
			}
		}
	}

	ret = 0;
	if(!(fdh < 0)) {
		if(select(fdh, &fdset, NULL, NULL, &seltimeout) > 0) {
			for(i=0; i<iostate->max; i++) {
				if(iostate->handle[i].enabled) {
					if(FD_ISSET(iostate->handle[i].fd, &fdset)) {
						ioPreRead(iostate, i);
						if(ioRead(iostate, i) > 0) {
							ret++;
						}
					}
				}
			}
		}
	}

#elif defined(IO_WINDOWS)

	HANDLE events[iostate->max];
	int fdc;

	fdc = 0;
	for(i=0; i<iostate->max; i++) {
		if(iostate->handle[i].enabled) {
			ioPreRead(iostate, i);
			if(iostate->handle[i].ovlr_used) {
				events[fdc] = iostate->handle[i].ovlr.hEvent;
				fdc++;
			}
		}
	}

	ret = 0;
	if(fdc > 0) {
		WaitForMultipleObjects(fdc, events, FALSE, (iostate->timeout * 1000));
		for(i=0; i<iostate->max; i++) {
			if(ioRead(iostate, i) > 0) {
				ret++;
			}
		}
	}
	else {
		Sleep(iostate->timeout * 1000);
	}

#else

	#error not implemented
	ret = 0;

#endif

	return ret;
}


// Writes data on specified handle ID. Returns amount of bytes written.
static int ioWrite(struct s_io_state *iostate, const int id, const unsigned char *write_buf, const int write_buf_size, const struct s_io_addr *destination_addr) {
	int ret;
	struct sockaddr_storage destination_sockaddr;
	struct sockaddr_in6 *destination_sockaddr_v6;
	struct sockaddr_in *destination_sockaddr_v4;

	switch(iostate->handle[id].type) {
		case IO_TYPE_SOCKET_V6:
			if(destination_addr != NULL) {
				if(memcmp(destination_addr->addr, IO_ADDRTYPE_UDP6, 4) == 0) {
					destination_sockaddr_v6 = (struct sockaddr_in6 *)&destination_sockaddr;
					memset(destination_sockaddr_v6, 0, sizeof(struct sockaddr_in6));
					destination_sockaddr_v6->sin6_family = AF_INET6;
					memcpy(destination_sockaddr_v6->sin6_addr.s6_addr, &destination_addr->addr[4], 16);
					memcpy(&destination_sockaddr_v6->sin6_port, &destination_addr->addr[20], 2);
					ret = ioHelperSendTo(&iostate->handle[id], write_buf, write_buf_size, (struct sockaddr *)destination_sockaddr_v6, sizeof(struct sockaddr_in6));
				}
				else if((iostate->nat64clat > 0) && (memcmp(destination_addr->addr, IO_ADDRTYPE_UDP4, 4) == 0)) {
					destination_sockaddr_v6 = (struct sockaddr_in6 *)&destination_sockaddr;
					memset(destination_sockaddr_v6, 0, sizeof(struct sockaddr_in6));
					destination_sockaddr_v6->sin6_family = AF_INET6;
					memcpy(destination_sockaddr_v6->sin6_addr.s6_addr, iostate->nat64_prefix, 12);
					memcpy(&destination_sockaddr_v6->sin6_addr.s6_addr[12], &destination_addr->addr[4], 4);
					memcpy(&destination_sockaddr_v6->sin6_port, &destination_addr->addr[8], 2);
					ret = ioHelperSendTo(&iostate->handle[id], write_buf, write_buf_size, (struct sockaddr *)destination_sockaddr_v6, sizeof(struct sockaddr_in6));
				}
				else {
					ret = 0;
				}
			}
			else {
				ret = 0;
			}
			break;
		case IO_TYPE_SOCKET_V4:
			if(destination_addr != NULL) {
				if(memcmp(destination_addr->addr, IO_ADDRTYPE_UDP4, 4) == 0) {
					destination_sockaddr_v4 = (struct sockaddr_in *)&destination_sockaddr;
					memset(destination_sockaddr_v4, 0, sizeof(struct sockaddr_in));
					destination_sockaddr_v4->sin_family = AF_INET;
					memcpy(&destination_sockaddr_v4->sin_addr.s_addr, &destination_addr->addr[4], 4);
					memcpy(&destination_sockaddr_v4->sin_port, &destination_addr->addr[8], 2);
					ret = ioHelperSendTo(&iostate->handle[id], write_buf, write_buf_size, (struct sockaddr *)destination_sockaddr_v4, sizeof(struct sockaddr_in));
				}
				else {
					ret = 0;
				}
			}
			else {
				ret = 0;
			}
			break;
		case IO_TYPE_FILE:
			ret = ioHelperWriteFile(&iostate->handle[id], write_buf, write_buf_size);
			break;
		default:
			ret = 0;
			break;
	}

	return ret;
}


// Writes data on one handle ID of the specified group. Returns amount of bytes written.
static int ioWriteGroup(struct s_io_state *iostate, const int group, const unsigned char *write_buf, const int write_buf_size, const struct s_io_addr *destination_addr) {
	int i;
	int ret;
	for(i=0; i<iostate->max; i++) {
		if(iostate->handle[i].group_id == group) {
			ret = ioWrite(iostate, i, write_buf, write_buf_size, destination_addr);
			if(ret > 0) {
				return ret;
			}
		}
	}
	return 0;
}


// Returns the first handle of the specified group that has data, or -1 if there is none.
static int ioGetGroup(struct s_io_state *iostate, const int group) {
	int i;
	for(i=0; i<iostate->max; i++) {
		if((iostate->handle[i].group_id == group) && (iostate->handle[i].content_len > 0)) {
			return i;
		}
	}
	return -1;
}


// Returns a pointer to the data buffer of the specified handle ID.
static unsigned char * ioGetData(struct s_io_state *iostate, const int id) {
	return &iostate->mem[id * iostate->bufsize];
}


// Returns the data buffer content length of the specified handle ID, or zero if there are no data.
static int ioGetDataLen(struct s_io_state *iostate, const int id) {
	return iostate->handle[id].content_len;
}


// Returns a pointer to the current source address of the specified handle ID.
static struct s_io_addr * ioGetAddr(struct s_io_state *iostate, const int id) {
	struct s_io_addr *ioaddr;
	struct sockaddr_storage *source_sockaddr;
	struct sockaddr_in6 *source_sockaddr_v6;
	struct sockaddr_in *source_sockaddr_v4;

	ioaddr = &iostate->handle[id].source_addr;

	source_sockaddr = &iostate->handle[id].source_sockaddr;
	switch(iostate->handle[id].type) {
		case IO_TYPE_SOCKET_V6: // copy v6 address
			source_sockaddr_v6 = (struct sockaddr_in6 *)source_sockaddr;
			if((iostate->nat64clat > 0) && (memcmp(source_sockaddr_v6->sin6_addr.s6_addr, iostate->nat64_prefix, 12) == 0)) {
				memcpy(&ioaddr->addr[0], IO_ADDRTYPE_UDP4, 4); // set address type
				memcpy(&ioaddr->addr[4], &source_sockaddr_v6->sin6_addr.s6_addr[12], 4); // copy source IPv4 address, extracted from NAT64 address
				memcpy(&ioaddr->addr[8], &source_sockaddr_v6->sin6_port, 2); // copy source port
				memcpy(&ioaddr->addr[10], "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 14); // empty bytes
			}
			else {
				memcpy(&ioaddr->addr[0], IO_ADDRTYPE_UDP6, 4); // set address type
				memcpy(&ioaddr->addr[4], source_sockaddr_v6->sin6_addr.s6_addr, 16); // copy source IPv6 address
				memcpy(&ioaddr->addr[20], &source_sockaddr_v6->sin6_port, 2); // copy source port
				memcpy(&ioaddr->addr[22], "\x00\x00", 2); // empty bytes
			}
			break;
		case IO_TYPE_SOCKET_V4: // copy v4 address
			source_sockaddr_v4 = (struct sockaddr_in *)source_sockaddr;
			memcpy(&ioaddr->addr[0], IO_ADDRTYPE_UDP4, 4); // set address type
			memcpy(&ioaddr->addr[4], &source_sockaddr_v4->sin_addr.s_addr, 4); // copy source IPv4 address
			memcpy(&ioaddr->addr[8], &source_sockaddr_v4->sin_port, 2); // copy source port
			memcpy(&ioaddr->addr[10], "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 14); // empty bytes
			break;
		default:
			break;
	}

	return ioaddr;
}


// Clear data of the specified handle ID.
static void ioGetClear(struct s_io_state *iostate, const int id) {
	iostate->handle[id].content_len = 0;
}


// Set group ID of handle ID
static void ioSetGroup(struct s_io_state *iostate, const int id, const int group) {
	if(id >= 0 && id < iostate->max) {
		iostate->handle[id].group_id = group;
	}
}


// Set sockmark value for new sockets.
static void ioSetSockmark(struct s_io_state *iostate, const int io_sockmark) {
	if(io_sockmark > 0) {
		iostate->sockmark = io_sockmark;
	}
	else {
		iostate->sockmark = 0;
	}
}


// Enable/Disable NAT64 CLAT support.
static void ioSetNat64Clat(struct s_io_state *iostate, const int enable) {
	if(enable > 0) {
		iostate->nat64clat = 1;
	}
	else {
		iostate->nat64clat = 0;
	}
}


// Set IO read timeout (in seconds).
static void ioSetTimeout(struct s_io_state *iostate, const int io_timeout) {
	if(io_timeout > 0) {
		iostate->timeout = io_timeout;
	}
	else {
		iostate->timeout = 0;
	}
}


// Closes all handles and resets defaults.
static void ioReset(struct s_io_state *iostate) {
	int i;
	for(i=0; i<iostate->max; i++) {
		ioClose(iostate, i);
		ioResetID(iostate, i);
	}
	iostate->timeout = 1;
	iostate->sockmark = 0;
	iostate->nat64clat = 0;
	memcpy(iostate->nat64_prefix, "\x00\x64\xff\x9b\x00\x00\x00\x00\x00\x00\x00\x00", 12);
	iostate->debug = 0;
}


// Create IO state structure. Returns 1 on success.
static int ioCreate(struct s_io_state *iostate, const int io_bufsize, const int io_max) {
#ifdef IO_WINDOWS
	WSADATA wsadata;
	if(WSAStartup(MAKEWORD(2,2), &wsadata) != 0) { return 0; }
#endif

	if((io_bufsize > 0) && (io_max > 0)) { // check parameters
		if((iostate->mem = (malloc(io_bufsize * io_max))) != NULL) {
			if((iostate->handle = (malloc(sizeof(struct s_io_handle) * io_max))) != NULL) {
				iostate->bufsize = io_bufsize;
				iostate->max = io_max;
				iostate->count = 0;
				memset(iostate->mem, 0, (io_bufsize * io_max));
				memset(iostate->handle, 0, (sizeof(struct s_io_handle) * io_max));
				ioReset(iostate);
				return 1;
			}
			free(iostate->mem);
		}
	}
	return 0;
}


// Destroy IO state structure.
static void ioDestroy(struct s_io_state *iostate) {
	ioReset(iostate);
	free(iostate->handle);
	free(iostate->mem);
	iostate->bufsize = 0;
	iostate->max = 0;
	iostate->count = 0;

#ifdef IO_WINDOWS
	WSACleanup();
#endif
}


#endif // F_IO_C 
