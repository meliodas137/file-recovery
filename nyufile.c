/*
https://stackoverflow.com/questions/17602229/how-to-make-int-from-char4-in-c
https://www.includehelp.com/c-programs/extract-bytes-from-int.aspx
https://www.codeproject.com/Questions/5263050/How-to-convert-char-array-to-a-byte-array-in-C-pro
https://www.geeksforgeeks.org/check-whether-k-th-bit-set-not/#
*/ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <openssl/sha.h>

#include "fatstruct.h"

const unsigned char EMPTY_SHA[] = "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09";
char *filename = NULL, *disk = NULL;
unsigned char sha1[SHA_DIGEST_LENGTH], *addr = NULL;
unsigned int *fat = NULL;
struct BootEntry* btEntry;

void showUsage(){
    printf("Usage: ./nyufile disk <options>\n");
    printf("  -i                     Print the file system information.\n");
    printf("  -l                     List the root directory.\n");
    printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
    printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
    exit(0);
}

int hexToInt(char c)
   {
   switch(c)
      {
      case '0': return 0;
      case '1': return 1;
      case '2': return 2;
      case '3': return 3;
      case '4': return 4;
      case '5': return 5;
      case '6': return 6;
      case '7': return 7;
      case '8': return 8;
      case '9': return 9;
      case 'a': return 10;
      case 'b': return 11;
      case 'c': return 12;
      case 'd': return 13;
      case 'e': return 14;
      case 'f': return 15;
      }
   return -1;
   }
void fixSha1(unsigned char *sha1_temp){
    int i = 0, j = 0;
    while(sha1_temp[i] != '\0') {
        sha1[j] = hexToInt(sha1_temp[i+1]) + 16*(hexToInt(sha1_temp[i]));
        i += 2;
        j++;
    }
}

int operation(int argc, char* argv[]) {
    opterr = 0;
    unsigned char *sha1_temp = NULL;
    int ch, op = 0;
    while ((ch = getopt(argc, argv, "ilr:R:s:")) != -1) {
        switch (ch) {
            case 'i': 
                if(sha1_temp == NULL && op == 0) op = 1;
                else showUsage();
                break;
            case 'l': 
                if(sha1_temp == NULL && op == 0) op = 2; 
                else showUsage();
                break;
            case 'r': 
                if(filename == NULL && op == 0) {
                    op = 3;
                    if((filename = (char*)optarg) == NULL) showUsage();
                }
                else showUsage();
                break;
            case 'R': 
                if(filename == NULL && op == 0) {
                    op = 4;
                    if((filename = (char*)optarg) == NULL) showUsage();
                }
                else showUsage();
                break;
            case 's':
                if(sha1_temp == NULL) {
                    if((sha1_temp = (unsigned char*)optarg) == NULL) showUsage();
                }
                else showUsage();
                break;
            default: showUsage();
        }
    }
    if(optind != argc-1 || (op == 4 && sha1_temp == NULL) || op == 0) showUsage();
    if(sha1_temp != NULL) {
        op = 4;
        fixSha1(sha1_temp);
    }
    disk = (char*)argv[optind];
    return op;
}

void mapDisk(){
    int fd = open(disk, O_RDWR);
    if (fd != -1){
        struct stat sb;
        if (fstat(fd, &sb) != -1) {
            addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (addr == MAP_FAILED) exit(0);
            btEntry = (struct BootEntry*)addr;
            fat = (unsigned int *)(addr + (btEntry->BPB_RsvdSecCnt)*(btEntry->BPB_BytsPerSec));
        }
    }
}

void showInfo(){
    printf("Number of FATs = %d\n", btEntry->BPB_NumFATs);
    printf("Number of bytes per sector = %d\n", btEntry->BPB_BytsPerSec);
    printf("Number of sectors per cluster = %d\n", btEntry->BPB_SecPerClus);
    printf("Number of reserved sectors = %d\n", btEntry->BPB_RsvdSecCnt);
}

unsigned int getClusterPosition(unsigned int cluster){
    return (btEntry->BPB_RsvdSecCnt + (btEntry->BPB_FATSz32*btEntry->BPB_NumFATs) + (btEntry->BPB_SecPerClus)*(cluster - 2))*(btEntry->BPB_BytsPerSec);
}

unsigned int getStartCluster(struct DirEntry* dirEnt){
    unsigned short high = dirEnt->DIR_FstClusHI, low = dirEnt->DIR_FstClusLO;
    return (high >> 8 | low);
}

char* getName(unsigned char* dirName, int type){
    int i = 0;
    char* name = malloc(13);
    while(i<8){
        if(dirName[i] == 0x20) break;
        name[i] = dirName[i];
        i++;
    }
    if(type & (1 << 4)) {
        name[i] = '/';
        name[i+1] = '\0';
        return name;
    }
    int j = 8;
    while(j<11){
        if(dirName[j] == 0x20) break;
        if(j == 8) name[i++] = '.'; 
        name[i++] = dirName[j++];
    }
    name[i] = '\0';
    return name;
}

