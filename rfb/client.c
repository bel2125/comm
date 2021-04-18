#include "../timing.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define MAX 256
#define SA struct sockaddr

#include "bmp.h"
#include "des.h"
#include "kbhit.h"

static unsigned long shot_nr = 0;


void
readall(int sockfd, void *buf, uint32_t len)
{
	uint32_t readb = 0;
	while (readb < len) {
		int s = read(sockfd, buf + readb, len - readb);
		if (s < 0) {
			fprintf(stderr, "Read error: %s\n", strerror(errno));
			exit(1);
		}
		readb += s;
	}
}


void
writeall(int sockfd, const void *buf, uint32_t len)
{
	uint32_t written = 0;
	while (written < len) {
		int s = write(sockfd, buf + written, len - written);
		if (s < 0) {
			fprintf(stderr, "Write error: %s\n", strerror(errno));
			exit(1);
		}
		written += s;
	}
}


void
readtext(int sockfd, uint32_t len)
{
	char *r = malloc(len + 1);
	if (!r) {
		fprintf(stderr, "Out of memory (%u)\n", len + 1);
		exit(1);
	}
	readall(sockfd, r, len);
	r[len] = 0;
	printf("%s", r);
	free(r);
}


static const uint8_t bitinverse[] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0,
    0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4,
    0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc,
    0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca,
    0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6,
    0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1,
    0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9,
    0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd,
    0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3,
    0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7,
    0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf,
    0x3f, 0xbf, 0x7f, 0xff,
};


