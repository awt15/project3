#include "FileSystem.h"

FileSystem::FileSystem(string image_name) {
    /* open the FAT32 image */
    m_img.open(image_name.c_str(), ios::in | ios::out | ios::binary);
    
    /* Check to make sure image has been opened */
    if (!m_img.is_open() ) {
        cout << "error:  Failed to open " << image_name << ".\n";
        exit (EXIT_SUCCESS);
    }
    // read boot info from the image
    m_img.seekg(11);
    m_img.read((char*)&m_BPB.BytesPerSec, sizeof(m_BPB.BytesPerSec)); // BytesPerSec 11-12
    m_img.read((char*)&m_BPB.SecPerClus, sizeof(m_BPB.SecPerClus));   // SecPerClus 13
    m_img.read((char*)&m_BPB.RsvdSecCnt, sizeof(m_BPB.RsvdSecCnt));   // RsvdSecCnt 14-15
    m_img.read((char*)&m_BPB.NumFATs, sizeof(m_BPB.NumFATs));         // NumFATs 16
    m_img.read((char*)&m_BPB.RootEntCnt, sizeof(m_BPB.RootEntCnt));   // RootEntCnt 17-18

    m_img.seekg(32);
    m_img.read((char*)&m_BPB.TotalSec32, sizeof(m_BPB.TotalSec32));   // TotalSec32 32-35
    m_img.read((char*)&m_BPB.FATSz32, sizeof(m_BPB.FATSz32));         // FATSz32 36-39

    m_img.seekg(44);
    m_img.read((char*)&m_BPB.RootClus, sizeof(m_BPB.RootClus));       // RootClus 44-47

    // set pwd to root dir
    m_pwd = Cluster::ToDataOffset(2, m_BPB);
}

/*
* strrev()
* Description:
* Reverses a string
*/
char * FileSystem::strrev(char *str){
    char c, *front, *back;

    if(!str || !*str)
        return str;

    for(front = str, back = str + strlen(str) - 1; front < back; front++, back--){
        c = *front;
        *front = *back;
        *back = c;
    }
    return str;
}