void printInfo(struct DirEntry* dirEnt){
    char *name = getName(dirEnt->DIR_Name, dirEnt->DIR_Attr);
    printf("%s", name);
    free(name);
    printf(" (");
    if(!(dirEnt->DIR_Attr & (1 << 4))) {
        if(dirEnt->DIR_FileSize == 0) {
            printf("size = 0)\n");
            return;
        }
        printf("size = %d, ", dirEnt->DIR_FileSize);
    }

    printf("starting cluster = %d)\n", getStartCluster(dirEnt));
}

void showRootDir(){
    unsigned int currClus = btEntry->BPB_RootClus;
    int count = 0, maxEntry = ((btEntry->BPB_SecPerClus)*(btEntry->BPB_BytsPerSec))/(sizeof(struct DirEntry));
    while(currClus < 0x0ffffff8) {
        int currCount = 0;
        unsigned int currPos = getClusterPosition(currClus);
        struct DirEntry* dirEnt;
        while(currCount < maxEntry && (dirEnt = (struct DirEntry*)(addr + currPos))->DIR_Name[0] != 0) {
            if(dirEnt->DIR_Name[0] != 0xe5) {
                printInfo(dirEnt);
                count++;
            }
            currCount++;
            currPos += sizeof(struct DirEntry);
        }
        currClus = fat[currClus];
    }
    printf("Total number of entries = %d\n", count);
    return;
}
void doRecovery(struct DirEntry* dirEnt, int useSHA){
    dirEnt->DIR_Name[0] = filename[0];
    if(useSHA == 1) printf("%s: successfully recovered with SHA-1\n", filename);
    else printf("%s: successfully recovered\n", filename);

    if(dirEnt->DIR_FileSize == 0) return;

    unsigned int clusters = (dirEnt->DIR_FileSize)/((btEntry->BPB_SecPerClus)*(btEntry->BPB_BytsPerSec));
    if(dirEnt->DIR_FileSize > clusters*(btEntry->BPB_SecPerClus)*(btEntry->BPB_BytsPerSec)) clusters++;
    
    unsigned int i = -1;
    while(++i < clusters - 1) fat[getStartCluster(dirEnt) + i] = getStartCluster(dirEnt) + i + 1; 
    fat[getStartCluster(dirEnt) + clusters - 1] = 0x0ffffff8;
    return;

}
void recoverFile(){
    unsigned int currClus = btEntry->BPB_RootClus;
    int maxEntry = ((btEntry->BPB_SecPerClus)*(btEntry->BPB_BytsPerSec))/(sizeof(struct DirEntry));

    struct DirEntry* delEntry = NULL;
    int multi = 0;
    while(currClus < 0x0ffffff8) {
        int currCount = 0;
        unsigned int currPos = getClusterPosition(currClus);
        struct DirEntry* dirEnt;
        while(currCount < maxEntry && (dirEnt = (struct DirEntry*)(addr + currPos))->DIR_Name[0] != 0) {
            char *name = getName(dirEnt->DIR_Name, dirEnt->DIR_Attr);
            if(dirEnt->DIR_Name[0] == 0xe5 && memcmp(name + 1, filename + 1, strlen(filename)) == 0) {
                if(delEntry == NULL) delEntry = dirEnt;
                else multi = 1;
            }
            free(name);
            currCount++;
            currPos += sizeof(struct DirEntry);
        }
        currClus = fat[currClus];
    }
    if(multi == 1) printf("%s: multiple candidates found\n", filename);
    else if(delEntry == NULL) printf("%s: file not found\n", filename);
    else doRecovery(delEntry, 0);
}

void recoverFileWithSHA(){
    unsigned int currClus = btEntry->BPB_RootClus;
    int maxEntry = ((btEntry->BPB_SecPerClus)*(btEntry->BPB_BytsPerSec))/(sizeof(struct DirEntry));

    while(currClus < 0x0ffffff8) {
        int currCount = 0;
        unsigned int currPos = getClusterPosition(currClus);
        struct DirEntry* dirEnt;
        while(currCount < maxEntry && (dirEnt = (struct DirEntry*)(addr + currPos))->DIR_Name[0] != 0) {
            char *name = getName(dirEnt->DIR_Name, dirEnt->DIR_Attr);
            if(dirEnt->DIR_Name[0] == 0xe5 && memcmp(name + 1, filename + 1, strlen(filename)) == 0) {
                unsigned char *md = malloc(SHA_DIGEST_LENGTH);
                SHA1(addr + getClusterPosition(getStartCluster(dirEnt)), dirEnt->DIR_FileSize, md);
                if(memcmp(sha1, md, SHA_DIGEST_LENGTH) == 0) {
                    doRecovery(dirEnt, 1);
                    free(md);
                    free(name);
                    return;
                }
                free(md);
            }
            free(name);
            currCount++;
            currPos += sizeof(struct DirEntry);
        }
        currClus = fat[currClus];
    }
    printf("%s: file not found\n", filename);
}

int main(int argc, char* argv[]){
    int op = operation(argc, argv);
    mapDisk(disk);
    switch (op)
    {
        case 1: showInfo(); break;
        case 2: showRootDir(); break;
        case 3: recoverFile(); break;
        case 4: recoverFileWithSHA(); break;
        default: break;
    }
    return 0;
}

// docker run -i --name file-recovery --privileged --rm -t -v "C:\Users\mayan\Desktop\Git Repo\Sem 2\file-recovery:/file-recovery" -w /file-recovery ytang/os bash