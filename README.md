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
		-Please enter correct amount of arguments when testing the function.
			This is because we did not have time to implement error checking
			if user had inputed incorrect amount of arguments.
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
Unfinished Portions:
	READ:

	WRITE:
