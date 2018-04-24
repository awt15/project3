//Anthony Tieu
//Mustafa Syed
//Vita Tran

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

	/*
	unsigned char	BS_DrvNum;
	unsigned char	BS_Reserved1;
	unsigned char	BS_BootSig;
	unsigned int	BS_VolID;
	unsigned char	BS_VolLab[11];
	unsigned char	BS_FilSysType[8];
	*/
	unsigned int	BPB_FATSz32;
	unsigned short	BPB_ExtFlags;
	unsigned short	BPB_FSVer;
	unsigned int	BPB_RootClus;
	unsigned char	BS_VolLab[11];
	unsigned char	BS_FilSysType[8];
	unsigned char   boot_code[436];
	unsigned short boot_sector_signature;

	//unsigned short	BPB_FSI_info;
	//unsigned short	BPB_BkBootSec;
	//unsigned char	BPB_Reserved[12];	
	//unsigned char	BS_DrvNum;
	//unsigned char	BS_Reserved1;

	//unsigned char	BS_BootSig;
	//unsigned int	BS_VolID;
	//unsigned char	BS_VolLab[11];
	//unsigned char	BS_FilSysType[8];
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
extern struct DIR directory;
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
unsigned int current_cluster_number;

long openedReadFile[100];
long openedWriteFile[100];
long openedFile[100];

struct BPB_32 bpb_32;
FILE *file;

unsigned int FirstDataSector;
unsigned int FirstSectorofCluster;
unsigned int * FAT;

//FUNCTIONS
int info();
int ls(int cluster_number);
int ls_name(char *name);
int cd(char *name);
unsigned int size(char* file);
int create(char *name);
int mkdir(char *name);
int rm(char *name);
int rmdir(char *name);
int rm(char *name);
void open(char *name, char *mode);
void close(char *name);
void readfile();
void writefile();

struct DIR find_file(unsigned int cluster, char *name);
long sector_offset(long sec);
unsigned int FAT_32(unsigned int cluster);
long return_cluster_path(char *string);
unsigned int return_cluster_dir(unsigned int cluster, char *name);
long first_sector_cluster(unsigned int cluster);

int main(int argc, char* argv[])
{
	int i = 0;
	char mode[1];
	char name[13];
	char operation[6];

	writeFileNum = 0;
	readFileNum = 0;
	openedFileNum = 0;
	workingDir = (char*)malloc(200*sizeof(char));
	parentDir = (char*)malloc(200*sizeof(char));

	while(i < 100)
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
			current_cluster_number = bpb_32.BPB_RootClus;

			workingDir[0] = '/';
			workingDir[1] = '\0';		
			parentDir[0] = '\0';
			parentCluster = -1;
			
			FAT = bpb_32.BPB_RsvdSecCnt * bpb_32.BPB_BytsPerSec;

			FirstDataSector = bpb_32.BPB_RsvdSecCnt + (bpb_32.BPB_NumFATs*bpb_32.BPB_FATSz32);
			FirstSectorofCluster = ((bpb_32.BPB_RootClus - 2)*bpb_32.BPB_SecPerClus) + FirstDataSector;

			for(;;)
			{
				printf("%s:%s>", fatImgName, workingDir);
				scanf("%s", operation);

				//FirstDataSector = bpb_32.BPB_RsvdSecCnt + (bpb_32.BPB_NumFATs*bpb_32.BPB_FATSz32);
				//FirstSectorofCluster = ((bpb_32.BPB_RootClus - 2)*bpb_32.BPB_SecPerClus) + FirstDataSector;

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
					//FIGURE A WAY OUT TO DIFFERENTIATE BETWEEN EMPTY/NON-EMPTY
					if (current_cluster_number == 0){
						current_cluster_number = 2;
					}
					ls(current_cluster_number);
					/*scanf("%s", name);
					getchar();
					ls_name(name);*/

				}
				else if (strcmp(operation, "cd") == 0)
				{
					scanf("%s", name);
					getchar();
					cd(name);
				}
				else if (strcmp(operation, "size") == 0)
				{
					scanf("%s", name);
					getchar();
					printf("%d\n", size(name));
				}
				else if (strcmp(operation, "create") == 0)
				{
					scanf("%s", name);
					getchar();
					create(name);
				}
				else if (strcmp(operation, "mkdir") == 0){
					scanf("%s", name);
					getchar();
					mkdir(name);
				}
				else if (strcmp(operation, "rm") == 0){
					scanf("%s", name);
					getchar();
					rm(name);
				}
				else if (strcmp(operation, "rmdir") == 0){
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
				else if (strcmp(operation, "read")==0)
				{
					readfile();
				}
				else if (strcmp(operation, "write")==0)
				{
					writefile();
				}
				else
					printf("Incorrect arguments. Enter exit, info, ls, cd, size, create, mkdir, rm, rmdir, open, close, read, write.\n");			
			}
			return 0;
		}
		else{
			printf("Could not find FAT_32 image.\n");
			return -1;
		}
	}
	else{
		printf("Incorrect number of arguments.\n");
		return -1;
	}
}

