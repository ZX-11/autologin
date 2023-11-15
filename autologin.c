#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

char* GET(char* hostname, char* path) {
	static char buf[4096];
	int sock = socket(AF_INET, SOCK_STREAM, 0);
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

char* urlencode(const char* src) {
	static char buf[64], temp[8];
	for (unsigned char* dest = buf; *src || (*dest = 0); src++) {
		if (isalnum(*src)) {
			*dest++ = *src;
		} else {
			int len = sprintf(temp, "%%%x", *src);
			strcpy(dest, temp);
			dest += len;
		}
	}
	return buf;
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		puts("usage: autologin [username] [password]");
		return 0;
	}
	static char buf[1024];
	for (;;) {
		char* resp = GET("10.10.43.3", "/");
		if (resp && strstr(resp, "Dr.COMWebLoginID_0.htm")) {
			sprintf(buf, "/drcom/"
					"login?callback=dr1003&DDDDD=%s&upass=%s&0MKKey=123456&"
					"R1=0&R2=&R3=0&R6=0&para=00&v6ip=&terminal_type=1&lang="
					"zh-cn&jsVersion=4.2.1&v=8321&lang=zh",
					argv[1], urlencode(argv[2]));
			puts(strstr(GET("10.10.43.3", buf), "\"result\":1") ? "自动登录成功" : "登录失败");
		}
		sleep(10);
	}
}
