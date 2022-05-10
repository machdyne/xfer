/*
 * xfer: serial file transfer program
 * Copyright (c) 2021 Lone Dynamics Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define PACKET_DATA_MAX 250

uint32_t crc32b(unsigned char *data, uint32_t len);
void send_file(char *filename);
void send_packet(char cmd, char *data, uint8_t len);
int get_ack(void);

int packets = 0;
int acks = 0;
int nacks = 0;
size_t bytes_total = 0;

int main(int argc, char **argv) {

	fprintf(stderr, "xfer sending file\n");

	char buf[256];

	if (argc < 2) {
		fprintf(stderr, "usage: xfer <file>\n");
		return(1);
	}

	send_file(argv[1]);

}

void cmd_load(char *data, uint32_t len) { send_packet('L', data, len); }
//void cmd_jump(uint32_t addr) { send_packet('J', (char *)&addr, 4); }
//void cmd_boot(uint32_t addr) { send_packet('B', (char *)&addr, 4); }

void send_file(char *filename) {
	FILE *f;
	size_t bytes_read;
	size_t filesize;
	char buf[PACKET_DATA_MAX];
	f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr, "unable to open file\n");
		return;
	}
	fseek(f, 0L, SEEK_END);
	filesize = ftell(f);
	rewind(f);

	while ((bytes_read = fread(buf, 1, PACKET_DATA_MAX, f)) != 0) {
		++packets;
		do {
			fprintf(stderr, "[%li / %li] ", bytes_total + bytes_read, filesize);
			cmd_load(buf, bytes_read);
		} while (!get_ack());
		bytes_total += bytes_read;
	}
	fclose(f);

	printf("D");
	fflush(stdout);
}

void send_packet(char cmd, char *data, uint8_t len) {

	char packet[2+PACKET_DATA_MAX+4];
	uint32_t crc;

	bzero(packet, 2+PACKET_DATA_MAX+4);

	packet[0] = cmd;
	packet[1] = len;

	memcpy(&packet[2], data, len);

	crc = crc32b(packet, 2 + len);

	memcpy(&packet[2 + len], (uint32_t *)&crc, 4);

	fwrite((void *)packet, 1, 2 + len + 4, stdout);
	fflush(stdout);

}

int get_ack(void) {
	int c;
	if ((c = getchar()) != EOF) {
		if (c == 'A') {
			++acks;
			fprintf(stderr, "packets: %i acks: %i nacks: %i\n",
				packets, acks, nacks);
			return 1;
		} else {
			++nacks;
			fprintf(stderr, "packets: %i acks: %i nacks: %i\n",
				packets, acks, nacks);
			return 0;
		}
	}
}

uint32_t crc32b(unsigned char *data, uint32_t len) {

	int i, j;
	uint32_t byte, crc, mask;

	crc = 0xffffffff;
	for (int i = 0; i < len; i++) {
		byte = data[i];
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xedb88320 & mask);
		}
		i = i + 1;
	}
	return ~crc;
}
