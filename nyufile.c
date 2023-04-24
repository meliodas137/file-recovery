#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "fatstruct.h"

char *filename = NULL, *sha1 = NULL, *disk = NULL, *addr = NULL;
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
        }
    }
}

void showInfo(){
    printf("Number of FATs = %d\n", btEntry->BPB_NumFATs);
    printf("Number of bytes per sector = %d\n", btEntry->BPB_BytsPerSec);
    printf("Number of sectors per cluster = %d\n", btEntry->BPB_SecPerClus);
    printf("Number of reserved sectors = %d\n", btEntry->BPB_RsvdSecCnt);
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