/* 
* myitoa()
* Description:
* Custom itoa function turns an integer into a string.
*/
void FileSystem::myitoa(int v, char *buff, int radix_base){
    static char table[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char *p=buff;
    unsigned int n = (v < 0 && radix_base == 10)? -v : (unsigned int) v;
    while(n >= static_cast<unsigned int>(radix_base)){
        *p++ = table[n % radix_base];
        n /= radix_base;
    }

    *p++ = table[n];
    if(v < 0 && radix_base == 10) *p++ = '-';
    *p = '\0';
    strrev(buff);
}

/*
*  undelete()
*  Params:  none
*  Description: Restores any previously removed and recoverable file
* to the current working directory
* Recovered filenames shoudl be given the name "recovered_n" 
* where n is the nth file recovered
*/
void FileSystem::undelete(){
    int i, nameIndex;
    DirectoryEntry dir;
    vector<DirectoryEntry> contents;
    streamoff offset;
    uint32_t cluster;
    const string name = "UNDEL.";
    char nameSuffix[3];

    m_img.seekg(Cluster::ToDataOffset(DataOffset::ToCluster(m_pwd, m_BPB), m_BPB));
    for(i = 0; i < static_cast<int>((m_BPB.BytesPerSec / sizeof(dir))); ++i){
        offset = m_img.tellg();
        m_img.read((char*)&dir, sizeof(dir));
        
        // is it recoverable?
        if(((dir.Attr & (ATTR_ARCHIVE | ATTR_HIDDEN)) == (ATTR_ARCHIVE | ATTR_HIDDEN)) && ((dir.Attr & ATTR_LONG_NAME) != ATTR_LONG_NAME)){
            // is the cluster it used to use in use?
            m_img.seekg(Cluster::ToFATOffset((dir.FstClusHI << 16 | dir.FstClusLO), m_BPB));
            m_img.read((char*)&cluster, sizeof(cluster));
            if(cluster != UNUSED) continue;
            
            // find a name that will work
            contents = GetDirectoryContents(m_pwd);
            for(nameIndex = 1; nameIndex < 999; ++nameIndex) {
                
                // int to string without the \0 lazy workaround
                myitoa(nameIndex, nameSuffix, 10);

                if(nameIndex < 10) nameSuffix[1] = 0x20;
                if(nameIndex < 100) nameSuffix[2] = 0x20;
                
                // make sure the name doesn't exist already
                Directory::StringToName(name + nameSuffix, (char*)dir.Name);
                if(Directory::Find(Directory::NameToString(dir), contents, false) != -1) continue;
                break;
            }
            if(nameIndex == 999) { cout << "Unable to recover because all names are taken." << endl; return; }

            // recover
            for(int j = 0; j < m_BPB.NumFATs; ++j){
                m_img.seekp(Cluster::ToFATOffset((dir.FstClusHI << 16 | dir.FstClusLO), m_BPB) + (j * m_BPB.FATSz32));
                m_img.write((char*)&EOC, sizeof(EOC));
            }
            dir.Attr ^= ATTR_ARCHIVE | ATTR_HIDDEN;
            if(dir.FileSize > m_BPB.BytesPerSec) dir.FileSize = m_BPB.BytesPerSec;
            m_img.seekp(offset);
            m_img.write((char*)&dir, sizeof(dir));
            m_img.flush();
        }
        m_img.seekg(offset + sizeof(dir));
    }
}

/**
*
*  fsinfo()
*  Params:  None
*  Description:
*  Prints the following summary of file system values:
*       Bytes per sector:  m_BPB.BytesPerSec 
*       Sectors per cluster:  (int)m_BPB.SecPerClus
*       Total sectors:  m_BPB.TotalSec32
*       Number of FATs
*       Sectors per FAT
*       Number of free sectors
*/
void FileSystem::fsinfo() {
    uint32_t FAT_val;
    int freeClusters = 0;

    // count free clusters
    for(uint32_t i = m_BPB.RootClus; i < m_BPB.FATSz32; ++i){
        m_img.seekg(Cluster::ToFATOffset(i, m_BPB));
        m_img.read((char*)&FAT_val, sizeof(FAT_val));

        if(FAT_val == 0) ++freeClusters;
    }

    // output results
    cout << "Bytes per sector: "       << setw(9)  << m_BPB.BytesPerSec               << endl
         << "Sectors per cluster: "    << setw(4)  << (int)m_BPB.SecPerClus           << endl
         << "Total sectors: "          << setw(15) << m_BPB.TotalSec32                << endl
         << "Number of FATs: "         << setw(9)  << (int)m_BPB.NumFATs              << endl
         << "Sectors per FAT: "        << setw(11) << m_BPB.FATSz32                   << endl
         << "Number of free sectors: " << setw(3)  << freeClusters * m_BPB.SecPerClus << endl;
}

/*
*  cd()
*  Params:  string: dir_name
*  Description:  Changes the present working directory m_pwd
*  to the directory named dir_name
*  Fails if dir_name is not found and prints an error message
*  
*/
void FileSystem::cd(const string dir_name) {
    int i;
    uint32_t cluster = 0;
    vector<DirectoryEntry> contents;

    // if the directory exists, change to it
    contents = GetDirectoryContents(m_pwd);
    if((i = Directory::Find(dir_name, contents, true)) != -1){
        if((cluster = ((cluster | contents[i].FstClusHI) << 16) | contents[i].FstClusLO) == 0) cluster = 2;
        m_pwd = Cluster::ToDataOffset(cluster, m_BPB);

        /* Handle the currentWorkingDirectory path */
        if ( dir_name.compare( ".." ) == 0) {

            /* Make sure we are not in the root */
            if (currentDirectory.size() > 0 ){
                currentDirectory.pop_back();
            }
        } else {

            /* only add the dir_name if not a "." */
            if ( dir_name.compare( "." ) != 0) {
                currentDirectory.push_back(dir_name);
            } 
        }
    }
    else cout << dir_name << ": no such directory." << endl;

}

/*
* ls()
* Params:  none
* Description:
* Prints all entries in the directory dir_name to the terminal
*
*/
void FileSystem::ls(const string dir_name)  {
    int i;
    uint32_t cluster = 0;
    vector<DirectoryEntry> contents;

    // if the directory exists, get its contents
    contents = GetDirectoryContents(m_pwd);
    if((i = Directory::Find(dir_name, contents, true)) != -1){
        if((cluster = ((cluster | contents[i].FstClusHI) << 16) | contents[i].FstClusLO) == 0) cluster = 2;
        contents = GetDirectoryContents(Cluster::ToDataOffset(cluster, m_BPB));
    }
    else { cout << dir_name << ": no such directory." << endl; return; }

    // output contents
    for(i = 0; i < (int)contents.size(); ++i) {
        cout << Directory::NameToString(contents[i]) << "   ";
    }
    if(i != 0) cout << endl;
}

/*
* rm()
* Params:  string:file_name
* Description:  deletes the file named file_name from the present directory
* This only removes the link from a directory to a file, not the data.  (file 
* no longer shows in ls and the space is reclaimed).
* Unlinks file reference from all FATs
* Fails if a file named file_name is not found or is not a file
*/
void FileSystem::rm(const string file_name) {
    int i;
    uint32_t cluster, nextCluster;
    DirectoryEntry dir;
    vector<DirectoryEntry> contents;

    // find the file in the directory
    contents = GetDirectoryContents(m_pwd);
    if((i = Directory::Find(file_name, contents, false)) == -1){cout << file_name << ": is not a file or does not exist in the present directory." << endl; return; }

    // free all FAT entries this file used
    cluster = ((contents[i].FstClusHI << 16) | contents[i].FstClusLO);
    do {
        // read link to next cluster
        m_img.seekg(Cluster::ToFATOffset(cluster, m_BPB));
        m_img.read((char*)&nextCluster, sizeof(nextCluster));

        // remove link to next cluster
        for(int i = 0; i < m_BPB.NumFATs; ++i){
            m_img.seekp(Cluster::ToFATOffset(cluster, m_BPB) + (i * m_BPB.FATSz32));
            m_img.write((char*)&UNUSED, sizeof(UNUSED));
        }

        cluster = nextCluster;
    } while(cluster != 0x0FFFFFF8 && cluster != 0x0FFFFFFF);

    // archive file in directory
    m_img.seekp(SeekToDirEntry(m_pwd, file_name));
    dir = contents[i];
    dir.Name[0] = DIR_FREE_ENTRY;
    dir.Attr = ATTR_ARCHIVE | ATTR_HIDDEN;
    m_img.write((char*)&dir, sizeof(dir));
    m_img.flush();
}

/*
*  mkdir()
*  Params:  string:dir_name
*  Description:  
*  Creates a direcotry named dir_name
*  Fails if a file or directory named dir_name already exists
*  Prints error message to terminal on failure.
*/
void FileSystem::mkdir(const string dir_name) {
    DirectoryEntry dir;
    streamoff dirOffset;
    uint32_t cluster = 0;
    vector<DirectoryEntry> contents;
    char name[1] = { (char)0x00 };

    // make sure something isn't already named that in the directory
    contents = GetDirectoryContents(m_pwd);
    if(Directory::Find(dir_name, contents, true)  != -1) { cout << dir_name << ": a directory with that name already exists" << endl; return; }
    if(Directory::Find(dir_name, contents, false) != -1) { cout << dir_name << ": a file with that name already exists."     << endl; return; }

    // make room in directory for the directory if there isn't
    dirOffset = SeekToDirEntry(m_pwd, "");
    if(dirOffset == 0) if((dirOffset = ExpandDirectory(m_pwd)) == 0) return;

    // find free cluster for the directory
    if((cluster = FindFreeCluster()) == 0) { cout << "No clusters are available to write to." << endl; return; }
    AllocateCluster(cluster);

    // create the directory entry in current directory
    Directory::Initialize(dir, dir_name, cluster, true);
    m_img.seekp(dirOffset);
    m_img.write((char*)&dir, sizeof(dir));

    // create the . and .. entries in the new directory
    m_img.seekp(Cluster::ToDataOffset(cluster, m_BPB));
    Directory::Initialize(dir, ".", cluster, true);
    m_img.write((char*)&dir, sizeof(dir));
    Directory::Initialize(dir, "..", DataOffset::ToCluster(dirOffset, m_BPB), true);
    m_img.write((char*)&dir, sizeof(dir));
    
    // fill the rest of the entries
    Directory::Initialize(dir, name, 0, false);
    m_img.write((char*)&dir, sizeof(dir));
    name[0] = (char)0xE5;
    for(uint32_t i = 3; i < (m_BPB.BytesPerSec / sizeof(dir)); ++i){
        m_img.write((char*)&dir, sizeof(dir));
    }
    m_img.flush();
}

/*
* rmdir()
* Params: string:dir_name
* Removes the empty direcotry dir_name from teh present working directory
* Fails if dir_name does not exists or dir_name is not empty
* Prints error message to terminal on failure
*/
void FileSystem::rmdir(const string dir_name) {
    int i, j;
    DirectoryEntry dir;
    uint32_t cluster = 0, nextCluster;
    vector<DirectoryEntry> contents;

    // find the file in the directory
    contents = GetDirectoryContents(m_pwd);
    if((i = Directory::Find(dir_name, contents, true)) == -1) { cout << dir_name << ": directory does not exist in the present directory." << endl; return; }

    // get the target directory contents
    if((cluster = ((contents[i].FstClusHI << 16) | contents[i].FstClusLO)) == 0) cluster = 2;
    contents = GetDirectoryContents(Cluster::ToDataOffset(cluster, m_BPB));

    // make sure the target directory is empty
    for(j = 0; j < (int)contents.size(); ++j) {
        if(Directory::NameToString(contents[j]).compare(".") != 0 && Directory::NameToString(contents[j]).compare("..") != 0) {
            cout << dir_name << ": is not an empty directory." << endl;
            return;
        }
    }

    // archive . and ..
    for(j = 0; j < 2; ++j) {
        m_img.seekg(Cluster::ToDataOffset(cluster, m_BPB));
        m_img.read((char*)&dir, sizeof(dir));

        dir.Attr |= ATTR_HIDDEN | ATTR_ARCHIVE;
        dir.Name[0] = DIR_FREE_ENTRY;

        m_img.seekp(Cluster::ToDataOffset(cluster, m_BPB));
        m_img.write((char*)&dir, sizeof(dir));
    }

    // free all FAT entries this directory used
    do {
        // read link to next cluster
        m_img.seekg(Cluster::ToFATOffset(cluster, m_BPB));
        m_img.read((char*)&nextCluster, sizeof(nextCluster));

        // remove link to next cluster
        for(int i = 0; i < m_BPB.NumFATs; ++i){
            m_img.seekp(Cluster::ToFATOffset(cluster, m_BPB) + (i * m_BPB.FATSz32));
            m_img.write((char*)&UNUSED, sizeof(UNUSED));
        }

        cluster = nextCluster;
    } while(cluster != 0x0FFFFFF8 && cluster != 0x0FFFFFFF);
    
    // archive directory in directory
    contents = GetDirectoryContents(m_pwd);
    m_img.seekp(SeekToDirEntry(m_pwd, dir_name));
    dir = contents[i];
    dir.Name[0] = DIR_FREE_ENTRY;
    dir.Attr = ATTR_HIDDEN | ATTR_ARCHIVE | ATTR_DIRECTORY;
    m_img.write((char*)&dir, sizeof(dir));
    m_img.flush();
}

/*
* create()
* Params:   string: file_name
* Description:  Creates a file with 0-bytes of data
* and the name file_name in the present working directory
* Fails if file_name already exists and prints an error
* message to the terminal
*/
void FileSystem::create(const string file_name) {
    DirectoryEntry dir;
    streamoff dirOffset;
    uint32_t cluster = 0;
    vector<DirectoryEntry> contents;

    // make sure something isn't already named that in the directory
    contents = GetDirectoryContents(m_pwd);
    if(Directory::Find(file_name, contents, false) != -1) { cout << file_name << ": a file with that name already exists."    << endl; return; }
    if(Directory::Find(file_name, contents, true)  != -1) { cout << file_name << ": directory with that name already exists." << endl; return; }

    // make room in directory for the file if there isn't
    dirOffset = SeekToDirEntry(m_pwd, "");
    if(dirOffset == 0) if((dirOffset = ExpandDirectory(m_pwd)) == 0) return;

    // find free cluster for the file
    if((cluster = FindFreeCluster()) == 0) { cout << "No clusters are available to write to." << endl; return; }
    AllocateCluster(cluster);

    // initialize DirectoryEntry
    Directory::Initialize(dir, file_name, cluster, false);

    // create the file
    m_img.seekp(dirOffset);
    m_img.write((char*)&dir, sizeof(dir));
    m_img.flush();
}

/*
* close()
* Params:  string file_name
* Description:
* Removes file_name from the open file table.
* Returns 1 if successful, 0 if unsuccessful.
*
*/
void FileSystem::close(const string file_name) {
    for(vector<File>::iterator it = m_fileTable.begin(); it != m_fileTable.end(); ++it){
        if(file_name.compare(it->name) == 0){
            m_fileTable.erase(it);
            cout << file_name << " is now closed." << endl;
            return;
        }
    }
    cout << file_name << ": could not be found in the open file table." << endl;
}

/*
*  size()
*  Params:  string:entry_name
*  Description:
*  Prints the size in bytes of entry_name
*/
void FileSystem::size(const string entry_name) {
    int i;
    vector<DirectoryEntry> contents;

    // if the file exists, output its size
    contents = GetDirectoryContents(m_pwd);
    if((i = Directory::Find(entry_name, contents, false)) != -1) cout << contents[i].FileSize << " bytes" << endl;
    else                                                         cout << entry_name << ": no such  file." << endl;
}

/*
*  Open()
*  Params: string:file_name, string:mode
*  Description:
*  Opens a file given by filename withone of the following modes:
*  "r": read only mode
*  "w": write-only mode
*  "rw": read-write
*  After a successful open call, the file is placed in the open-file table
*
*  Return 1 if success, else return 0 and print error message.
*/
void FileSystem::open(const string file_name, const string mode) {
    int i, j;
    uint32_t cluster = 0;
    vector<DirectoryEntry> contents;

    // find the file in the directory
    contents = GetDirectoryContents(m_pwd);
    if((i = Directory::Find(file_name, contents, true))  != -1) { cout << "error: " << file_name << " cannot open a directory." << endl; return; }
    if((i = Directory::Find(file_name, contents, false)) == -1) { cout << "error: " << file_name << " does not exist."          << endl; return; }

    // make sure it isn't already open
    for(j = 0; j < (int)m_fileTable.size(); ++j){
        if(m_fileTable[j].name == file_name) {
            cout << "error: " << file_name << " already open." << endl;
            return;
        }
    }

    // add it to the open-file table
    cluster = ((contents[i].FstClusHI << 16) | contents[i].FstClusLO);
    m_fileTable.push_back(File(file_name, mode, cluster, contents[i].FileSize));
    cout << file_name << " has been opened with " << mode << " permission." << endl;
}

/*
*   write( ) 
*   Params:  string:file_name, uint:strat_pos, string:quoted_data
*   Description:
*   Writes contenst of quoted_data to file_name starting at start_pos
*   On sucess, prints to terminal
*   Wrote <quoted_data> to <start_pos>:<file_name> of length <data_length>
*   If start_pos is outisde bounds of file, grow file to make start_pos valid
*   Fails if file_name is not in open file talbe or not open for write
*/
void FileSystem::write(const string file_name, const unsigned int start_pos, const string quoted_data) {
    DirectoryEntry dir;

    /* WARNING that we don't use "i" or "newsize" so I commented them out */
    /* int i = 1, newSize = 0; */
    uint32_t nextCluster = 0, newCluster = 0, temp, bufferPos, numLeftToWrite, numToWrite;

    // make sure it's been opened
    for(vector<File>::iterator it = m_fileTable.begin(); it != m_fileTable.end(); ++it) {
        if(file_name.compare(it->name) == 0){
            // check for the right permissions
            if(it->permissions.find("w") == string::npos){  cout << "error: " << file_name << " is not open for writing." << endl; return; }

            // find the cluster start_pos is in
            m_img.seekg(Cluster::ToFATOffset((nextCluster = it->cluster), m_BPB));
            for(bufferPos = start_pos; bufferPos >= m_BPB.BytesPerSec; bufferPos -= m_BPB.BytesPerSec){
                temp = nextCluster;
                m_img.read((char*)&nextCluster, sizeof(nextCluster));
                
                // allocate new clusters if needed to get to start_pos
                if(nextCluster == EOC) {
                    // find free cluster for the file
                    if((newCluster = FindFreeCluster()) == 0) { cout << "No clusters are available to write to." << endl; return; }
                    AllocateCluster(newCluster);
                
                    // change the FAT entries to the new cluster
                    for(int i = 0; i < m_BPB.NumFATs; ++i){
                        m_img.seekp(Cluster::ToFATOffset(temp, m_BPB) + (i * m_BPB.FATSz32));
                        m_img.write((char*)&newCluster, sizeof(newCluster));
                        m_img.flush();
                    }
                    nextCluster = newCluster;
                }
                m_img.seekg(Cluster::ToFATOffset(nextCluster, m_BPB));
            }

            // how many bytes do we need to read
            numLeftToWrite = quoted_data.length();
        
            // start writing
            m_img.seekp(Cluster::ToDataOffset(nextCluster, m_BPB) + bufferPos);
            for(int stringPos = 0; numLeftToWrite > 0; bufferPos = 0){
                // how many can bytes we read in this cluster
                if(bufferPos + numLeftToWrite >= m_BPB.BytesPerSec) numToWrite = m_BPB.BytesPerSec - bufferPos;
                else                                                numToWrite = numLeftToWrite;
                numLeftToWrite -= numToWrite;
            
                // write
                for(int i = 0; i < (int)numToWrite; ++i) m_img.put(quoted_data[stringPos++]);
                m_img.flush();

                // go to next cluster
                temp = nextCluster;
                m_img.seekg(Cluster::ToFATOffset(nextCluster, m_BPB));
                m_img.read((char*)&nextCluster, sizeof(nextCluster));
                
                // allocate new cluster if needed
                if(numLeftToWrite > 0 && nextCluster == EOC) {
                    // find free cluster for the file
                    if((newCluster = FindFreeCluster()) == 0) { cout << "No clusters are available to write to." << endl; return; }
                    AllocateCluster(newCluster);

                    // change the FAT entries to the new cluster
                    for(int i = 0; i < m_BPB.NumFATs; ++i){
                        m_img.seekp(Cluster::ToFATOffset(temp, m_BPB) + (i * m_BPB.FATSz32));
                        m_img.write((char*)&newCluster, sizeof(newCluster));
                        m_img.flush();
                    }
                    nextCluster = newCluster;
                }
                m_img.seekp(Cluster::ToDataOffset(nextCluster, m_BPB));
            }

            // update file size
            m_img.seekg(SeekToDirEntry(m_pwd, it->name));
            m_img.read((char*)&dir, sizeof(dir));
            dir.FileSize = (it->size = quoted_data.length() + start_pos);
            m_img.seekp(SeekToDirEntry(m_pwd, it->name));
            m_img.write((char*)&dir, sizeof(dir));
            m_img.flush();

            cout << "Wrote " << quoted_data << " to " << start_pos << ":" << file_name << " of length " << quoted_data.length() + start_pos << endl;
            return;
        }
    }
    cout << "error: " << file_name << " is not in the open file table." << endl;
}

/*
*   read()
*   Params: string:file_name, uint:start_pos, uint:num_bytes
*   Description:
*   Reads num_bytes starting at start_pos from file_name and prints them to the terminal. 
*   If there are less bytes to read than num_bytes, print remaining data
*   Fails if file_name is not in open file table, not open for reading, not a file
*   or if start_pos is greater than the size of the file file_name
*/
void FileSystem::read(const string file_name, const unsigned int start_pos, const unsigned int num_bytes) {
    string readBytes = "";
    vector<DirectoryEntry> contents;
    uint32_t nextCluster = 0, numToRead, numLeftToRead, bufferPos;

    // make sure they aren't trying to read a directory
    contents = GetDirectoryContents(m_pwd);
    if(Directory::Find(file_name, contents, true) != -1){
        cout << "error: " << file_name << ": no such file." << endl;
        return;
    }

    // make sure it's been opened
    for(vector<File>::iterator it = m_fileTable.begin(); it != m_fileTable.end(); ++it) {
        if(file_name.compare(it->name) == 0){
            // error checking
            if(it->permissions.find("r") == string::npos){ cout << "error: " << file_name << " is not open for reading."                  << endl; return; }
            if((int)start_pos > it->size) {                cout << "error: " << start_pos << " is greater than the size of " << file_name << endl; return; }

            // find the cluster start_pos is in
            m_img.seekg(Cluster::ToFATOffset((nextCluster = it->cluster), m_BPB));
            for(bufferPos = start_pos; bufferPos >= m_BPB.BytesPerSec; bufferPos -= m_BPB.BytesPerSec){
                m_img.read((char*)&nextCluster, sizeof(nextCluster));
                m_img.seekg(Cluster::ToFATOffset(nextCluster, m_BPB));
            }

            // how many bytes do we need to read
            numLeftToRead = (int)(start_pos + num_bytes) >= it->size ? it->size - start_pos : num_bytes;

            // start reading
            m_img.seekg(Cluster::ToDataOffset(nextCluster, m_BPB) + bufferPos);
            for(;numLeftToRead > 0; numLeftToRead -= numToRead){
                // how many can bytes we read in this cluster
                if(bufferPos + numLeftToRead >= m_BPB.BytesPerSec) numToRead = m_BPB.BytesPerSec - bufferPos;
                else                                               numToRead = numLeftToRead;

                // read
                for(int i = 0; i < (int)numToRead; ++i) readBytes += m_img.get();
                
                // go to next cluster
                m_img.seekg(Cluster::ToFATOffset(nextCluster, m_BPB));
                m_img.read((char*)&nextCluster, sizeof(nextCluster));
                m_img.seekg(Cluster::ToDataOffset(nextCluster, m_BPB));
                bufferPos = 0;
            }
            cout << readBytes << endl;
            return;
        }
    }
    cout << "error: " << file_name << " is not in the open file table." << endl;
}

vector<DirectoryEntry> FileSystem::GetDirectoryContents(const uint32_t offset) {
    DirectoryEntry dir;
    uint32_t nextClusterNum;
    vector<DirectoryEntry> contents;

    // set original cluster
    nextClusterNum = DataOffset::ToCluster(offset, m_BPB);

    // output for each cluster of the directory
    do {
        m_img.seekg(Cluster::ToDataOffset(nextClusterNum, m_BPB));

        // add this entry to contents if it isn't hidden
        m_img.read((char*)&dir, sizeof(dir));
        for(uint32_t i = 0; i < (m_BPB.BytesPerSec / sizeof(dir)); ++i) {
            // if this is the last allocated entry, leave the loop
            if(dir.Name[0] == 0x00 && (dir.Attr & ATTR_HIDDEN) != ATTR_HIDDEN) break;

            // if this isn't hidden and isn't empty, add it to the list
            if((dir.Attr & ATTR_HIDDEN) != ATTR_HIDDEN && dir.Name[0] != DIR_FREE_ENTRY) {
                contents.push_back(dir);
            }
            m_img.read((char*)&dir, sizeof(dir));
        }

        // see if we need to go to next cluster
        m_img.seekg(Cluster::ToFATOffset(nextClusterNum, m_BPB)); 
        m_img.read((char*)&nextClusterNum, sizeof(nextClusterNum));
    } while(nextClusterNum != 0x0FFFFFF8 && nextClusterNum != 0x0FFFFFFF);

    return contents;
}
uint32_t FileSystem::FindFreeCluster(){
    static uint32_t lastFreeCluster = m_BPB.RootClus;
    uint32_t i, result, FAT_val = 0;

    for(i = lastFreeCluster; i < (m_BPB.FATSz32 / m_BPB.SecPerClus); ++i){
        m_img.seekg(Cluster::ToFATOffset(i, m_BPB));
        m_img.read((char*)&FAT_val, sizeof(FAT_val));

        if(FAT_val == 0) return lastFreeCluster = i;
    }

    // search what we didn't search if there was nothing free
    result = FindFreeCluster(m_BPB.RootClus, lastFreeCluster);
    lastFreeCluster = result == 0 ? m_BPB.RootClus : result;
    return result;
}
uint32_t FileSystem::FindFreeCluster(const uint32_t& start_pos, const uint32_t& end_pos) {
    uint32_t i, FAT_val = 0;

    for(i = start_pos; i < end_pos; ++i){
        m_img.seekg(Cluster::ToFATOffset(i, m_BPB));
        m_img.read((char*)&FAT_val, sizeof(FAT_val));

        if(FAT_val == 0) return i;
    }

    return 0;
}
void FileSystem::AllocateCluster(const uint32_t& cluster){
    for(int i = 0; i < m_BPB.NumFATs; ++i){
        m_img.seekp(Cluster::ToFATOffset(cluster, m_BPB) + (i * m_BPB.FATSz32));
        m_img.write((char*)&EOC, sizeof(EOC));
        m_img.flush();
    }
}
streamoff FileSystem::SeekToDirEntry(const uint32_t& offset, const string& name) {
    DirectoryEntry dir;
    uint32_t i, nextClusterNum;

    // set original cluster
    nextClusterNum = DataOffset::ToCluster(offset, m_BPB);

    // search each cluster of directory until we find what we're looking for
    do {
        m_img.seekg(Cluster::ToDataOffset(nextClusterNum, m_BPB));

        // is this what we're looking for?
        for(i = 0; i < (m_BPB.BytesPerSec / sizeof(dir)); ++i){
            m_img.read((char*)&dir, sizeof(dir));
            if((dir.Name[0] == 0xE5 || dir.Name[0] == 0x00) && name.compare("") == 0) return m_img.tellg() - (streamoff)sizeof(dir);
            if(name.compare(Directory::NameToString(dir)) == 0)                       return m_img.tellg() - (streamoff)sizeof(dir);
        }

        // check next cluster if there is one
        m_img.seekg(Cluster::ToFATOffset(nextClusterNum, m_BPB)); 
        m_img.read((char*)&nextClusterNum, sizeof(nextClusterNum));
    } while(nextClusterNum != 0x0FFFFFF8 && nextClusterNum != 0x0FFFFFFF);

    return 0;
}
uint32_t FileSystem::ExpandDirectory(const uint32_t& offset) {
    int freeClusterNum, clusterNum;

    if((freeClusterNum = FindFreeCluster()) == -1){
        cout << "No clusters are available to write to." << endl;
        return 0;
    }
            
    // find the last cluster this directory uses
    clusterNum = DataOffset::ToCluster(m_pwd, m_BPB);
    do {
        m_img.seekg(Cluster::ToFATOffset(clusterNum, m_BPB));
        m_img.read((char*)&clusterNum, sizeof(clusterNum));
    } while(clusterNum != 0x0FFFFFF8 && clusterNum != 0x0FFFFFFF);

    // update FAT
    for(int i = 0; i < m_BPB.NumFATs; ++i){
        m_img.seekg(Cluster::ToFATOffset(clusterNum, m_BPB) + (i * m_BPB.FATSz32));
        m_img.write((char*)&freeClusterNum, sizeof(freeClusterNum));
    }
    AllocateCluster(freeClusterNum);

    return Cluster::ToDataOffset(freeClusterNum, m_BPB);
}
string FileSystem::getCurrentWorkingDirectory( ) {
    string startPath = "/";

    /* iterate through the vector and build the current path */
    for (int i = 0;  i < (int)currentDirectory.size(); ++i){
            startPath += currentDirectory[i] + "/";
    }

    return startPath;

}