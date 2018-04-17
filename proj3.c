#include <stdio.h>

typedef struct{
	unsigned char jmp[3];
	char oem[8];

	unsigned short sector_size;
	unsigned char sectors_per_cluster;
	unsigned short reserved_sectors;
	unsigned char number_of_fats;
	unsigned short root_dir_entries;
	unsigned short total_sectors_short; //if zero, later field is used
	unsigned char media_descriptor;
	unsigned short fat_size_sectors;
	unsigned short sectors_per_track;
	unsigned short number_of_heads;
	unsigned int hidden_sectors;
	unsigned int total_sectors_long;

	unsigned int bpb_FATz32;
	unsigned short bpb_extflags;
	unsigned short bpb_fsver;
	unsigned short bpb_rootcluster;

	char volume_lable[11];
	char fs_type[8];
	char boot_code[436];
	unsigned short boot_sector_signature;
}__attribute((packed)) FAT32BootBlock;

typedef struct{
	unsigned int FSI_LeadSig;
	unsigned char FSI_Reserved1[480];
	unsigned int FSI_StrucSig;
	unsigned int FSI_Free_Count;
	unsigned int FSI_Nxt_Free;
	unsigned char FSI_Reserved2[12];
	unsigned int FSI_TrailSig;
}__attribute((packed)) FSI;

typedef struct{
	unsigned char DIR_Name[11];
	unsigned char DIR_Attr;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LstAccDate;
	unsigned short DIR_FstClusHI;
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO;
	unsigned int DIR_FileSize;
}__attribute((packed)) DIR;

//FUNCTIONS
int info(void);
int exit(void);

//GLOBAL VARIABLES
FILE *file;
struct FAT32BootBlock BPB;
struct FSI FSI;
struct DIR DIR;

int main(int argc, char* argv[]){
	return 0;
}

int info(void){
	//	long offset = BPB32.BPB_FSInfo*BPB32.BPB_BytsPerSec;
	fseek(file, offset, SEEK_SET);
	fread(&FSI,sizeof(struct FSI), 1, file);

	//PRINT STATEMENTS?
	/*
	printf(" Bytes per sector: %d\n", BPB32.BPB_BytsPerSec);
	printf(" Sectors per cluster: %d\n", BPB32.BPB_SecPerClus);
	printf(" Total sectors: %d\n", BPB32.BPB_TotSec32);
	printf(" Number of FATs: %d\n", BPB32.BPB_NumFATs);
	printf(" Sectors per FAT: %d\n", BPB32.BPB_FATSz32);
	printf(" Number of free sectors: %d\n", FSI.FSI_Free_Count);
	*/
	return 0;
}

int exit(void){
	//clear any space up
	exit();
	return 0;
}
