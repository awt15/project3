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
#define READ_ONLY 1
#define WRITE_ONLY 2
#define READ_WRITE 3
#define WRITE_READ 4

//STRUCTURES
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

	unsigned int	BPB_FATSz32;
	unsigned short	BPB_ExtFlags;
	unsigned short	BPB_FSVer;
	unsigned int	BPB_RootClus;
	unsigned char	BS_VolLab[11];
	unsigned char	BS_FilSysType[8];
	unsigned char   boot_code[436];
	unsigned short boot_sector_signature;
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

struct OPEN_FILES
{
	int file_first_cluster_number;
	unsigned short mode;
}__attribute((packed));

//MAIN GLOBALS: STRUCTS/FILE
extern struct FSI BPB_FSI_info;
extern struct BPB_32 bpb_32;
extern struct DIR directory;
extern FILE *file;

//MORE GLOBALS
char *fatImgName;
char *parentDir;
char *workingDir;
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
struct OPEN_FILES open_files_array[100];
int open_files_arraysize = 0;

//MAIN FUNCTIONS
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
void open(char *name, unsigned short mode);
void close(char *name);
void readfile(char *name, int offset, int size);
void writefile(char *name, int offset, int size, char *string);

//SUB-FUNCTIONS
struct DIR find_file(unsigned int cluster, char *name);
void change_val_cluster(unsigned int value, unsigned int cluster);
void empty_val_cluster(unsigned int cluster);
int opened(unsigned int cluster);
void close_file(unsigned int cluster);
unsigned int FAT_32(unsigned int cluster);
unsigned int return_cluster_dir(unsigned int cluster, char *name);
long sector_offset(long sec);
long first_sector_cluster(unsigned int cluster);
long find_empty_cluster(unsigned int cluster);
long return_offset(unsigned int cluster, char *name);
long return_cluster_path(char *string);
long empty_cluster();


int main(int argc, char* argv[])
{
	int i = 0;
	char mode[3];
	char name[13];
	char operation[6];

	writeFileNum = 0;
	readFileNum = 0;
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
			
			FirstDataSector = bpb_32.BPB_RsvdSecCnt + (bpb_32.BPB_NumFATs*bpb_32.BPB_FATSz32);
			FirstSectorofCluster = ((bpb_32.BPB_RootClus - 2)*bpb_32.BPB_SecPerClus) + FirstDataSector;

			//LOOP for User
			for(;;)
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
				else if(strcmp(operation, "cd") == 0)
				{
					scanf("%s", name);
					
					if(strcmp(name,"..") != 0)
					{
						parentCluster = current_cluster_number;
					}
					getchar();
					cd(name);
				}
				else if(strcmp(operation, "size") == 0)
				{
					scanf("%s", name);
					getchar();
					int s = size(name);
					if(s == -1)
					{
						continue;
					}
					else
					{
						printf("%d\n", size(name));
					}
				}
				else if(strcmp(operation, "creat") == 0)
				{
					scanf("%s", name);
					getchar();
					create(name);
				}
				else if(strcmp(operation, "mkdir") == 0)
				{
					scanf("%s", name);
					getchar();
					mkdir(name);
				}
				else if(strcmp(operation, "rm") == 0)
				{
					scanf("%s", name);
					getchar();
					rm(name);
				}
				else if(strcmp(operation, "rmdir") == 0)
				{
					scanf("%s", name);
					getchar();
					rmdir(name);
				}
				else if(strcmp(operation, "open") == 0)
				{
					fgets(name, sizeof(name), stdin);
					
					if(sscanf(name, "%s %s", name, mode) == 2)
					{
						if(strcmp(mode, "r") == 0)
						{
							open(name, READ_ONLY);
						}
						else if(strcmp(mode, "w") == 0)
						{
							open(name, WRITE_ONLY);
						}
						else if(strcmp(mode, "rw") == 0)
						{
							open(name, READ_WRITE);
						}
						else if(strcmp(mode, "wr") == 0)
						{
							open(name, WRITE_READ);
						}
						else
						{
							printf("ERROR: Invalid mode.\n");
						}
					}
					else
					{
						printf("ERROR: Invalid arguments.\n");
					}					
				}
				else if(strcmp(operation, "close") == 0)
				{
					scanf("%s", name);
					getchar();
					close(name);
				}
				else if(strcmp(operation, "read")==0)
				{
					int offset, size;
					scanf("%s %d %d", name, &offset, &size);
					getchar();
					readfile(name, offset, size);
				}
				else if(strcmp(operation, "write")==0)
				{
					int offset, size;
					char string[13];
					scanf("%s %d %d %s", name, offset, size, string);
					getchar();
					writefile(name, offset, size, string);
				}
				else
				{
					printf("Incorrect arguments. Enter exit, info, ls, cd, size, create, mkdir, rm, rmdir, open, close, read, write.\n");			
				}
			}
			return 0;
		}
		else
		{
			printf("Could not find FAT_32 image.\n");
			return -1;
		}
	}
	else
	{
		printf("Incorrect number of arguments.\n");
		return -1;
	}
}

