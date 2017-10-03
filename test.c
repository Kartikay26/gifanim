#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned char BYTE;
typedef unsigned short int TWOBYTE;

typedef struct gifHeader{
	char signature[6];
	TWOBYTE width;
	TWOBYTE height;
	// unsigned gct_flag:1;
	// unsigned color_res:3;
	// unsigned sort_flag:1;
	// unsigned gct_size:3;
	BYTE packed_data;
	BYTE bgcolor;
	BYTE pixel_aspect_ratio;
} __attribute__((packed)) gifHeader;

typedef struct ct_color
{
	BYTE r;
	BYTE g;
	BYTE b;
} ct_color;

void writeoutinbinary(BYTE x);
int readpackedfield(BYTE packed_field, int n, int m);

// then the color table 3*2^(pix)

// NOTE FREAD SYNTAX
// fread(buffer, numbytes=sizeof(x), count=1, fptr);

// int main(){
// 	int x;
// 	scanf("%d",&x);
// 	writeoutinbinary(x);
// 	x = readpackedfield(x,1,3);
// 	printf("\n");
// 	writeoutinbinary(x);
// 	printf("\n");
// }

int main(){
	char filename[] = "test.gif";
	FILE* fptr;
	fptr = fopen(filename,"rb");
	if (fptr == NULL)
	{
		printf("Error, Cannot open file.\n");
		exit(1);
	}

	gifHeader header;
	fread(&header, sizeof(gifHeader), 1, fptr);
	printf("SIGNATURE: %c%c%c%c%c%c\n", header.signature[0],
			header.signature[1],
			header.signature[2],
			header.signature[3],
			header.signature[4],
			header.signature[5]);
	//printf("width: %d\n", header.width);
	//printf("height: %d\n", header.height);
	int * image_data = (int*) malloc(header.width*header.height);
	//printf("packed_data: \n");
	//printf("GCT flag: %d\n", readpackedfield(header.packed_data,7,1));
	//printf("color res: %d bits\n", readpackedfield(header.packed_data,4,3) + 1);
	//printf("sort flag: %d\n", readpackedfield(header.packed_data,3,1));
	int gct_colors = round((pow(2,readpackedfield(header.packed_data,0,3) +1)));
	//printf("GCT size: %d colors\n", gct_colors);
	//printf("pixel_aspect_ratio: %d\n", header.pixel_aspect_ratio);
	if(!readpackedfield(header.packed_data,7,1)){printf("Error, no GCT\n"); return 1;}
	
	ct_color* globalcolortable = (ct_color*) malloc(gct_colors*sizeof(ct_color));
	ct_color* localcolortable;
	
	fread(globalcolortable, sizeof(ct_color), gct_colors, fptr);
	printf("Read %d colors into GCT\n",gct_colors);

	int frames = 0;
	float total_time = 0;
	int total_data=0;

	unsigned char ch;
	while((ch=getc(fptr))!=EOF)
	{
		 if (ch == 0x21) {
			//printf("checking next char ...\n");
			ch = getc(fptr);
			if(ch==0xf9){
				printf("Graphics control block\n");
				BYTE block_size;
				BYTE packed_field;
				TWOBYTE delaytime;
				BYTE transparentcolor;
				block_size = getc(fptr);
				packed_field = getc(fptr);
				//printf("Tranparency: %d\n",readpackedfield(packed_field,0,1));
				//printf("UserInput: %d\n",readpackedfield(packed_field,1,1));
				//printf("DisposalMethod: %d\n",readpackedfield(packed_field,2,3));
				fread(&delaytime, sizeof(delaytime), 1, fptr);
				transparentcolor = getc(fptr);
				//printf("block size: %d\n", block_size);
				printf("delaytime: %d\n", delaytime);
				total_time += delaytime/100.0;
				if(getc(fptr)!=0x00)printf("coud not find block Terminator.\n");
				//printf("Block terminated.\n");
			} else if(ch==0xfe){
				printf("Comment SECTION\n");
				BYTE block_size = getc(fptr);
				//printf("Comment Data: \t\"");
				fseek(fptr, block_size, SEEK_CUR);
				//for (int i = 0; i < block_size; ++i)
				//{
					//printf("%c", (getc(fptr)));
				//}
				//printf("\"\n");
				if(getc(fptr)!=0x00)printf("coud not find block Terminator.\n");
				//printf("Block terminated.\n");
			} else if(ch==0xff){
				printf("APPLICATION extension\n");
				BYTE block_size = getc(fptr);
				printf("header Block size is %u\n",block_size);
				fseek(fptr, block_size, SEEK_CUR);
				//printf("Application name is \t");
				//for (int i = 0; i < block_size; ++i)
				//{
					//printf("%c", (getc(fptr)));
				//}
				//printf("\nSkipping application data now\n");
				while(block_size = getc(fptr))
				fseek(fptr, block_size, SEEK_CUR);
			} else {
				printf("CANT READ\n");
			}
		} else if(ch == 0x2C) {
			printf("IMAGE DESCRIPTOR BLOCK\n");
			frames++;
			TWOBYTE left; fread(&left,2,1,fptr);
			// printf("left %d\n",left);
			TWOBYTE top;fread(&top,2,1,fptr);
			// printf("top %d\n", top);
			TWOBYTE width;fread(&width,2,1,fptr);
			// printf("width %d\n", width);
			TWOBYTE height;fread(&height,2,1,fptr);
			// printf("height %d\n", height);
			BYTE packfield_3 = getc(fptr);
			int LCT_size = readpackedfield(packfield_3,0,3);
			int sort_flag = readpackedfield(packfield_3,5,1);
			int interlace_flag = readpackedfield(packfield_3,6,1);
			int LCT_flag = readpackedfield(packfield_3,7,1);
			if(LCT_flag){
				int lct_colors = round((pow(2,LCT_size+1)));
				localcolortable = (ct_color*) malloc(lct_colors*sizeof(ct_color));
				fread(localcolortable, sizeof(ct_color), lct_colors, fptr);
				printf("Read %d colors into Local Color Table\n",lct_colors);
			}
			if(interlace_flag){
				printf("Interlacing not yet implemented.\n");
				return 1;
			}
			printf("Reading image data ... ");
			BYTE Lzw_codesize = getc(fptr);
			//printf("LZW Code size: %d\n",Lzw_codesize);
			//printf("Data follows\n");
			BYTE block_size;
			int totaldatasize = 0;
			while(block_size = getc(fptr)){
				//printf("Read block of size %d\n", block_size);
				fseek(fptr, block_size, SEEK_CUR);
				totaldatasize += block_size;
				total_data+=totaldatasize;
			}
			printf("Total data read in data block %d bytes.\n", totaldatasize);
			//printf("Found block terminator\n");
		} else if(ch == 0x3B) {
			printf("FILE Terminator found. File ended. \n");
			printf("Total number of frames read = %d.\n", frames);
			printf("Total data read = %d bytes.\n", total_data);
			printf("Total animation time = %f sec.\n", total_time);
			return 0;
		} else{
			printf("Kuch toh garbar hai ERRCODE: 0x%02x \n", ch);
			return 1;
		}
	}
	free(image_data);
	free(globalcolortable);
	free(localcolortable);
	fclose(fptr);
	exit(0);
}

int readpackedfield(BYTE packed_field, int n, int m){
	// read bits from a packed field
	// read bit n from lsb (n=0 is lsb) and read m bits
	// returns int
	int x = packed_field;
	int ans = 0;
	int bit_no = n;
	for(int i=n; i<n+m; i++){
		ans += (x&(1<<i))>>n;
	}
	return ans;
}

void writeoutinbinary(BYTE x){
	int i=7;
	while(i>=0)
	{
		printf("%d", ((x&(1<<i))>>i)); // print i'th bit of x
		i--;
	}
}