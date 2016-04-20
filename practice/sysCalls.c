#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define PAGE_SIZE 256

int main(int argc, char const *argv[]) {
    char fname[] = "testfile";
    int fd;
    char buf[PAGE_SIZE];
    int size;
    struct stat statBuf;

    //ファイルの存在確認
    if(access(fname, F_OK) == 0){
        unlink(fname);
    }

    //ファイルの作成とエラー処理
    if((fd = open(fname, O_RDWR | O_CREAT, S_IRWXU)) == -1){
        printf("%sを作成できませんでした。\n", fname);
        exit(EXIT_FAILURE);
    }

    //Xを256バイト目まで書き込み
    memset(buf, 'X', PAGE_SIZE);
    if(write(fd, buf, PAGE_SIZE) == -1){
        printf("%sに書き込むことができませんでした。\n", fname);
        exit(EXIT_FAILURE);
    }

    //156バイト目に移動
    lseek(fd, 156, SEEK_SET);

    //testを書き込み
    strcpy(buf, "test");
    if(write(fd, buf, strlen(buf)) == -1){
        printf("%sに書き込むことができませんでした。\n", fname);
        exit(EXIT_FAILURE);
    }

    //152バイト目に移動
    lseek(fd, 152, SEEK_SET);
    size = 10;//読み込む文字数
    if(read(fd, buf, size) == -1){
        printf("%sから読み込むことができませんでした。\n", fname);
        exit(EXIT_FAILURE);
    }
    buf[size] = '\0';

    //読み込んだ内容を表示
    printf("%s\n", buf);

    //ファイルサイズの取得
    if(fstat(fd, &statBuf) == -1){
        printf("ファイルサイズを読み込むことができませんでした。\n");
        exit(EXIT_FAILURE);
    }

    printf("filesize %d\n", (int)statBuf.st_size);


    close(fd);
    return 0;
}
