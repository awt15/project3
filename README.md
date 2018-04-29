Project 3: FAT32 File System

Members:
  -Anthony Tieu
  -Vita Tran
  -Mustafa Syed
  
Tar File:
  -Named: p3-Tieu-Tran-Syed.tar
  -Files:
  	-README.md
	-proj3.c
	-makefile

Makefile Instructions:
	a.out:
		compiles our c file using "gcc proj3.c".
	clean:
		removes our executable using "rm a.out".

Known Bugs:
	Whole Project:
		-Not sure if error checking if fully working but most of the functions
			should be checking for correct amount of arguments.
	CD:
		-"cd .." only works whenever the parent directory is the root.
			If the parent directory is not the root then "cd .." changes our
			current_cluster_number to garbage memory which messes up the code.
	LS:
		-"ls .." since this uses "cd .." part of the function, it doesn't really
			work.
	CREAT:
		-CREAT can create file fine. However, when trying to remove the files
			created, it gives us some bugs because sometimes when changing to a
			different cluster once the current one is full, it'll give us errors.
	RM:
		-RM can remove the files regularly, however, removing the files created by
			CREAT, it messes up sometimes. It could be removed from bottom to top, but in
			random order, it messes up.(We believe the RM function should be fine and 
			that the problem lies within the CREAT function).
	MKDIR:
		-MKDIR is based off of CREAT. It has similar problems as CREAT to where if 
			you make enough directories. Trying to RMDIR them would cause problems because
			of the change in cluster numbers.
	RMDIR:
		-RMDIR is based off of RM. It has similar problem as RM where if RMDIR some of the
			directories in a random order would fail the other files. However, trying
			to remove from bottom to top would be fine.
Unfinished Portions:
	READ:
		-We did not get enough time to start on read, but was also confused on how to test
			with OFFSET and SIZE. 
	WRITE:
		-We did not get enough time to start on read, but was also confused on how to test
			with OFFSET and SIZE. 