int info()
{
	struct FSI BPB_FSI_info;

	//offset = bpb_32.BPB_FSI_info * bpb_32.BPB_BytsPerSec;
	fseek(file, 0x00, SEEK_SET);
	fread(&BPB_FSI_info, sizeof(struct FSI), 1, file);

	//printf("Number of free Sectors: %d\n", BPB_FSI_info.FSI_Free_Count);
	//printf("Sectors per Cluster: %d\n", bpb_32.BPB_SecPerClus);
	//printf("Total Sectors: %d\n", bpb_32.BPB_TotSec32);
	printf("Bytes per Sector: %d\n", bpb_32.BPB_BytsPerSec);
	//printf("Sectors per FAT: %d\n", bpb_32.BPB_FATSz32);
	printf("Number of FATs: %d\n", bpb_32.BPB_NumFATs);
	printf("Root Cluster: %d\n", bpb_32.BPB_RootClus);
	printf("First Data Sector: %x\n", FirstDataSector);
	printf("FAT: %x\n", FAT);
	return 0;
}
//EMPTY LS
int ls(int current_cluster_number)
{				
	struct DIR directory;
	int i;
	int counter = 0;
	long offset;
	long offset_total;
	const int MAX = 15;
	char dir_content[MAX];
	//offset = FirstSectorofCluster * bpb_32.BPB_BytsPerSec;
	//offset_total = offset + bpb_32.BPB_BytsPerSec;
	while (1){
		offset = FirstSectorofCluster * bpb_32.BPB_BytsPerSec;
		fseek(file, offset, SEEK_SET);
		offset_total = offset + bpb_32.BPB_BytsPerSec;
		while (offset < offset_total)
		{
			//printf("offset: %d\n", offset);
			//printf("offset + bpb: %d\n", offset_total);
			fread(&directory, sizeof(struct DIR), 1, file);
			offset += 32;

			/*if(directory.DIR_Name[0] == 0)	continue;
			else if(directory.DIR_Name[0] == 0xE5)	break;
			else if(directory.DIR_Name[0] == 0x5)	directory.DIR_Name[0] = 0xE5;
			*/
			if(directory.DIR_Attr == 0x10 || directory.DIR_Attr == 0x20)
			{
				//printf("TESTING *** [%d]\n",directory.DIR_Attr);
				/*for (int i = 0; i < 10; i++){
					if (directory.DIR_Name[i] != ' '){
						dir_content[i] = directory.DIR_Name[i];
					}
					else{
						dir_content[i] = '\0';
						break;
					}
				}
				dir_content[10] = '\0';
				*/
				while (counter < MAX)
				{
					if (directory.DIR_Name[counter] == ' ')
					{
						dir_content[counter] = '\0';
						dir_content[MAX - 1] = '\0';
						break;
					}
					else
					{
						dir_content[counter] = directory.DIR_Name[counter];
					}
					counter++;
				}
				counter = 0;
				printf("%s\n", dir_content);
				/*for (i = 0; i < MAX; i++)
				{
					dir_content[i] = '\0';

				}*/
				memset(dir_content, 0, sizeof(dir_content));
			}
		}
		printf("TESTING LS: CURRENT CLUSTER NUM: %d\n", current_cluster_number);
		if((FAT_32(current_cluster_number)!= 0x0FFFFFF8) && (FAT_32(current_cluster_number) != 0x0FFFFFFF) && (FAT_32(current_cluster_number) != 0x00000000))
		{
			current_cluster_number = FAT_32(current_cluster_number);
		}	
		else
		{
			break;
		}
	}
	//printf("%s\n", directory.DIR_Name);
	//fread( ,sizeof(int), 1, file);
}
//LS DIRECTORY
int ls_name(char *name)
{
	//Looks up all directories inside the current directory (FSEEK, i*FAT32DirectoryStructureCreatedByYou, i == counter)
	//int i = 0;
	//iterate through while i*FAT32....CreatedByYou < sector_size
	//for(i; (i * FAT32DirectoryStructureCreatedByYou) < sector_size; i++)
	//{
		//When that happens lookup FAT[current_cluster_number]
		//if(FAT[current_cluster_number!=0x0FFFFFF8 || 0x0FFFFFFF || 0x00000000])
			//current_cluster_number = FAT[current_cluster_number]
			//reset loop
		//else
			//break
	//}
}

