//Anthony Tieu
//Mustafa Syed
//Vita Tran

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define END_OF_CLUSTER 0x0FFFFFF8
#define ATTRIBUTE_NAME_LONG 0x0F
#define ENTRY_LAST 0x00
#define ENTRY_EMPTY 0xE5
#define ATTR_CONST 0x20
#define CLUSTER_END 0xFFFFFFFF
#define OFFSET_CONST 32

struct BPB_32
{
	unsigned char	BS_jmpBoot[3];
	unsigned char	BS_OEMName[8];
	unsigned short	BPB_BytsPerSec;
	unsigned char	BPB_SecPerClus;
	unsigned short	BPB_RsvdSecCnt;

	unsigned char	BPB_NumFATs;
	unsigned short	BPB_RootEntCnt;
	unsigned short	BPB_TotSec16;
	unsigned char	BPB_Media;
	unsigned short	BPB_FATSz16;
	unsigned short	BPB_SecPerTrk;
	unsigned short	BPB_NumHeads;
	unsigned int	BPB_HiddSec;
	unsigned int	BPB_TotSec32;

	unsigned char	BS_DrvNum;
	unsigned char	BS_Reserved1;
	unsigned char	BS_BootSig;
	unsigned int	BS_VolID;
	unsigned char	BS_VolLab[11];
	unsigned char	BS_FilSysType[8];

	unsigned int	BPB_FATSz32;
	unsigned short	BPB_ExtFlags;
	unsigned short	BPB_FSVer;
	unsigned int	BPB_RootClus;
	unsigned short	BPB_FSI_info;
	unsigned short	BPB_BkBootSec;
	unsigned char	BPB_Reserved[12];	
} __attribute__((packed));

struct FSI
{
	unsigned int FSI_LeadSig;
	unsigned char FSI_Reserved1[480];
	unsigned int FSI_StrucSig;
	unsigned int FSI_Free_Count;
	unsigned int FSI_Nxt_Free;
	unsigned char FSI_Reserved2[12];
	unsigned int FSI_TrailSig;
}__attribute((packed));

struct DIR
{
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
}__attribute((packed));


//GLOBAL VARIABLES
extern struct FSI BPB_FSI_info;
extern struct BPB_32 bpb_32;
extern long openedFile[100];
extern int openedFileNum;
extern FILE *file;

//MORE GLOBALS
char *fatImgName;
char *parentDir;
char *workingDir;

int openedFileNum;
int readFileNum;
int writeFileNum;

unsigned int parentCluster;
unsigned int currCluster;

long openedReadFile[100];
long openedWriteFile[100];
long openedFile[100];

struct BPB_32 bpb_32;
FILE *file;

//FUNCTIONS
int info();
int ls(char *name);
int cd(char *name);
int size();
int create(char *name);
int mkdir(char *name);
int rm(char *name);
int rmdir(char *name);
int rm(char *name);
void open(char *name, char *mode);
void close(char *name);
void readfile();
void writefile();

int main(int argc, char* argv[])
{
	int i = 0;
	char mode[1];
	char name[13];
	char operation[6];

	writeFileNum = 0;
	readFileNum = 0;
	openedFileNum = 0;
	workingDir = (char*)malloc(200 * sizeof(char));
	parentDir = (char*)malloc(200 * sizeof(char));

	while (i < 100)
	{
		openedFile[i] = 0;
		openedReadFile[i] = 0;
		openedWriteFile[i] = 0;
		++i;
	}

	if (argc == 2)
	{
		if (file = fopen(argv[1], "rb+"))
		{
			fread(&bpb_32, sizeof(struct BPB_32), 1, file);

			fatImgName = argv[1];
			currCluster = bpb_32.BPB_RootClus;

			workingDir[0] = '/';
			workingDir[1] = '\0';
			parentDir[0] = '\0';
			parentCluster = -1;

			for (;;)
			{
				printf("%s:%s>", fatImgName, workingDir);
				scanf("%s", operation);

				if (strcmp(operation, "exit") == 0)
				{
					fclose(file);
					break;
				}
				else if (strcmp(operation, "info") == 0)
				{
					info();
				}
				else if (strcmp(operation, "ls") == 0)
				{
					scanf("%s", name);
					getchar();
					ls(name);
				}
				else if (strcmp(operation, "cd") == 0)
				{
					scanf("%s", name);
					getchar();
					cd(name);
				}
				else if (strcmp(operation, "size") == 0)
				{
					size();
				}
				else if (strcmp(operation, "create") == 0)
				{
					scanf("%s", name);
					getchar();
					create(name);
				}
				else if (strcmp(operation, "mkdir") == 0) {
					scanf("%s", name);
					getchar();
					mkdir(name);
				}
				else if (strcmp(operation, "rm") == 0) {
					scanf("%s", name);
					getchar();
					rm(name);
				}
				else if (strcmp(operation, "rmdir") == 0) {
					scanf("%s", name);
					getchar();
					rmdir(name);
				}
				else if (strcmp(operation, "open") == 0)
				{
					scanf("%s", name);
					scanf("%s", mode);
					getchar();
					open(name, mode);
				}
				else if (strcmp(operation, "close") == 0)
				{
					scanf("%s", name);
					getchar();
					close(name);
				}
				else if (strcmp(operation, "read") == 0)
				{
					readfile();
				}
				else if (strcmp(operation, "write") == 0)
				{
					writefile();
				}
				else
					printf("Incorrect arguments. Enter exit, info, ls, cd, size, create, mkdir, rm, rmdir, open, close, read, write.\n");
			}
			return 0;
		}
		else {
			printf("Could not find FAT_32 image.\n");
			return -1;
		}
	}
	else {
		printf("Incorrect number of arguments.\n");
		return -1;
	}
}

int info()
{
	long offset;
	struct FSI BPB_FSI_info;

	offset = bpb_32.BPB_FSI_info * bpb_32.BPB_BytsPerSec;
	fseek(file, 0x00000000, SEEK_SET);
	fread(&BPB_FSI_info, sizeof(struct FSI), 1, file);

	printf("Number of free Sectors: %d\n", BPB_FSI_info.FSI_Free_Count);
	printf("Sectors per Cluster: %d\n", bpb_32.BPB_SecPerClus);
	printf("Total Sectors: %d\n", bpb_32.BPB_TotSec32);
	printf("Bytes per Sector: %d\n", bpb_32.BPB_BytsPerSec);
	printf("Sectors per FAT: %d\n", bpb_32.BPB_FATSz32);
	printf("Number of FATs: %d\n", bpb_32.BPB_NumFATs);

	return 0;
}

int ls(char *name)
{
	//Looks up all directories inside the current directory (FSEEK, i*FAT32DirectoryStructureCreatedByYou, i == counter)
	int i = 0;
	//iterate through while i*FAT32....CreatedByYou < sector_size
	for(i; (i * FAT32DirectoryStructureCreatedByYou) < sector_size; i++)
	{
		//When that happens lookup FAT[current_cluster_number]
		//if(FAT[current_cluster_number!=0x0FFFFFF8 || 0x0FFFFFFF || 0x00000000])
			//current_cluster_number = FAT[current_cluster_number]
			//reset loop
		//else
			//break
	}
}

int cd(char *name)
{

}

int size()
{

}

int create(char *name)
{

}

int mkdir(char *name)
{

}

int rm(char *name)
{

}

int rmdir(char *name)
{

}

void open(char *name, char *mode)
{

}

void close(char *name)
{

}

void readfile()
{

}

void writefile()
{

}
