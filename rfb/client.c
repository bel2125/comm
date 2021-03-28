#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../timing.h"
#define MAX 256
#define PORT 5900
#define SA struct sockaddr

#include "bmp.c"

static unsigned long shot_nr = 0;

void readall(int sockfd, void *buf, uint32_t len)
{
	uint32_t readb = 0;
	while (readb < len) {
		int s = read(sockfd, buf+readb, len-readb);
		if (s<0) {
			fprintf(stderr, "Read error: %s\n", strerror(errno));
			exit(1);
		}
		readb += s;
	}
}


void writeall(int sockfd, const void *buf, uint32_t len)
{
	uint32_t written = 0;
	while (written < len) {
		int s = write(sockfd, buf+written, len-written);
		if (s<0) {
			fprintf(stderr, "Write error: %s\n", strerror(errno));
			exit(1);
		}
		written += s;
	}
}


void readtext(int sockfd, uint32_t len)
{
	char *r = malloc(len+1);
	if (!r) {
		fprintf(stderr, "Out of memory (%u)\n", len+1);
		exit(1);
	}
	readall(sockfd, r, len);
	r[len]=0;
	printf("%s", r);
	free(r);
}


int proto(int sockfd)
{
    char buf[MAX];
    int n;

    bzero(buf, sizeof(buf));
	readall(sockfd, buf, 12);
    printf("Server proto: %s\n", buf);
	if ((0!=memcmp(buf, "RFB 003.", 8)) || (buf[11]!='\n')) {
		printf("Version not supported\n");
		return 2;
	}

    bzero(buf, sizeof(buf));
	sprintf(buf, "RFB 003.003\n");
	writeall(sockfd, buf, 12);

    bzero(buf, sizeof(buf));
	readall(sockfd, buf, 4);
    printf("Server sec: %u\n", ntohl(*(uint32_t*)buf));

    bzero(buf, sizeof(buf));
	buf[0] = 0; /* shared? */
	writeall(sockfd, buf, 1);

    bzero(buf, sizeof(buf));
    readall(sockfd, buf, 24);

	uint16_t width = ntohs(*(uint16_t*)(buf));
	uint16_t heigth = ntohs(*(uint16_t*)(buf+2));
    printf("Width: %u\n", width);
    printf("Heigth: %u\n", heigth);

	uint8_t bpp = *(uint8_t*)(buf+4);
	uint8_t depth = *(uint8_t*)(buf+5);
    printf("BPP: %u\n", bpp);
    printf("Depth: %u\n", depth);

	uint8_t bigend = !!*(uint8_t*)(buf+6);
	uint8_t truecol = !!*(uint8_t*)(buf+7);
    printf("Big-Endian: %u\n", bigend);
    printf("TrueColor: %u\n", truecol);

	uint16_t red_max = ntohs(*(uint16_t*)(buf+8));
	uint16_t green_max = ntohs(*(uint16_t*)(buf+10));
	uint16_t blue_max = ntohs(*(uint16_t*)(buf+12));
    printf("Red-Max: %u\n", red_max);
    printf("Green-Max: %u\n", green_max);
    printf("Blue-Max: %u\n", blue_max);

	uint8_t red_shift = *(uint8_t*)(buf+14);
	uint8_t green_shift = *(uint8_t*)(buf+15);
	uint8_t blue_shift = *(uint8_t*)(buf+16);
    printf("Red-Shift: %u\n", red_shift);
    printf("Green-Shift: %u\n", green_shift);
    printf("Blue-Shift: %u\n", blue_shift);

	uint32_t namelen = ntohl(*(uint32_t*)(buf+20));
    printf("Name-Len: %u\n", namelen);

    if (namelen>0) {
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

	uint32_t *maxBuffer = calloc((uint32_t)width*(uint32_t)heigth, 4);
	if (maxBuffer == NULL) {
		printf("Out of memory\n");
		return 2;
	}
	printf("Allocated %u bytes\n", (uint32_t)width*(uint32_t)heigth * 4);

	uint8_t incremental = 0;

loop_upd:

	bzero(buf, sizeof(buf));
	buf[0] = 3; /* FramebufferUpdateRequest */
	buf[1] = incremental; /* incremental */
	*(uint16_t*)(buf+2) = htons(0); /* x */
	*(uint16_t*)(buf+4) = htons(0); /* y */
	*(uint16_t*)(buf+6) = htons(width); /* width */
	*(uint16_t*)(buf+8) = htons(heigth); /* x */
	writeall(sockfd, buf, 10);

loop_msg:
	bzero(buf, sizeof(buf));
	readall(sockfd, buf, 1);
	uint8_t msg = buf[0];
	if (msg == 0) {
		/* fb update */
		readall(sockfd, buf+1, 3);
	} else if (msg == 1) {
		/* colormap update */
		readall(sockfd, buf+1, 11);
		/* ... ignored ... */
		goto loop_msg;
	} else if (msg == 2) {
		/* alarm bell */
		printf("BEEP!\n");
		goto loop_msg;
	} else if (msg == 3) {
		readall(sockfd, buf+1, 7);
		uint32_t textlen = ntohl(*(uint32_t*)(buf+4)); /* text len */
		printf("Text(%u): ", textlen);
		readtext(sockfd, textlen);
		printf("\n");
		/* ... ignored ... */
		goto loop_msg;
	} else {
		printf("Server sent unsupported msg %u\n", msg);
		return 2;
	}

	uint16_t nr = ntohs(*(uint16_t*)(buf+2));
	printf("Received %u update frames:\n", nr);
	for (int32_t n = 0; n<(int32_t)nr; n++) {
		bzero(buf, sizeof(buf));
		readall(sockfd, buf, 12);
		uint16_t rx = ntohs(*(uint16_t*)(buf));
		uint16_t ry = ntohs(*(uint16_t*)(buf+2));
		uint16_t rwidth = ntohs(*(uint16_t*)(buf+4));
		uint16_t rheigth = ntohs(*(uint16_t*)(buf+6));
		uint32_t rencoding = ntohl(*(uint16_t*)(buf+8));
		printf("  (%i) x: [%u-%u], y: [%u-%u], enc: %u\n", n, rx, rwidth, ry, rheigth, rencoding);
		
		if (rencoding != 0) {
			printf("Server sent unsupported encoding %u\n", rencoding);
			return 2;
		}
		if ((rx+rwidth > width) || (ry+rheigth > heigth)) {
			printf("Server position out of screen\n");
			return 2;
		}

		uint32_t pixels_expected = rwidth*rheigth;
		uint32_t bytes_expected = pixels_expected*4;

		readall(sockfd, maxBuffer, bytes_expected);
		
		if ((width==rwidth) && (heigth==rheigth)) {
			incremental = 1;
			shot_nr++;
			char name[64];
			sprintf(name, "screen-%04lu.bmp", shot_nr);
			int ret = writebmp(name, width, heigth, maxBuffer);
			printf("screenshot created: %i\n", ret);
		}
	}
	
	/* wait for next update request */
	usleep(100);
	

	goto loop_upd;
}


int main(int argc, char *argv[])
{
    int sockfd, ret;
    struct sockaddr_in server_address;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "Cannot create socket\n");
        return 1; /* failed */
    }
    bzero(&server_address, sizeof(server_address));

    /* Server address and port */
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("192.168.0.103");
    server_address.sin_port = htons(PORT);

    /* Connect to server */
    if (connect(sockfd, (SA*)&server_address, sizeof(server_address)) != 0) {
        fprintf(stderr, "Cannot connect to server\n");
        return 1; /* failed */
	}

    /* Run communication protocol */
    ret = proto(sockfd);

    /* Close the socket */
    close(sockfd);
	return ret;
}