int
proto(int sockfd)
{
	char buf[MAX];

	bzero(buf, sizeof(buf));
	readall(sockfd, buf, 12);
	printf("Server proto: %s\n", buf);
	if ((0 != memcmp(buf, "RFB 003.", 8)) || (buf[11] != '\n')) {
		printf("Version not supported\n");
		return 2;
	}

	bzero(buf, sizeof(buf));
	sprintf(buf, "RFB 003.003\n");
	writeall(sockfd, buf, 12);

	bzero(buf, sizeof(buf));
	readall(sockfd, buf, 4);
	uint32_t sec = ntohl(*(uint32_t *)buf);
	printf("Server security: %u\n", sec);

	if (sec == 1) {
		/* no security */

	} else if (sec == 2) {
		/* VNC password with flipped DES encoding of 16 byte challenge */
		uint8_t challenge[16];
		readall(sockfd, challenge, 16);

		uint8_t pass[256];
		memset(pass, 0, sizeof(pass));
		printf("Password required: ");
		fgets(pass, sizeof(pass), stdin);
		char *c = strchr(pass, '\r');
		if (c) {
			*c = 0;
		}
		c = strchr(pass, '\n');
		if (c) {
			*c = 0;
		}

		uint8_t response[16];
		memcpy(response, challenge, 16);

		uint8_t des_schedule[16][6];
		uint8_t passinv[8];
		memset(passinv, 0, sizeof(passinv));
		for (int32_t i = 0; i < 8; i++) {
			if (pass[i]) {
				passinv[i] = bitinverse[pass[i]];
			} else {
				break;
			}
		}
		des_key_setup(passinv, des_schedule, DES_ENCRYPT);
		des_crypt(response, response, des_schedule);
		des_crypt(response + 8, response + 8, des_schedule);

		writeall(sockfd, response, 16);

		uint32_t securityresult;
		readall(sockfd, &securityresult, 4);
		securityresult = ntohl(securityresult);
		if (securityresult) {
			printf("Security check failed: %u\n", securityresult);
			return 4;
		}

	} else {
		printf("Security %u not supported\n", sec);
		return 2;
	}

	bzero(buf, sizeof(buf));
	buf[0] = 0; /* shared ? */
	writeall(sockfd, buf, 1);

	bzero(buf, sizeof(buf));
	readall(sockfd, buf, 24);

	uint16_t width = ntohs(*(uint16_t *)(buf));
	uint16_t heigth = ntohs(*(uint16_t *)(buf + 2));
	printf("Width: %u\n", width);
	printf("Heigth: %u\n", heigth);

	uint8_t bpp = *(uint8_t *)(buf + 4);
	uint8_t depth = *(uint8_t *)(buf + 5);
	printf("BPP: %u\n", bpp);
	printf("Depth: %u\n", depth);

	uint8_t bigend = !!*(uint8_t *)(buf + 6);
	uint8_t truecol = !!*(uint8_t *)(buf + 7);
	printf("Big-Endian: %u\n", bigend);
	printf("TrueColor: %u\n", truecol);

	uint16_t red_max = ntohs(*(uint16_t *)(buf + 8));
	uint16_t green_max = ntohs(*(uint16_t *)(buf + 10));
	uint16_t blue_max = ntohs(*(uint16_t *)(buf + 12));
	printf("Red-Max: %u\n", red_max);
	printf("Green-Max: %u\n", green_max);
	printf("Blue-Max: %u\n", blue_max);

	uint8_t red_shift = *(uint8_t *)(buf + 14);
	uint8_t green_shift = *(uint8_t *)(buf + 15);
	uint8_t blue_shift = *(uint8_t *)(buf + 16);
	printf("Red-Shift: %u\n", red_shift);
	printf("Green-Shift: %u\n", green_shift);
	printf("Blue-Shift: %u\n", blue_shift);

	uint32_t namelen = ntohl(*(uint32_t *)(buf + 20));
	printf("Name-Len: %u\n", namelen);

	if (namelen > 0) {
		printf("Name(%u): ", namelen);
		readtext(sockfd, namelen);
		printf("\n");
	}

	if (!truecol) {
		printf("Expected TrueColor\n");
		return 3;
	}
	if (bpp != 32) {
		printf("Expected 32 bbp\n");
		return 3;
	}
	if (bigend) {
		printf("BigEndian not supported\n");
		return 3;
	}

	/* Confirm pixel format (optional) */
	bzero(buf, 4);
	writeall(sockfd, buf, 20);

	/* Allow only raw encoding (optional) */
	bzero(buf, sizeof(buf));
	buf[0] = 2;
	buf[3] = 1;
	writeall(sockfd, buf, 8);


	uint32_t *maxBuffer = calloc((uint32_t)width * (uint32_t)heigth, 4);
	if (maxBuffer == NULL) {
		printf("Out of memory\n");
		return 2;
	}
	printf("Allocated %u bytes\n", (uint32_t)width * (uint32_t)heigth * 4);


	/* unbuffered IO will allow to use kbhit/getch */
	setunbuffered();


	/* incremental: do we need a full update */
	uint8_t incremental = 0;

loop_upd:

	bzero(buf, sizeof(buf));
	buf[0] = 3;                             /* FramebufferUpdateRequest */
	buf[1] = incremental;                   /* incremental */
	*(uint16_t *)(buf + 2) = htons(0);      /* x */
	*(uint16_t *)(buf + 4) = htons(0);      /* y */
	*(uint16_t *)(buf + 6) = htons(width);  /* width */
	*(uint16_t *)(buf + 8) = htons(heigth); /* x */
	writeall(sockfd, buf, 10);

loop_msg:
	bzero(buf, sizeof(buf));
	readall(sockfd, buf, 1);
	uint8_t msg = buf[0];
	if (msg == 0) {
		/* fb update */
		readall(sockfd, buf + 1, 3);
	} else if (msg == 1) {
		/* colormap update */
		readall(sockfd, buf + 1, 11);
		/* ... ignored ... */
		goto loop_msg;
	} else if (msg == 2) {
		/* alarm bell */
		printf("BEEP!\n");
		goto loop_msg;
	} else if (msg == 3) {
		readall(sockfd, buf + 1, 7);
		uint32_t textlen = ntohl(*(uint32_t *)(buf + 4)); /* text len */
		printf("Text(%u): ", textlen);
		readtext(sockfd, textlen);
		printf("\n");
		/* ... ignored ... */
		goto loop_msg;
	} else {
		printf("Server sent unsupported msg %u\n", msg);
		return 2;
	}

	uint16_t nr = ntohs(*(uint16_t *)(buf + 2));
	printf("Received %u update frames:\n", nr);
	for (int32_t n = 0; n < (int32_t)nr; n++) {
		bzero(buf, sizeof(buf));
		readall(sockfd, buf, 12);
		uint16_t rx = ntohs(*(uint16_t *)(buf));
		uint16_t ry = ntohs(*(uint16_t *)(buf + 2));
		uint16_t rwidth = ntohs(*(uint16_t *)(buf + 4));
		uint16_t rheigth = ntohs(*(uint16_t *)(buf + 6));
		uint32_t rencoding = ntohl(*(uint16_t *)(buf + 8));
		printf("  (%i) x: [%u-%u], y: [%u-%u], enc: %u\n",
		       n,
		       rx,
		       rwidth,
		       ry,
		       rheigth,
		       rencoding);

		if (rencoding != 0) {
			printf("Server sent unsupported encoding %u\n", rencoding);
			return 2;
		}
		if ((rx + rwidth > width) || (ry + rheigth > heigth)) {
			printf("Server position out of screen\n");
			return 2;
		}

		uint32_t pixels_expected = rwidth * rheigth;
		uint32_t bytes_expected = pixels_expected * 4;

		readall(sockfd, maxBuffer, bytes_expected);

		if ((width == rwidth) && (heigth == rheigth)) {
			incremental = 1;
			shot_nr++;
			char name[64];
			sprintf(name, "screen-%04lu.bmp", shot_nr);
			int ret = writebmp(name, width, heigth, maxBuffer);
			printf("screenshot created: %i\n", ret);
		}
	}

	/* keyevent */
	if (kbhit) {
		uint32_t key_code = getch();
		printf("Send key %u\n", key_code);
		bzero(buf, sizeof(buf));
		buf[0] = 4;
		buf[1] = 1; /* down */
		*(uint32_t *)(buf + 4) = htonl(key_code);
		writeall(sockfd, buf, 8);
		buf[1] = 0; /* up */
		writeall(sockfd, buf, 8);
		key_code = 0;
		incremental = 0;
	}

	/* wait for next update request */
	usleep(100);

	goto loop_upd;
}


int
main(int argc, char *argv[])
{
	int sockfd, ret;
	struct sockaddr_in server_address;

	if (argc < 3) {
		printf("Usage: %s IP-Address Port\n", argv[0]);
		return 0;
	}
	const char *IP = argv[1];
	uint16_t PORT = (uint16_t)atoi(argv[2]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		fprintf(stderr, "Cannot create socket\n");
		return 1; /* failed */
	}
	bzero(&server_address, sizeof(server_address));

	/* Server address and port */
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(IP);
	server_address.sin_port = htons(PORT);

	/* Connect to server */
	if (connect(sockfd, (SA *)&server_address, sizeof(server_address)) != 0) {
		fprintf(stderr, "Cannot connect to server\n");
		return 1; /* failed */
	}

	/* Run communication protocol */
	ret = proto(sockfd);

	/* Close the socket */
	close(sockfd);
	return ret;
}