int cd(char *name)
{
	int i = 0;
	int j = 0;
	char fileName[12];
	struct DIR DIR_entry;
 
	while (name[i] != '\0')
	{
		if (name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		fileName[i] = name[i];
		++i;
	}

	while (i < 11)
	{
		fileName[i] = ' ';
		++i;
	}

	fileName[i] = '\0';
	
	if (strcmp(name, ".") == 0)
	{
		//root?		
	}
	else if (strcmp(name, "/") == 0)
	{
		workingDir[0] = '/';
		workingDir[1] = '\0';
		current_cluster_number = bpb_32.BPB_RootClus;
	}
	else if (strcmp(name, "..") == 0)
	{
		if (strcmp(workingDir, "/") == 0)
		{
			printf("Error! It's in the root directory.\n");
			return -1;
		}
		else
		{	
			while (workingDir[i] != '\0')
			{
				++i;
			}
			while ((i = i -1) && workingDir[i] != '/')
			{
				--i;
			}
			
			if (i == 0)
			{
				workingDir[i + 1] = '\0';
			}
			else
			{
				workingDir[i] = '\0';
			}
			current_cluster_number = return_cluster_path(workingDir);
		}
	}
	else
	{
		DIR_entry = find_file(current_cluster_number, fileName);

		if (DIR_entry.DIR_Name[0] == ENTRY_LAST)
		{
			printf("Error: No directory found!\n");
		}
		else 
		{
			if (DIR_entry.DIR_Attr == 0x10)
			{
				printf("TESTING: CD ELSE SUB\n");
				current_cluster_number = return_cluster_dir(current_cluster_number, fileName);
				printf("TESTING CD: CURRENT CLUSTER NUM: %d\n", current_cluster_number);
				i = 0;
				j = 0;
				while (workingDir[i] != '\0')
				{
					printf("TESTING: LOOP1\n");
					++i;
				}
				printf("WORKING DIR: %s\n", workingDir);

				if (workingDir[i - 1] != '/')
				{
					workingDir[i++] = '/';
				}
				printf("WORKING DIR: %s\n", workingDir);

				while (name[j] != '\0')
				{
					printf("TESTING: LOOP2\n");
					workingDir[i] = name[j];
					++i;
					++j;
				}
				printf("WORKING DIR: %s\n", workingDir);

				workingDir[i] = '\0';
				printf("WORKING DIR: %s\n", workingDir);
			}
			else
			{
				printf("Error: That is not a directory!\n");
			}
		}
	}
	printf("TESTING CD 2: CURRENT CLUSTER NUM: %d\n", current_cluster_number);
	return 0;
}

unsigned int size(char* file)
{
	struct DIR curDIR;
	long offset;
	unsigned int size;
	int sector;
	unsigned int cluster;

	cluster = current_cluster_number;
	while(cluster < END_OF_CLUSTER)
	{
		sector = first_sector_cluster(cluster);
		//printf("sec: %d\n",sector);
		offset = sector * bpb_32.BPB_BytsPerSec;
		//printf("off: %d\n",offset);
		while(offset < (sector * bpb_32.BPB_BytsPerSec + bpb_32.BPB_BytsPerSec*bpb_32.BPB_SecPerClus))
		{
			//curDIR = HAS TO BE WHATEVER THE FILE ITS CHECKING IS I THINK
			if(curDIR.DIR_Name[0] == ENTRY_EMPTY)
			{
				continue;
			}
			else if(curDIR.DIR_Name[0] == ENTRY_LAST)
			{
				break;
			}
			if(curDIR.DIR_Attr != ATTRIBUTE_NAME_LONG)
			{
				if(strcmp(curDIR.DIR_Name, file) == 0)
				{
					size = curDIR.DIR_FileSize;
					return size;
				}
			}
			offset += 32;
		}
		cluster = FAT_32(cluster);
	}
}

int create (char *name)
{

}

int mkdir (char *name)
{

}

int rm (char *name)
{

}

int rmdir (char *name)
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

struct DIR find_file(unsigned int cluster, char *name)
{
	int i;
	long offset;
	char fileName[12];
	struct DIR DIR_entry;

	for(;;)
	{
		offset = sector_offset(FirstSectorofCluster);
		fseek(file, offset, SEEK_SET);

		long temp = offset + bpb_32.BPB_BytsPerSec;
		while (temp >= offset)
		{

			fread(&DIR_entry, sizeof(struct DIR), 1, file);

			if (DIR_entry.DIR_Name[0] == ENTRY_EMPTY)
			{
				continue;
			}
			else if (DIR_entry.DIR_Name[0] == ENTRY_LAST)
			{
				return DIR_entry;
			}
			else if (DIR_entry.DIR_Name[0] == 0x05)
			{
				DIR_entry.DIR_Name[0] = ENTRY_EMPTY;
			}

			if (DIR_entry.DIR_Attr != ATTRIBUTE_NAME_LONG)
			{

				for (i=0; i<11; i++)
				{
					fileName[i]=DIR_entry.DIR_Name[i];
				}

				fileName[11]= '\0';

				if (strcmp(fileName, name) == 0)
				{
					return DIR_entry;
				}
			}
			offset += 32;
		}

		cluster = FAT_32(cluster);

		if (END_OF_CLUSTER < cluster)
		{
			break;
		}
	}
}

long sector_offset(long sec)
{
	return (bpb_32.BPB_BytsPerSec * sec);
}

unsigned int FAT_32(unsigned int cluster)
{
	unsigned int nextCluster;
	long offset;

	offset = bpb_32.BPB_RsvdSecCnt*bpb_32.BPB_BytsPerSec + cluster*4;
	fseek(file, offset, SEEK_SET);
	fread(&nextCluster, sizeof(unsigned int), 1, file);

	return nextCluster;
}

long return_cluster_path(char *string){
	int i = 1;
	int j = 0;
	long cluster;
	unsigned char name[11];
	cluster = bpb_32.BPB_RootClus;

	for(;;){
		for (i; ; i++){

			if (string[i] != '/' && string[i] != '\0'){
				name[j] = string[i];
			}
			else{
				name[j] = '\0';
				break;
			}		
		}

		if (strcmp(name, "") != 0)
			cluster = return_cluster_dir(cluster, name);
		else
			break;
	}
	return cluster;
}

unsigned int return_cluster_dir(unsigned int cluster, char *name){
	int i = 0;
	char fileName[12];
	long offset;
	struct DIR DIR_entry;

	for(;;)
	{
		offset = sector_offset(FirstSectorofCluster);
		//printf("TESTING OFFSET: %d\n", offset);
		fseek(file, offset, SEEK_SET);


		//printf("TESTING: Return DIR\n");
		long temp = sector_offset(FirstSectorofCluster + bpb_32.BPB_SecPerClus);
		//long temp = FirstSectorofCluster*bpb_32.BPB_BytsPerSec + bpb_32.BPB_BytsPerSec*bpb_32.BPB_SecPerClus;
		//printf("TESTING TEMP: %d\n", temp);
		while ( offset < temp )
		{
			printf("TESTING: While Loop DIR\n");
			fread(&DIR_entry, sizeof(struct DIR), 1, file);
			offset+=32;
			printf("TESTING TEMP: %d\n", temp);
			printf("TESTING OFFSET: %d\n", offset);
			if (DIR_entry.DIR_Name[0] == ENTRY_EMPTY)
				continue;
			else if (DIR_entry.DIR_Name[0] == ENTRY_LAST)
				break;
			else if (DIR_entry.DIR_Name[0] == 0x05)
				DIR_entry.DIR_Name[0] = ENTRY_EMPTY;

			if (DIR_entry.DIR_Attr != ATTRIBUTE_NAME_LONG){
				printf("TEST DIR NAME: %s\n", DIR_entry.DIR_Name);
				printf("TEST FILE NAME: %s\n", fileName);
				for (i = 0; i < 11; i++)
				{
					fileName[i] = DIR_entry.DIR_Name[i];
					printf("CHAR FILE: %c\n", fileName[i]);
					printf("DIR NAME: %c\n",DIR_entry.DIR_Name[i]);
				}	


				fileName[11] = '\0';
			}

			if ((strcmp(fileName, name) == 0) && DIR_entry.DIR_Attr == 0x10)
			{		
				printf("fileName: %s\n", fileName);
				printf("name: %s\n", name);
				return (DIR_entry.DIR_FstClusHI *0x100 + DIR_entry.DIR_FstClusLO);
			}
			printf("TESTING: End of while loop.\n");
		}
		printf("TESTING: AFTER while loop.\n");
		cluster = FAT_32(cluster);
		if((cluster == 0x0FFFFFF8) || (cluster == 0x0FFFFFFF) || (cluster == 0x00000000))
		{
			break;
		}	
		/*
		cluster = FAT_32(cluster);
		if((cluster != 0x0FFFFFF8) && (cluster != 0x0FFFFFFF) && (cluster != 0x00000000))
		{
			current_cluster_number = cluster;
		}	
		else
		{
			break;
		}
		*/
	}
}

long first_sector_cluster(unsigned int cluster)
{
	return ( (cluster - 2) * bpb_32.BPB_SecPerClus + bpb_32.BPB_RsvdSecCnt + bpb_32.BPB_FATSz32 * 2);
}