#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "fatstruct.h"

char *filename = NULL, *sha1 = NULL, *disk = NULL, *addr = NULL;
char *fat = NULL;
struct BootEntry* btEntry;

void showUsage(){
    printf("Usage: ./nyufile disk <options>\n");
    printf("  -i                     Print the file system information.\n");
    printf("  -l                     List the root directory.\n");
    printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
    printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
    exit(0);
}

int operation(int argc, char* argv[]) {
    opterr = 0;
    int ch, op = 0;
    while ((ch = getopt(argc, argv, "ilr:R:s:")) != -1) {
        switch (ch) {
            case 'i': 
                if(sha1 == NULL && op == 0) op = 1;
                else showUsage();
                break;
            case 'l': 
                if(sha1 == NULL && op == 0) op = 2; 
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
                if(sha1 == NULL) {
                    if((sha1 = (char*)optarg) == NULL) showUsage();
                }
                else showUsage();
                break;
            default: showUsage();
        }
    }
    if(optind != argc-1 || (op == 4 && sha1 == NULL) || op == 0) showUsage();
    if(sha1 != NULL) op = 4;
    disk = (char*)argv[optind];
    return op;
}

void mapDisk(){
    int fd = open(disk, O_RDWR);
    if (fd != -1){
        struct stat sb;
        if (fstat(fd, &sb) != -1) {
            addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
            if (addr == MAP_FAILED) exit(0);
            btEntry = (BootEntry*)addr;
            fat = (addr + (btEntry->BPB_RsvdSecCnt)*(btEntry->BPB_BytsPerSec));
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

unsigned int getNextCluster(unsigned int currClus){
    return *(fat + 4*currClus);
}

unsigned int getStartCluster(struct DirEntry* dirEnt){
    unsigned short high = dirEnt->DIR_FstClusHI, low = dirEnt->DIR_FstClusLO;
    char a[4];
    a[1] = (high & 0xFF);
    a[0] = ((high >> 8) & 0xFF);
    a[3] = ((low) & 0xFF);
    a[2] = ((low >> 8) & 0xFF); 

    return (a[0] | a[1] | a[2] | a[3]);
}

void printName(unsigned char* dirName, int type){
    int i = 0;
    while(i<8){
        if(dirName[i] == 0x20) break;
        printf("%c", dirName[i]);
        i++;
    }
    if(type == 0x10) {
        printf("/ ");
        return;
    }
    i = 8;
    while(i<12){
        if(dirName[i] == 0x20) break;
        if(i == 8) printf("."); 
        printf("%c", dirName[i]);
        i++;
    }
    printf(" ");
}

void printInfo(struct DirEntry* dirEnt){
    printName(dirEnt->DIR_Name, dirEnt->DIR_Attr);
    printf("(");
    if(dirEnt->DIR_Attr != 0x10) {
        if(dirEnt->DIR_FileSize == 0) {
            printf("size = 0)\n");
            return;
        }
        printf("size = %d, ", dirEnt->DIR_FileSize);
    }

    printf("starting cluster = %d)\n", getStartCluster(dirEnt));
}

int main(int argc, char* argv[]){
    int op = operation(argc, argv);
    mapDisk(disk);
    switch (op)
    {
        case 1: showInfo(); break;
        default: break;
    }
    return 0;
}

// docker run -i --name file-recovery --privileged --rm -t -v "C:\Users\mayan\Desktop\Git Repo\Sem 2\file-recovery:/file-recovery" -w /file-recovery ytang/os bash