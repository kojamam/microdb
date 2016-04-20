#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
    char fname[] = "testfile";

    //ファイルの存在確認
    if(access(fname, F_OK) == 0){
        unlink(fname);
    }

    //ファイルの作成とエラー処理
    if(open(fname, O_CREAT) == -1){
        printf("%sを作成できませんでした。\n", fname);
        exit(EXIT_FAILURE);
    }


    return 0;
}
