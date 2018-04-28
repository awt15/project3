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

void change_val_cluster(unsigned int value, unsigned int cluster);
void empty_val_cluster(unsigned int cluster);

int equals(unsigned char name1[], unsigned char name2[]);
int unopened(long offset);

unsigned int FAT_32(unsigned int cluster);
unsigned int return_cluster_dir(unsigned int cluster, char *name);

long sector_offset(long sec);
long first_sector_cluster(unsigned int cluster);
long return_offset(unsigned int cluster, char *name);
long return_cluster_path(char *string);

long empty_cluster();


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
					if (current_cluster_number == 0)
					{
						current_cluster_number = 2;
					}

					fgets(name, sizeof(name), stdin);
					if(sscanf(name,"%s\n", name) != 1)
					{
						ls(current_cluster_number);
					}
					else
					{
						ls_name(name);
					}
				}
				else if (strcmp(operation, "cd") == 0)
				{
					scanf("%s", name);
					if(strcmp(name,"..") != 0)
						parentCluster = current_cluster_number;
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

//MAIN FUNCTIONS
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

	while (1){
		offset = first_sector_cluster(current_cluster_number) * bpb_32.BPB_BytsPerSec;
		offset_total = offset + bpb_32.BPB_BytsPerSec;
		fseek(file, offset, SEEK_SET);
		while (offset < offset_total)
		{
			fread(&directory, sizeof(struct DIR), 1, file);
			offset += OFFSET_CONST;
			if(directory.DIR_Attr == 0x10 || directory.DIR_Attr == 0x20)
			{
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
				memset(dir_content, 0, sizeof(dir_content));
			}
		}
		if((FAT_32(current_cluster_number)!= 0x0FFFFFF8) && (FAT_32(current_cluster_number) != 0x0FFFFFFF) && (FAT_32(current_cluster_number) != 0x00000000))
		{
			current_cluster_number = FAT_32(current_cluster_number);
		}	
		else
		{
			break;
		}	
	}
}
//LS DIRECTORY
int ls_name(char *name)
{
	cd(name);
	ls(current_cluster_number);
	cd("..");
	return 0;
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
		printf("TESTING WE IN HERE 2\n");
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
			printf("TESTING CLUSTER NUMBER BEFORE %d\n", current_cluster_number);
			printf("TESTING WORKING DIR: %s\n", workingDir);
			current_cluster_number = return_cluster_path(workingDir);
			//current_cluster_number = parentCluster;
			printf("TESTING CLUSTER NUMBER AFTER %d\n", current_cluster_number);
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
				current_cluster_number = return_cluster_dir(current_cluster_number, fileName);
				i = 0;
				j = 0;
				while (workingDir[i] != '\0')
				{
					++i;
				}

				if (workingDir[i - 1] != '/')
				{
					workingDir[i++] = '/';
				}

				while (name[j] != '\0')
				{
					workingDir[i] = name[j];
					++i;
					++j;
				}

				workingDir[i] = '\0';
			}
			else
			{
				printf("Error: That is not a directory!\n");
			}
		}
	}
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
	int ec_number = empty_cluster();
	printf("THIS IS EMPTY CLUSTER: %d\n", ec_number);

	if(ec_number != 1)
	{
		//SET FAT[i] = 0X0FFFFFF8
		//SET FAT[current_cluster number = hexcode of 'i'

		//fwrite, set attr to 0x10
		//dir_clusHI = ec_number/0x100
		//dir_clusLO = ec_number%0x100
		//dir_NTRes = 0
		//dir_FileSize = 0
		//do .. and .
	}
}

