/*
 * file.c -- ファイルアクセスモジュール
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/microdb.h"


/*
 * initializeFileModule -- ファイルアクセスモジュールの初期化処理
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result initializeFileModule()
{
    return OK;
}

/*
 * finalizeFileModule -- ファイルアクセスモジュールの終了処理
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result finalizeFileModule()
{
    return OK;
}

/*
 * createFile -- ファイルの作成
 *
 * 引数:
 *	filename: 作成するファイルのファイル名
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result createFile(char *filename)
{
    int fd;
    if((fd = creat(filename, S_IRUSR|S_IWUSR)) == -1){
        return NG;
    }
    close(fd);
    return OK;
}

/*
 * deleteFile -- ファイルの削除
 *
 * 引数:
 *	filename: 削除するファイルのファイル名
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result deleteFile(char *filename)
{
    if(unlink(filename)  == -1){
        return NG;
    }
    return OK;
}

/*
 * openFile -- ファイルのオープン
 *
 * 引数:
 *	filename: オープンしたいファイルのファイル名
 *
 * 返り値:
 *	オープンしたファイルのFile構造体
 *	オープンに失敗した場合にはNULLを返す
 */
File *openFile(char *filename)
{
    File *file;
    file = malloc(sizeof(File));

    strcpy(file->name, filename);

    if((file->desc = open(filename, O_RDWR)) == -1){
        return NULL;
    }

    return file;
}

/*
 * closeFile -- ファイルのクローズ
 *
 * 引数:
 *	クローズするファイルのFile構造体
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result closeFile(File *file)
{
    if(close(file->desc) == -1){
        return NG;
    }
    free(file);
    return OK;
}

/*
 * readPage -- 1ページ分のデータのファイルからの読み出し
 *
 * 引数:
 *	file: アクセスするファイルのFile構造体
 *	pageNum: 読み出すページの番号
 *	page: 読み出した内容を格納するPAGE_SIZEバイトの領域
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result readPage(File *file, int pageNum, char *page)
{
    if(lseek(file->desc, pageNum*PAGE_SIZE, SEEK_SET)  == -1){
        return NG;
    }
    if (read(file->desc, page, PAGE_SIZE) == -1) {
        return NG;
    }
    return OK;
}

/*
 * writePage -- 1ページ分のデータのファイルへの書き出し
 *
 * 引数:
 *	file: アクセスするファイルのFile構造体
 *	pageNum: 書き出すページの番号
 *	page: 書き出す内容を格納するPAGE_SIZEバイトの領域
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result writePage(File *file, int pageNum, char *page)
{
    if(lseek(file->desc, pageNum*PAGE_SIZE, SEEK_SET)  == -1){
        return NG;
    }
    if (write(file->desc, page, PAGE_SIZE) == -1) {
        return NG;
    }
    return OK;
}

/*
 * getNumPages -- ファイルのページ数の取得
 *
 * 引数:
 *	filename: ファイル名
 *
 * 返り値:
 *	引数で指定されたファイルの大きさ(ページ数)
 *	エラーの場合には-1を返す
 */
int getNumPages(char *filename)
{
    int pageCount;
    struct stat statBuf;
    int fileSize;

    if (stat(filename, &statBuf) == -1) {
        return -1;
    }

    if((fileSize = (int)statBuf.st_size) == 0){
        pageCount = 0;
    }else{
        pageCount = (fileSize-1)/PAGE_SIZE + 1;
    }

    return pageCount;
}
