/* fileモジュールのテストコード */

#include <stdio.h>
#include <string.h>
#include "../microdb.h"

#define TEST_FILE_NAME "sample"

int main(int argc, char const *argv[]) {
    Result res;
    File *file;
    char page[PAGE_SIZE] = "adc";

    initializeFileModule();

    createFile(TEST_FILE_NAME);
    printf("%d\n", getNumPages(TEST_FILE_NAME));

    file = openFile(TEST_FILE_NAME);

    res = writePage(file, 0, page);
    printf("writePage: %d\n", res);

    res = readPage(file, 0, page);
    printf("readPage: status->%d, read->%s\n", res, page);

    printf("%d\n", getNumPages(TEST_FILE_NAME));

    closeFile(file);
    // deleteFile(TEST_FILE_NAME);

    finalizeFileModule();
    return 0;
}