int rm (char *name)
{
	int i = 0;
	int j = 0;
	int temp;
	long offset;
	unsigned int nextCluster;
	char fileName[12];
	char empty[32];
	struct DIR DIR_entry;

	while(name[i] != '\0')
	{
		if(name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		i++;
	}
	i = 0;
	while(i < 8)
	{
		if(name[j] != '\0' && name[j] != '.')
		{
			fileName[i] = name[j];
			i++;
			j++;
		}
		else
		{
			temp = i;
			break;
		}
	}

	for(i = temp; i < 8; i++)
	{
		fileName[i] = ' ';
	}

	if(name[temp++] == '.')
	{
		i = 8;
		while(i < 11)
		{
			if(name[temp] != '\0')
			{
				fileName[i] = name[temp++];
			}
			else
			{
				temp = i;
				break;
			}

			if(i == 10)
			{
				temp = i++;
			}
			i++;
		}
		while(temp < 11)
		{
			fileName[temp] = ' ';
			temp++;
		}
	}
	else
	{
		while(temp < 11)
		{
			fileName[temp] = ' ';
			temp++;
		}
	}
	
	fileName[11] = '\0';
	i = 0;

	while(i < 32)
	{
		empty[i] = '\0';
		i++;
	}

	DIR_entry = find_file(current_cluster_number, fileName);
	printf("TESTING DIR_entry name: %s\n", DIR_entry.DIR_Name);
	offset = return_offset(current_cluster_number, fileName);

	if(DIR_entry.DIR_Attr == 0x10)
	{
		printf("Error: This is a directory\n");
		return CLUSTER_END;
		//return 0xFFFE;
	}
	else if(DIR_entry.DIR_Attr == 0x20)
	{
		if(!unopened(offset))
		{
			printf("Error: Already Opened\n");
		}
		else
		{
			nextCluster = (DIR_entry.DIR_FstClusHI << 16 | DIR_entry.DIR_FstClusLO);
			empty_val_cluster(nextCluster);
			fseek(file, offset, SEEK_SET);
			fwrite(&empty, OFFSET_CONST, 1, file);
			return CLUSTER_END;
			//return 0xFFFE;	
		}
	}
	else
	{
		printf("Error: Not a File\n");
		return CLUSTER_END;
		//return 0xFFFE;
	}
/*	else
	{
		printf("Error: No such Entry\n");
		return 0xFFFE;
	}
*/
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

//UTILITIES FUNCTION
struct DIR find_file(unsigned int cluster, char *name)
{
	int i;
	long offset;
	char fileName[12];
	struct DIR DIR_entry;

	for(;;)
	{
		offset = first_sector_cluster(current_cluster_number) * bpb_32.BPB_BytsPerSec;
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
			offset += OFFSET_CONST;
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
			if (string[i] != '\0'){
				name[j] = string[i];
				j++;
			}
			else{
				name[j] = '\0';
				j = 0;
				break;
			}		
		}

		printf("testing name: %s\n", name);
		if (strcmp(name, "") != 0)
		{
			printf("testing name loop: %s\n", name);
			printf("cluster loop: %d\n", cluster);
			cluster = return_cluster_dir(cluster, name);
			printf("cluster loop: %d\n", cluster);
		}
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
		offset = sector_offset(first_sector_cluster(current_cluster_number));
		fseek(file, offset, SEEK_SET);

		long temp = sector_offset(first_sector_cluster(current_cluster_number) + bpb_32.BPB_SecPerClus);
		while ( offset < temp )
		{
			fread(&DIR_entry, sizeof(struct DIR), 1, file);
			offset+=OFFSET_CONST;
			if (DIR_entry.DIR_Name[0] == ENTRY_EMPTY)
				continue;
			else if (DIR_entry.DIR_Name[0] == ENTRY_LAST)
				break;
			else if (DIR_entry.DIR_Name[0] == 0x05)
				DIR_entry.DIR_Name[0] = ENTRY_EMPTY;

			if (DIR_entry.DIR_Attr != ATTRIBUTE_NAME_LONG){
				for (i = 0; i < 11; i++)
				{
					fileName[i] = DIR_entry.DIR_Name[i];
				}	

				fileName[11] = '\0';
			}

			if ((strcmp(fileName, name) == 0) && DIR_entry.DIR_Attr == 0x10)
			{		
				return (DIR_entry.DIR_FstClusHI *0x100 + DIR_entry.DIR_FstClusLO);
			}
		}
		cluster = FAT_32(cluster);
		if((cluster == 0x0FFFFFF8) || (cluster == 0x0FFFFFFF) || (cluster == 0x00000000))
		{
			break;
		}	
	}
}

long first_sector_cluster(unsigned int cluster)
{
	return ( (cluster - 2) * bpb_32.BPB_SecPerClus + bpb_32.BPB_RsvdSecCnt + bpb_32.BPB_FATSz32 * 2);
}

long empty_cluster()
{
	//root starts at 2
	int i = 2;
	while(1)
	{
		//empty
		if(FAT_32(i) == 0)
		{
			return i;
		}
		else
		{
			i++;
		}
	}	
}

void change_val_cluster(unsigned int value, unsigned int cluster)
{
	long offset;
	offset = bpb_32.BPB_RsvdSecCnt * bpb_32.BPB_BytsPerSec + cluster * 4;
	fseek(file, offset, SEEK_SET);
	fwrite(&value, sizeof(unsigned int), 1, file);
	fflush(file);
}

void empty_val_cluster(unsigned int cluster)
{
	int i = 0;
	unsigned char *empty;
	unsigned int value = 0;
	long offset;

	empty = (unsigned char *)malloc(sizeof(unsigned char) * bpb_32.BPB_BytsPerSec * bpb_32.BPB_SecPerClus);

	for (i; i < bpb_32.BPB_BytsPerSec * bpb_32.BPB_SecPerClus; i++)
	{
		empty[i] = 0;
	}

	offset = bpb_32.BPB_RsvdSecCnt * bpb_32.BPB_BytsPerSec + cluster * 4;

	if (FAT_32(cluster) <= 0x0FFFFFEF
		&& FAT_32(cluster) >= 0x00000002)
	{
		empty_val_cluster(FAT_32(cluster));	
	}

	fseek(file, sector_offset(first_sector_cluster(cluster)), SEEK_SET);
	fwrite(empty, bpb_32.BPB_BytsPerSec*bpb_32.BPB_SecPerClus, 1, file);
	fseek(file, offset, SEEK_SET);
	fwrite(&value, sizeof(unsigned int), 1, file);
}

int equals(unsigned char name1[], unsigned char name2[])
{
	int i;
	for(i = 0; i < 11; i++)
	{
		if(name1[i] != name2[i])
		{
			return 0;
		}
	}
	return 1;
}

int unopened(long offset)
{
	int i;
	for (i = 0; i < openedFileNum; i++)
	{
		if (openedFile[i] == offset)
		{
			return 0;
		}
	}
	return 1;
}

long return_offset(unsigned int cluster, char *name)
{	
	int i;
	char fileName[12];
	long offset;
	struct DIR DIR_entry;

	for(;;)
	{
		offset = sector_offset(first_sector_cluster(cluster));
		fseek(file, offset, SEEK_SET);

		long temp = sector_offset(first_sector_cluster(cluster)) + bpb_32.BPB_BytsPerSec * bpb_32.BPB_SecPerClus;
		while (temp >= offset)
		{
			fread(&DIR_entry, sizeof(struct DIR), 1, file);

			if (DIR_entry.DIR_Name[0] == 0x05)
			{
				DIR_entry.DIR_Name[0] = ENTRY_EMPTY;
			}

			if (DIR_entry.DIR_Attr != ATTRIBUTE_NAME_LONG)
			{
				for (i = 0; i < 11; i++)
				{
					fileName[i]=DIR_entry.DIR_Name[i];
				}

				fileName[11] = '\0';

				if (strcmp(fileName, name) == 0)
				{
					return offset;
				}
			}

			offset += OFFSET_CONST;
		}
		cluster = FAT_32(cluster);

		if (END_OF_CLUSTER < cluster)
		{
			break;
		}
	}
}