//MAIN FUNCTIONS
int info()
{
	struct FSI BPB_FSI_info;

	fseek(file, 0x00, SEEK_SET);
	fread(&BPB_FSI_info, sizeof(struct FSI), 1, file);
	
	printf("Sectors per Cluster: %d\n", bpb_32.BPB_SecPerClus);
	printf("Total Sectors: %d\n", bpb_32.BPB_TotSec32);
	printf("Bytes per Sector: %d\n", bpb_32.BPB_BytsPerSec);
	printf("Sectors per FAT: %d\n", bpb_32.BPB_FATSz32);
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

	while(1)
	{
		offset = first_sector_cluster(current_cluster_number) * bpb_32.BPB_BytsPerSec;
		offset_total = offset + bpb_32.BPB_BytsPerSec;
		fseek(file, offset, SEEK_SET);
		while(offset < offset_total)
		{
			fread(&directory, sizeof(struct DIR), 1, file);
			offset += OFFSET_CONST;
			
			if(directory.DIR_Attr == 0x10 || directory.DIR_Attr == 0x20)
			{
				while(counter < MAX)
				{
					if(directory.DIR_Name[counter] == ' ')
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
	if(cd(name) == -1)
	{
		return 0;
	}
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
	
	//Getting Directory Name
	while(name[i] != '\0')
	{
		if (name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		fileName[i] = name[i];
		++i;
	}

	while(i < 11)
	{
		fileName[i] = ' ';
		++i;
	}

	fileName[i] = '\0';
	
	//Directory Name Possibilities
	if(strcmp(name, ".") == 0)
	{
		//root?		
	}
	else if(strcmp(name, "/") == 0)
	{
		workingDir[0] = '/';
		workingDir[1] = '\0';
		current_cluster_number = bpb_32.BPB_RootClus;
	}
	else if(strcmp(name, "..") == 0)
	{
		if(strcmp(workingDir, "/") == 0)
		{
			printf("ERROR: It's in the root directory.\n");
			return -1;
		}
		else
		{	
			while(workingDir[i] != '\0')
			{
				++i;
			}

			while((i = i -1) && workingDir[i] != '/')
			{
				--i;
			}
			
			if(i == 0)
			{
				workingDir[i + 1] = '\0';
			}
			else
			{
				workingDir[i] = '\0';
			}
			//printf("TESTING CLUSTER NUMBER BEFORE %d\n", current_cluster_number);
			//printf("TESTING WORKING DIR: %s\n", workingDir);
			current_cluster_number = return_cluster_path(workingDir);
			//current_cluster_number = parentCluster;
			//printf("TESTING CLUSTER NUMBER AFTER %d\n", current_cluster_number);
		}
	}
	else
	{
		DIR_entry = find_file(current_cluster_number, fileName);
		if(DIR_entry.DIR_Name[0] == ENTRY_LAST)
		{
			printf("ERROR: No directory found!\n");
			return -1;
		}
		else 
		{
			if(DIR_entry.DIR_Attr == 0x10)
			{
				current_cluster_number = return_cluster_dir(current_cluster_number, fileName);
				i = 0;
				j = 0;
				while(workingDir[i] != '\0')
				{
					++i;
				}

				if(workingDir[i - 1] != '/')
				{
					workingDir[i++] = '/';
				}

				while(name[j] != '\0')
				{
					workingDir[i] = name[j];
					++i;
					++j;
				}

				workingDir[i] = '\0';
			}
			else
			{
				printf("ERROR: That is not a directory!\n");
				return -1;
			}
		}
	}
	return 0;
}

unsigned int size(char* file)
{
	int i = 0;
	int j = 0;
	char fileName[12];
	struct DIR DIR_entry;
	
	//Getting File Name
	while(file[i] != '\0')
	{
		if(file[i] >= 'a' && file[i] <= 'z')
		{
			file[i] -= OFFSET_CONST;
		}
		fileName[i] = file[i];
		++i;
	}

	while(i < 11)
	{
		fileName[i] = ' ';
		++i;
	}

	fileName[i] = '\0';
	
	//File Name Possibilities
	if(strcmp(file, ".") == 0)
	{
		//root?		
	}
	else
	{
		DIR_entry = find_file(current_cluster_number, fileName);
		if(DIR_entry.DIR_Name[0] == ENTRY_LAST)
		{
			printf("ERROR: FILENAME was not found!\n");
			return -1;
		}
		else 
		{
			return DIR_entry.DIR_FileSize;
		}
	}
	return 0;
}

int create (char *name)
{
	int j = 0;
	int i = 0;
	int temp;
	char fileName[12];
	unsigned int newCluster;
	long offset;
	struct DIR emptyEntry, DIR_entry;
	struct FSI BPB_FSI_info;


	// loops set up the file name by accessing indices in name 
	// prep file name before creating it
	while(name[i] != '\0') 
	{
		if(name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		i++;
	}

	// read in name portion of file into fileName
	i = 0;
	while(i < 8) 
	{
		if(name[j] != '\0' && name[j] != '.')
		{
			fileName[i] = name[i];
			i++;
			j++;
		}
		else
		{
			temp = i;
			break;
		}
	}

	// fill up the rest of fileName with spaces
	for(i = temp; i < 8; i++)
	{
		fileName[i] = ' ';
	}

	// accounting for extensions
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

	// set the end of fileName to null character
	fileName[11] = '\0';
	
	// get the directory entry
	DIR_entry = find_file(current_cluster_number, fileName);
	if(DIR_entry.DIR_Name[0] == 0)
	{
		offset = find_empty_cluster(current_cluster_number);
		i = 0;
		while(i < 11)
		{
			emptyEntry.DIR_Name[i] = fileName[i];
			++i;
		}

		// set up the directory 
		emptyEntry.DIR_Attr = 0x20;
		emptyEntry.DIR_NTRes = 0;
		emptyEntry.DIR_FileSize = 0;

		//Get the right cluster number
		if(BPB_FSI_info.FSI_Nxt_Free == 0xFFFFFFFF)
		{
			newCluster = 2;
		}
		else
		{
			newCluster = BPB_FSI_info.FSI_Nxt_Free + 1;
		}
		
		//Testing cluster number
		for(;;)
		{
			if(FAT_32(newCluster) == 0 || FAT_32(newCluster) == 0x0FFFFFFF || FAT_32(newCluster) == 0x0FFFFFF8)
			{
				emptyEntry.DIR_FstClusHI = (newCluster >> 16);
				emptyEntry.DIR_FstClusLO = (newCluster & 0xFFFF);
				change_val_cluster(0x0FFFFFF8, newCluster);
				BPB_FSI_info.FSI_Nxt_Free = newCluster;

				fflush(file);
				break;
			}
			if(newCluster == 0xFFFFFFFF)
			{
				newCluster = 1;
			}
		}

		fseek(file, offset, SEEK_SET);
		fwrite(&emptyEntry, sizeof(struct DIR), 1, file);
		fflush(file);
		return 0xFFF0;
	}
	else
	{
		printf("ERROR: Entry already exists\n");
		return 0xFFFE;
	}
}

int mkdir (char *name)
{
	int j = 0;
	int i = 0;
	int temp;
	char fileName[12];
	unsigned int newCluster;
	long offset;
	struct DIR emptyEntry, DIR_entry;
	struct FSI BPB_FSI_info;


	// loops set up the file name by accessing indices in name 
	// prep file name before creating it
	while(name[i] != '\0') 
	{
		if(name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		i++;
	}

	// read in name portion of file into fileName
	i = 0;
	while(i < 8) 
	{
		if(name[j] != '\0' && name[j] != '.')
		{
			fileName[i] = name[i];
			i++;
			j++;
		}
		else
		{
			temp = i;
			break;
		}
	}

	// fill up the rest of fileName with spaces
	for(i = temp; i < 8; i++)
	{
		fileName[i] = ' ';
	}

	// accounting for extensions
	if(name[temp++] == '.') 
	{
		i = 8;

		while (i < 11)
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

	// set the end of fileName to null character
	fileName[11] = '\0';
	
	// get the directory entry
	DIR_entry = find_file(current_cluster_number, fileName);
	if(DIR_entry.DIR_Name[0] == 0)
	{
		offset = find_empty_cluster(current_cluster_number);
		i = 0;
		while(i < 11)
		{
			emptyEntry.DIR_Name[i] = fileName[i];
			++i;
		}

		// set up the directory 
		emptyEntry.DIR_Attr = 0x10;
		emptyEntry.DIR_NTRes = 0;
		emptyEntry.DIR_FileSize = 0;
			
		//Getting Right Cluster number
		if(BPB_FSI_info.FSI_Nxt_Free == 0xFFFFFFFF)
		{
			newCluster = 2;
		}
		else
		{
			newCluster = BPB_FSI_info.FSI_Nxt_Free + 1;
		}
		
		//Testing cluster number
		for(;;)
		{
			if(FAT_32(newCluster) == 0 || FAT_32(newCluster) == 0x0FFFFFFF || FAT_32(newCluster) == 0x0FFFFFF8)
			{
				emptyEntry.DIR_FstClusHI = (newCluster >> 16);
				emptyEntry.DIR_FstClusLO = (newCluster & 0xFFFF);
				change_val_cluster(0x0FFFFFF8, newCluster);
				BPB_FSI_info.FSI_Nxt_Free = newCluster;

				fflush(file);
				break;
			}
			
			if(newCluster == 0xFFFFFFFF)
			{
				newCluster = 1;
			}
		}

		fseek(file, offset, SEEK_SET);
		fwrite(&emptyEntry, sizeof(struct DIR), 1, file);
		fflush(file);
		
		//fill in directory with "." ".."
		offset = find_empty_cluster(newCluster);
		emptyEntry.DIR_Name[0] = '.';
		i = 1;
		while(i < 12)
		{
			emptyEntry.DIR_Name[i] = ' ';
			i++;
		}
		emptyEntry.DIR_Attr = 0x10;
		emptyEntry.DIR_NTRes = 0;
		emptyEntry.DIR_FileSize = 0;
		emptyEntry.DIR_FstClusHI = (newCluster >> 16);
		emptyEntry.DIR_FstClusLO = (newCluster & 0xFFFF);
		fseek(file, offset, SEEK_SET);
		fwrite(&emptyEntry, sizeof(struct DIR), 1, file);

		offset = find_empty_cluster(newCluster);
		emptyEntry.DIR_Name[0] = '.';
		emptyEntry.DIR_Name[1] = '.';

		i = 2;
		while(i < 12)
		{
			emptyEntry.DIR_Name[i] = ' ';
			i++;
		}
		emptyEntry.DIR_Attr = 0x10;
		emptyEntry.DIR_NTRes = 0;
		emptyEntry.DIR_FileSize = 0;
		emptyEntry.DIR_FstClusHI = (newCluster >> 16);
		emptyEntry.DIR_FstClusLO = (newCluster & 0xFFFF);
		fseek(file, offset, SEEK_SET);
		fwrite(&emptyEntry, sizeof(struct DIR), 1, file);
		
		return 0xFFF0;
	}
	else
	{
		printf("ERROR: Entry already exists\n");
		return 0xFFFE;
	}
}

int rm (char *name)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int temp;
	long offset;
	unsigned int nextCluster;
	char fileName[12];
	char empty[32];
	struct DIR DIR_entry;
	unsigned int cluster;

	//Getting File Name
	while(name[i] != '\0')
	{
		if(name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		fileName[i] = name[i];
		++i;
	}

	while(i < 11)
	{
		fileName[i] = ' ';
		++i;
	}

	fileName[i] = '\0';

	while(j < 32)
	{
		empty[j] = '\0';
		j++;
	}

	//Getting correct offset of file
	DIR_entry = find_file(current_cluster_number, fileName);
	offset = return_offset(current_cluster_number, fileName);
	if(offset == -1)	return 0;

	//Testing file and removing if it is unopened
	if(DIR_entry.DIR_Attr == 0x10)
	{
		printf("ERROR: This is a directory\n");
		return CLUSTER_END;
	}
	else if(DIR_entry.DIR_Attr == 0x20)
	{
		cluster = (DIR_entry.DIR_FstClusHI << 16 | DIR_entry.DIR_FstClusLO);
		if(opened(cluster) == 1)
		{
			printf("ERROR: File is open\n");
		}
		else
		{
			empty_val_cluster(cluster);	//changed from NextCluster to cluster
			fseek(file, offset, SEEK_SET);
			fwrite(&empty, OFFSET_CONST, 1, file);
			return CLUSTER_END;	
		}
	}
	else
	{
		printf("Error: Not a file\n");
	}
}

int rmdir (char *name)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int temp;
	long offset;
	unsigned int nextCluster;
	char fileName[12];
	char empty[32];
	struct DIR DIR_entry;

	//Getting Directory Name
	while(name[i] != '\0')
	{
		if(name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		fileName[i] = name[i];
		++i;
	}

	while(i < 11)
	{
		fileName[i] = ' ';
		++i;
	}

	fileName[i] = '\0';

	while(i < 32)
	{
		empty[i] = '\0';
		i++;
	}

	//Getting right offset for directory
	DIR_entry = find_file(current_cluster_number, fileName);
	offset = return_offset(current_cluster_number, fileName);

	//Testing directory and removing if it is
	if(DIR_entry.DIR_Attr == 0x10)
	{
		nextCluster = (DIR_entry.DIR_FstClusHI << 16 | DIR_entry.DIR_FstClusLO);
		empty_val_cluster(nextCluster);
		fseek(file, offset, SEEK_SET);
		fwrite(&empty, OFFSET_CONST, 1, file);
		return CLUSTER_END;
	}
	else if(DIR_entry.DIR_Attr == 0x20)
	{
		printf("ERROR: This is a file\n");
		return CLUSTER_END;
	}
	else
	{
		printf("ERROR: Not a directory\n");
		return CLUSTER_END;
	}
}

void open(char *name, unsigned short mode)
{
	int i = 0;
	char fileName[12];
	struct DIR DIR_entry;
	int filemode = -1;				//if the file is READ_ONLY or not
	unsigned int cluster;
	
	//Getting File Name
	while(name[i] != '\0')
	{
		if (name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		fileName[i] = name[i];
		++i;
	}

	while(i < 11)
	{
		fileName[i] = ' ';
		++i;
	}

	fileName[i] = '\0';
	DIR_entry = find_file(current_cluster_number, fileName);
	
	//Making sure file is not adirectory
	if(DIR_entry.DIR_Attr == 0x10)
	{
		printf("ERROR: This is a directory\n");
	}
	else if((DIR_entry.DIR_Attr == 0x20)||(DIR_entry.DIR_Attr == 0x01))
	{
		if(DIR_entry.DIR_Attr == 0x01){
			filemode = READ_ONLY;
		}
		else
		{
			filemode = 0;
		}
		cluster = (DIR_entry.DIR_FstClusHI << 16 | DIR_entry.DIR_FstClusLO);
		if(opened(cluster) == 1)
		{
			printf("ERROR: File is already open\n");
		}
		else
		{
			if((filemode == READ_ONLY)&&(mode == READ_ONLY))
			{
				open_files_array[open_files_arraysize].file_first_cluster_number = cluster;
				open_files_array[open_files_arraysize].mode = mode;
				open_files_arraysize += 1;
			}
			else if(filemode != READ_ONLY)
			{
				open_files_array[open_files_arraysize].file_first_cluster_number = cluster;
				open_files_array[open_files_arraysize].mode = mode;
				open_files_arraysize += 1;
			}
			else
			{
				printf("ERROR: File is READ ONLY and cannot be written to\n");
			}
		}	
	}
	else
	{
		printf("ERROR: Not a File\n");
	}

}

void close(char *name)
{
	int i = 0;
	char fileName[12];
	struct DIR DIR_entry;
	int filemode = -1;				//if the file is READ_ONLY or not
	unsigned int cluster;
	
	//Getting file name
	while(name[i] != '\0')
	{
		if(name[i] >= 'a' && name[i] <= 'z')
		{
			name[i] -= OFFSET_CONST;
		}
		fileName[i] = name[i];
		++i;
	}

	while(i < 11)
	{
		fileName[i] = ' ';
		++i;
	}
	fileName[i] = '\0';
	DIR_entry = find_file(current_cluster_number, fileName); //Making sure file is not a directory
	if(DIR_entry.DIR_Attr == 0x10)
	{
		printf("ERROR: This is a directory\n");
	}
	else if((DIR_entry.DIR_Attr == 0x20)||(DIR_entry.DIR_Attr == 0x01))
	{
		cluster = (DIR_entry.DIR_FstClusHI << 16 | DIR_entry.DIR_FstClusLO);
		if(opened(cluster) == 1)
		{
			close_file(cluster);
		}
		else
		{
			printf("ERROR: File is not open\n");
		}	
	}
	else
	{
		printf("ERROR: Not a file\n");
	}

}

void readfile(char *name, int offset, int size)
{
	/*
	struct DIR dir;
	int file_size = sizeof(name);
	dir = find_file(current_cluster_number, name);
	if(opened(offset))
	{
		printf("ERROR: File is not opened.\n");
	}
	else if(strcmp(dir.DIR_Name, name) != 0)
	{
		printf("ERROR: File cannot be found.\n");
	}
	else if(dir.DIR_Attr == 0x10)
	{
		printf("ERROR: File is a directory.\n");
	}
	else if(offset > file_size)
	{
		printf("ERROR: OFFSET is larger than the size of the file.\n");
	}
	else
	{
	}
	*/
}

void writefile(char *name, int offset, int size, char *string)
{
	/*
	struct DIR dir;
	int file_size = sizeof(name);
	dir = find_file(current_cluster_number, name);
	if(opened(offset))
	{
		printf("ERROR: File is not opened.\n");
	}
	else if(strcmp(dir.DIR_Name, name) != 0)
	{
		printf("ERROR: File cannot be found.\n");
	}
	else if(dir.DIR_Attr == 0x10)
	{
		printf("ERROR: File is a directory.\n");
	}
	else if(offset > file_size)
	{
		printf("ERROR: OFFSET is larger than the size of the file.\n");
	}
	else
	{
	}
	*/
}

//UTILITIES FUNCTION

//Find File return a directory struct filled with the correct information
//given the cluster number and name
struct DIR find_file(unsigned int cluster, char *name)
{
	int i;
	long offset;
	char fileName[12];
	struct DIR DIR_entry;

	long sectorbytes;
	while(1)
	{
		offset = (first_sector_cluster(cluster)) * bpb_32.BPB_BytsPerSec;
		fseek(file, offset, SEEK_SET);
		long temp = offset + bpb_32.BPB_BytsPerSec;
		while(temp > offset)
		{
			fread(&DIR_entry, sizeof(struct DIR), 1, file);

			if(DIR_entry.DIR_Name[0] == ENTRY_EMPTY)
			{
				continue;
			}
			else if(DIR_entry.DIR_Name[0] == ENTRY_LAST)
			{
				return DIR_entry;
			}
			else if(DIR_entry.DIR_Name[0] == 0x05)
			{
				DIR_entry.DIR_Name[0] = ENTRY_EMPTY;
			}

			if(DIR_entry.DIR_Attr != ATTRIBUTE_NAME_LONG)
			{

				for(i=0; i<11; i++)
				{
					fileName[i]=DIR_entry.DIR_Name[i];
				}

				fileName[11]= '\0';
				if(strcmp(fileName, name) == 0)
				{
					return DIR_entry;
				}
			}
			offset += OFFSET_CONST;
		}
		cluster = FAT_32(cluster);

		if(cluster >= 0x0FFFFFF8)
		{
			break;
		}
	}
}

//Used to find offset
long sector_offset(long sec)
{
	return (bpb_32.BPB_BytsPerSec * sec);
}

//Used for FAT[cluster]
unsigned int FAT_32(unsigned int cluster)
{
	unsigned int nextCluster;
	long offset;

	offset = bpb_32.BPB_RsvdSecCnt * bpb_32.BPB_BytsPerSec + cluster * 4;
	fseek(file, offset, SEEK_SET);
	fread(&nextCluster, sizeof(unsigned int), 1, file);

	return nextCluster;
}

//Returning the correct cluster given the name
long return_cluster_path(char *string)
{
	int i = 1;
	int j = 0;
	long cluster;
	unsigned char name[11];
	cluster = bpb_32.BPB_RootClus;
	for(;;)
	{
		for(i; ; i++)
		{
			if(string[i] != '\0')
			{
				name[j] = string[i];
				j++;
			}
			else
			{
				name[j] = '\0';
				j = 0;
				break;
			}		
		}

		if(strcmp(name, "") != 0)
		{
			//printf("testing name loop: %s\n", name);
			//printf("cluster loop: %d\n", cluster);
			cluster = return_cluster_dir(cluster, name);
			//printf("cluster loop: %d\n", cluster);
		}
		else
		{
			break;
		}
	}
	return cluster;
}

//Returns cluster number based off of file name and cluster number
unsigned int return_cluster_dir(unsigned int cluster, char *name)
{
	int i = 0;
	char fileName[12];
	long offset;
	struct DIR DIR_entry;
	for(;;)
	{
		offset = sector_offset(first_sector_cluster(current_cluster_number));
		fseek(file, offset, SEEK_SET);

		long temp = sector_offset(first_sector_cluster(current_cluster_number) + bpb_32.BPB_SecPerClus);
		while(offset < temp)
		{
			fread(&DIR_entry, sizeof(struct DIR), 1, file);
			offset+=OFFSET_CONST;
			if(DIR_entry.DIR_Name[0] == ENTRY_EMPTY)
			{
				continue;
			}
			else if(DIR_entry.DIR_Name[0] == ENTRY_LAST)
			{
				break;
			}
			else if(DIR_entry.DIR_Name[0] == 0x05)
			{
				DIR_entry.DIR_Name[0] = ENTRY_EMPTY;
			}

			if(DIR_entry.DIR_Attr != ATTRIBUTE_NAME_LONG)
			{
				for (i = 0; i < 11; i++)
				{
					fileName[i] = DIR_entry.DIR_Name[i];
				}	

				fileName[11] = '\0';
			}

			if((strcmp(fileName, name) == 0) && DIR_entry.DIR_Attr == 0x10)
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

//First sector cluster formula
long first_sector_cluster(unsigned int cluster)
{
	return ((cluster - 2) * bpb_32.BPB_SecPerClus + bpb_32.BPB_RsvdSecCnt + bpb_32.BPB_FATSz32 * 2);
}

//Empty Clusters
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

//For Writing/Reading
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

	for(i; i < bpb_32.BPB_BytsPerSec * bpb_32.BPB_SecPerClus; i++)
	{
		empty[i] = 0;
	}

	offset = bpb_32.BPB_RsvdSecCnt * bpb_32.BPB_BytsPerSec + cluster * 4;

	if(FAT_32(cluster) <= 0x0FFFFFEF
		&& FAT_32(cluster) >= 0x00000002)
	{
		empty_val_cluster(FAT_32(cluster));	
	}

	fseek(file, sector_offset(first_sector_cluster(cluster)), SEEK_SET);
	fwrite(empty, bpb_32.BPB_BytsPerSec*bpb_32.BPB_SecPerClus, 1, file);
	fseek(file, offset, SEEK_SET);
	fwrite(&value, sizeof(unsigned int), 1, file);
}

//Check if file is opened or not
int opened(unsigned int cluster)
{
	int i;
	if(open_files_arraysize == 0)
	{	
		return 0;
	}
	else
	{
		for(i = 0; i < open_files_arraysize; i++)
		{
			if(open_files_array[i].file_first_cluster_number == cluster)
			{
				return 1;
			}
		}
	}
	return 0;
}

void close_file(unsigned int cluster)
{
	int i;
	int j;
	for(i = 0; i < open_files_arraysize; i++)
	{
		if(open_files_array[i].file_first_cluster_number == cluster)
		{
			open_files_array[i].file_first_cluster_number = -1;
			for(j = i + 1; j < open_files_arraysize; j++)
			{
				open_files_array[j-1].file_first_cluster_number = open_files_array[j].file_first_cluster_number;
				open_files_array[j-1].mode = open_files_array[j].mode;
			}
			break;
		}
	}
	open_files_arraysize -= 1;
}

//return the correct offset given cluster and file name
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
		while(temp >= offset)
		{
			fread(&DIR_entry, sizeof(struct DIR), 1, file);

			if(DIR_entry.DIR_Name[0] == 0x05)
			{
				DIR_entry.DIR_Name[0] = ENTRY_EMPTY;
			}
			/*else if(DIR_entry.DIR_Name[0] == ENTRY_LAST)
			{
				printf("ERROR: No file found.\n");
				//return -1;
			}*/

			if(DIR_entry.DIR_Attr != ATTRIBUTE_NAME_LONG)
			{
				for (i = 0; i < 11; i++)
				{
					fileName[i]=DIR_entry.DIR_Name[i];
				}

				fileName[11] = '\0';

				if(strcmp(fileName, name) == 0)
				{
					return offset;
				}
			}

			offset += OFFSET_CONST;
		}
		cluster = FAT_32(cluster);

		if(END_OF_CLUSTER < cluster)
		{
			break;
		}
	}
}

//Getting complete empty cluster and returning the offset
long find_empty_cluster(unsigned int cluster){
	unsigned int nextCluster;
	long offset;
	struct FSI BPB_FSI_info;
	struct DIR DIR_entry;

	// begin loop
	for(;;)
	{
		offset = sector_offset(first_sector_cluster(cluster));
		long temp = offset + bpb_32.BPB_BytsPerSec;
		fseek(file, offset, SEEK_SET);

		// temp variable to compare with offset
		while(temp > offset)
		{
			if(offset > temp)
			{
				break;
			}

			fread(&DIR_entry, sizeof(struct DIR), 1, file);

			if((DIR_entry.DIR_Name[0] != ENTRY_EMPTY) && (DIR_entry.DIR_Name[0] != ENTRY_LAST))
			{
				offset += OFFSET_CONST;
				fseek(file,offset,SEEK_SET);
				continue;
			}
			else
			{
				return offset;
			}

		}

		if(END_OF_CLUSTER <= FAT_32(cluster))
		{	
			if(BPB_FSI_info.FSI_Nxt_Free != CLUSTER_END)
			{	
				nextCluster = BPB_FSI_info.FSI_Nxt_Free + 1;
			}	
			else
			{
				nextCluster = 0x00000002;
			}

			for( ; ; nextCluster++)
			{
				if(FAT_32(nextCluster) == 0x00000000)
				{
					change_val_cluster(nextCluster, cluster);
					change_val_cluster(END_OF_CLUSTER, nextCluster);
					BPB_FSI_info.FSI_Nxt_Free = cluster;
					fflush(file);
					break;
				}
				if(nextCluster == CLUSTER_END)
				{	
					nextCluster = 0x00000001;
				}
			}
			cluster = nextCluster;
		}
		else
		{
			cluster = FAT_32(cluster);
		}
	}	
}