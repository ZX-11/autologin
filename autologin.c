// Compile:       cc -s autologin.c -Ofast -o autologin 
// Windows mingw: cc -s autologin.c -Ofast -o autologin -lwsock32 -fexec-charset=gbk

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __WIN32
#include <WinSock2.h>
#define close(s) closesocket(s)
#define sleep(s) Sleep(1000*s)
#pragma comment(lib,"ws2_32")
typedef SOCKET socket_t;
static __attribute__((constructor)) void _init() {
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
}
static __attribute__((destructor)) void _clean() { WSACleanup(); }
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#endif

char* GET(char* hostname, char* path) {
	static char buf[4096];
	socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock != -1);
	struct sockaddr_in server = {
		.sin_addr.s_addr = inet_addr(hostname),
		.sin_family = AF_INET,
		.sin_port = htons(80),
	};
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1) return NULL;
	int len = sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, hostname);
	assert(send(sock, buf, len, 0) >= 0);
	len = recv(sock, buf, sizeof(buf) - 1, 0);
	if (strncmp(buf + len - 4, "\r\n\r\n", 4) == 0) {
		len += recv(sock, buf + len, sizeof(buf) - len - 1, 0);
	}
	buf[len] = 0;
	close(sock);
	return buf;
}

char* urlencode(const unsigned char* src) {
	static char buf[64];
	for (char* dest = buf; *src || (*dest = 0); src++) {
		if (isalnum(*src)) {
			*dest++ = *src;
		} else {
			dest += sprintf(dest, "%%%x", *src);
		}
	}
	return buf;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		puts("usage: autologin [username] [password] [interval (optional, default 10, in seconds)]");
		return 0;
	}
	static char buf[256];
	int interval = argc == 4 ? atoi(argv[3]) : 10;
	for (char *resp, *result; ; sleep(interval)) {
		if ((resp = GET("10.10.43.3", "/")) && (result = strstr(resp, "Dr.COMWebLoginID")) && result[17] == '0') {
			sprintf(buf, "/drcom/"
				"login?callback=dr1003&DDDDD=%s&upass=%s&0MKKey=123456&"
				"R1=0&R2=&R3=0&R6=0&para=00&v6ip=&terminal_type=1&lang="
				"zh-cn&jsVersion=4.2.1&v=8321&lang=zh",
				argv[1], urlencode((unsigned char*)argv[2]));
			time_t t = time(NULL);
			fputs(ctime(&t), stdout);
			puts((result = strstr(GET("10.10.43.3", buf), "result")) && result[8] == '1' ? "自动登录成功" : "登录失败");
		}
	}
}
