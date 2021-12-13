
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define HIGHBYTE(word) (((word) >> 8) & 0xFF)
#define LOWBYTE(word) ((word)&0xFF)

void
comm(int sockfd)
{
	uint8_t request[8 + 256], response[8 + 256];
	uint16_t transId = 0;
	uint16_t length = 6;
	uint8_t unitId = 1;
	uint8_t functionCode = 3;
	uint16_t registerStart = 0;
	uint16_t registerCount = 10;

	for (;;) {
		memset(request, 0, sizeof(request));
		memset(response, 0, sizeof(response));

		transId++;
		request[0] = HIGHBYTE(transId);
		request[1] = LOWBYTE(transId);
		request[2] = 0; // protocol ID
		request[3] = 0; // protocol ID
		request[4] = HIGHBYTE(length);
		request[5] = LOWBYTE(length);
		request[6] = unitId;
		request[7] = functionCode;
		request[8] = HIGHBYTE(registerStart);
		request[9] = LOWBYTE(registerStart);
		request[10] = HIGHBYTE(registerCount);
		request[11] = LOWBYTE(registerCount);

		write(sockfd, request, length + 6);

		usleep(1000);

		fd_set readsocks;
		struct timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		FD_ZERO(&readsocks);
		FD_SET(sockfd, &readsocks);

		int sr = select(sockfd + 1, &readsocks, NULL, NULL, &timeout);
		printf("sr: %i\n", sr);
		if (sr <= 0) {
			return;
		}

		int br = read(sockfd, response, sizeof(response));
		printf("br: %i\n", br);
		for (int i = 0; i < br; i++) {
			printf("%i\t%i\t%02x\n", i, response[i], response[i]);
		}
		printf("---\n");
		printf("transaction: %u\n", response[0] * 256 + response[1]);
		printf("protocol zero: %u\n", response[2] * 256 + response[3]);
		printf("length: %u\n", response[4] * 256 + response[5]);
		printf("unit: %u\n", response[6]);
		printf("code: %u\n", response[7]);
		printf("bytes to follow: %u\n", response[8]);
		for (int i = 9; i < br - 1; i += 2) {
			uint16_t v = response[i] * 256 + response[i + 1];
			printf("%6i\t%04x\n", v, v);
		}
	}
}


int
main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in servaddr;

	// socket create and varification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed\n");
		return 1;
	}
	memset(&servaddr, 0, sizeof(servaddr));

	// assign IP address and PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	servaddr.sin_port = htons(atoi(argv[2]));

	// connect client to server
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed...\n");
		return 1;
	} else {
		printf("connected to the server..\n");
	}

	// communication
	comm(sockfd);

	// close the socket
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);

	return 0;
}
