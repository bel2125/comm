
#include <stdio.h>

struct tbmpheader {
	char B;
	char M;
	unsigned char file_size_LE[4];
	char zero_1[4];
	char offs_54;
	char zero_2[3];
	char len_40;
	char zero_3[3];
	unsigned char width_LE[4];
	unsigned char height_LE[4];
	char plane_1;
	char zero_4[1];
	char colordepth;
	char zero_5[5];
	unsigned char raw_bytes_LE[4];	
	char zero_6[16];
};


int writebmp(const char *filename, uint32_t width, uint32_t height, uint32_t *pixels)
{
	FILE *f = fopen(filename, "w");
	if (!f) {
		return -1;
	}
	
	struct tbmpheader bmpheader;
	memset(&bmpheader, 0, sizeof(bmpheader));
	bmpheader.B = 'B';
	bmpheader.M = 'M';
	bmpheader.offs_54 = 54;
	bmpheader.len_40 = 40;
	bmpheader.plane_1 = 1;
	
	bmpheader.colordepth = 32; /* support 32 bit colors only */
	bmpheader.width_LE[0] = (unsigned char)(width >> 0);
	bmpheader.width_LE[1] = (unsigned char)(width >> 8);
	bmpheader.width_LE[2] = (unsigned char)(width >> 16);
	bmpheader.width_LE[3] = (unsigned char)(width >> 24);
	bmpheader.height_LE[0] = (unsigned char)(height >> 0);
	bmpheader.height_LE[1] = (unsigned char)(height >> 8);
	bmpheader.height_LE[2] = (unsigned char)(height >> 16);
	bmpheader.height_LE[3] = (unsigned char)(height >> 24);
	
	uint32_t rawsize = width * height * 4;
	bmpheader.raw_bytes_LE[0] = (unsigned char)(rawsize >> 0);
	bmpheader.raw_bytes_LE[1] = (unsigned char)(rawsize >> 8);
	bmpheader.raw_bytes_LE[2] = (unsigned char)(rawsize >> 16);
	bmpheader.raw_bytes_LE[3] = (unsigned char)(rawsize >> 24);
	
	uint32_t filesize = rawsize + sizeof(bmpheader);
	bmpheader.file_size_LE[0] = (unsigned char)(filesize >> 0);
	bmpheader.file_size_LE[1] = (unsigned char)(filesize >> 8);
	bmpheader.file_size_LE[2] = (unsigned char)(filesize >> 16);
	bmpheader.file_size_LE[3] = (unsigned char)(filesize >> 24);
	
	fwrite(&bmpheader, sizeof(bmpheader), 1, f);
	
	uint32_t h = height;
	while (h>0) {
		h--;
		fwrite(pixels+h*width, width, 4, f);
	}
	
	fclose(f);
	return 0;
}

