#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fatstruct.h"

char *filename = NULL, *sha1 = NULL;
// char *disk;

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
                if(op == 0) op = 1;
                else showUsage();
                break;
            case 'l': 
                if(op == 0) op = 2; 
                else showUsage();
                break;
            case 'r': 
                if(filename == NULL && (op == 0 || op == 5)) {
                    op = 3;
                    if((filename = (char*)optarg) == NULL) showUsage();
                }
                else showUsage();
                break;
            case 'R': 
                if(filename == NULL && (op == 0 || op == 5)) {
                    op = 4;
                    if((filename = (char*)optarg) == NULL) showUsage();
                }
                else showUsage();
                break;
            case 's':
                if(sha1 == NULL && (op == 0 || op == 3 || op == 4)) {
                    if(op == 0) op = 5;
                    if((sha1 = (char*)optarg) == NULL) showUsage();
                }
                else showUsage();
                break;
            default: showUsage();
        }
    }
    if(optind != argc-1 || op == 5) showUsage();
    else if(sha1 != NULL) op = 4;
    // disk = (char*)argv[optind];
    return op;
}

int main(int argc, char* argv[]){
    operation(argc, argv);
    return 0;
}

// docker run -i --name file-recovery --privileged --rm -t -v "C:\Users\mayan\Desktop\Git Repo\Sem 2\file-recovery:/file-recovery" -w /file-recovery ytang/os bash