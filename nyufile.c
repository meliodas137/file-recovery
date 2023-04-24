#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

int main(int argc, char* argv[]){

}