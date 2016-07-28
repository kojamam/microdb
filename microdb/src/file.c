/*
 * file.c -- ファイルアクセスモジュール
 */

#include "../include/microdb.h"

/*
 * NUM_BUFFER -- ファイルアクセスモジュールが管理するバッファの大きさ(ページ数)
 */
#define NUM_BUFFER 4

/*------バッファ-------*/
/*
 * modifyFlag -- 変更フラグ
 */
typedef enum { UNMODIFIED = 0, MODIFIED = 1 } modifyFlag;

/*
 * Buffer -- 1ページ分のバッファを記憶する構造体
 */
typedef struct Buffer Buffer;
struct Buffer {
    File *file;				/* バッファの内容が格納されたファイル */
    /* file == NULLならこのバッファは未使用 */
    int pageNum;			/* ページ番号 */
    char page[PAGE_SIZE];		/* ページの内容を格納する配列 */
    struct Buffer *prev;		/* 一つ前のバッファへのポインタ */
    struct Buffer *next;		/* 一つ後ろのバッファへのポインタ */
    modifyFlag modified;		/* ページの内容が更新されたかどうかを示すフラグ */
};

/*
 * bufferListHead -- LRUリストの先頭へのポインタ
 */
static Buffer *bufferListHead = NULL;

/*
 * bufferListTail -- LRUリストの最後へのポインタ
 */
static Buffer *bufferListTail = NULL;

/*
 * initializeBufferList -- バッファリストの初期化
 *
 * **注意**
 *	この関数は、ファイルアクセスモジュールを使用する前に必ず一度だけ呼び出すこと。
 *	(initializeFileModule()から呼び出すこと。)
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	初期化に成功すればOK、失敗すればNGを返す。
 */
static Result initializeBufferList()
{
    Buffer *oldBuf = NULL;
    Buffer *buf;
    int i;
    
    /*
     * NUM_BUFFER個分のバッファを用意し、
     * ポインタをつないで両方向リストにする
     */
    for (i = 0; i < NUM_BUFFER; i++) {
        /* 1個分のバッファ(Buffer構造体)のメモリ領域の確保 */
        if ((buf = (Buffer *) malloc(sizeof(Buffer))) == NULL) {
            /* メモリ不足なのでエラーを返す */
            return NG;
        }
        
        /* Buffer構造体の初期化 */
        buf->file = NULL;
        buf->pageNum = -1;
        buf->modified = UNMODIFIED;
        memset(buf->page, 0, PAGE_SIZE);
        buf->prev = NULL;
        buf->next = NULL;
        
        /* ポインタをつないで両方向リストにする */
        if (oldBuf != NULL) {
            oldBuf->next = buf;
        }
        buf->prev = oldBuf;
        
        /* リストの一番最初の要素へのポインタを保存 */
        if (buf->prev == NULL) {
            bufferListHead = buf;
        }
        
        /* リストの一番最後の要素へのポインタを保存 */
        if (i == NUM_BUFFER - 1) {
            bufferListTail = buf;
        }
        
        /* 次のループのために保存 */
        oldBuf = buf;
    }
    
    return OK;
}

/*
 * finalizeBufferList -- バッファリストの終了処理
 *
 * **注意**
 *	この関数は、ファイルアクセスモジュールの使用後に必ず一度だけ呼び出すこと。
 *	(finalizeFileModule()から呼び出すこと。)
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	終了処理に成功すればOK、失敗すればNGを返す。
 */
static Result finalizeBufferList()
{
    /* ?????? */
    return OK;
}

/*
 * moveBufferToListHead -- バッファをリストの先頭へ移動
 *
 * 引数:
 *	buf: リストの先頭に移動させるバッファへのポインタ
 *
 * 返り値:
 *	なし
 */
static void moveBufferToListHead(Buffer *buf)
{
    /* 自分の前後と先頭を保存 */
    Buffer  *oldPrev = buf->prev,
            *oldNext = buf->next,
            *oldHead = bufferListHead;
    
    /* 今先頭にないなら移動させる */
    if(bufferListHead != buf){
        /* 自分が末端なら変更しておく */
        if (bufferListTail == buf) {
            bufferListTail = oldPrev;
        }else{
            /* 末端でないときのみoldNextがNULLでないので設定 */
            oldNext->prev = oldPrev;
        }
        /* oldPrevのnextを設定 */
        oldPrev->next = oldNext;
        
        /* 先頭に移動 */
        bufferListHead = buf;
        buf->next = oldHead;
        buf->prev = NULL;
        oldHead->prev = buf;
        
    }
}

/*-------ファイルモジュール本体--------*/


/*
 * initializeFileModule -- ファイルアクセスモジュールの初期化処理
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result initializeFileModule(){
    initializeBufferList();
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
Result finalizeFileModule(){
    finalizeBufferList();
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
Result createFile(char *filename){
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
Result deleteFile(char *filename){
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
File *openFile(char *filename){
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
Result closeFile(File *file){
    
    Buffer *buf;
    
    for (buf = bufferListHead; buf != NULL; buf = buf->next) {
        /*クローズするファイルのバッファであるとき*/
        if(file == buf->file){
            if(buf->modified == MODIFIED){
                /* 変更フラグが立っていたらバッファをファイルに書き戻す */
                if(lseek(buf->file->desc, buf->pageNum*PAGE_SIZE, SEEK_SET)  == -1){
                    return NG;
                }
                if (write(buf->file->desc, buf->page, PAGE_SIZE) == -1) {
                    return NG;
                }
            }
            /*バッファを空にする*/
            buf->file = NULL;
            buf->pageNum = 0;
            buf->modified = UNMODIFIED;
            memset(buf->page, 0, PAGE_SIZE);
        }
    }
    
    
    /* ファイルのクローズ */
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
Result readPage(File *file, int pageNum, char *page){
    
    Buffer *buf, *emptyBuf = NULL;
    
    /*
     * 読み出しを要求されたページがバッファに保存されているかどうか、
     * リストの先頭から順に探す
     */
    for (buf = bufferListHead; buf != NULL; buf = buf->next) {
        /* 要求されたページがリストの中にあるかどうかチェックする */
        if (buf->file == file && buf->pageNum == pageNum) {
            /* 要求されたページがバッファにあったので、その内容を引数のpageにコピーする */
            memcpy(page, buf->page, PAGE_SIZE);
            
            /* アクセスされたバッファを、リストの先頭に移動させる */
            moveBufferToListHead(buf);
            
            return OK;
        }
    }

    /* バッファの空きを探す */
    for (buf = bufferListHead; buf != NULL; buf = buf->next) {
        /* 途中で空きバッファが見つかったらそれをemptyBufに */
        if(buf->file == NULL){
            emptyBuf = buf;
            break;
        }
        
        /* 最後まで空きバッファが見つからなかったら */
        if(buf->next == NULL && buf->file != NULL){
            /* 末端バッファをファイルに書き戻す */
            if(lseek(buf->file->desc, buf->pageNum*PAGE_SIZE, SEEK_SET)  == -1){
                return NG;
            }
            if (write(buf->file->desc, buf->page, PAGE_SIZE) == -1) {
                return NG;
            }
            
            /* 末端バッファを初期化してemptyBufに */
            buf->file = NULL;
            buf->pageNum = -1;
            memset(buf->page, 0, PAGE_SIZE);
            
            emptyBuf = buf;
            
        }
    }
    
    /*
     * lseekとreadシステムコールで空きバッファにファイルの内容を読み込み、
     * Buffer構造体に保存する
     */
    
    /* 読み出し位置の設定 */
    if (lseek(file->desc, pageNum * PAGE_SIZE, SEEK_SET) == -1) {
        return NG;
    }
    
    /* 1ページ分のデータの読み出し */
    if (read(file->desc, emptyBuf->page, PAGE_SIZE) < PAGE_SIZE) {
        return NG;
    }
    
    /* バッファの内容を引数のpageにコピー */
    memcpy(page, emptyBuf->page, PAGE_SIZE);
    
    /* Buffer構造体(emptyBuf)への各種情報の設定 */
    emptyBuf->file = file;
    emptyBuf->modified = UNMODIFIED;
    emptyBuf->pageNum = pageNum;
    
    /* アクセスされたバッファ(emptyBuf)を、リストの先頭に移動させる */
    moveBufferToListHead(emptyBuf);
    
    
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
Result writePage(File *file, int pageNum, char *page){
    
    Buffer *buf, *emptyBuf = NULL;
    
    /*
     * 書き込みを要求されたページがバッファに保存されているかどうか、
     * リストの先頭から順に探す
     */
    for (buf = bufferListHead; buf != NULL; buf = buf->next) {
        /* 要求されたページがリストの中にあるかどうかチェックする */
        if (buf->file == file && buf->pageNum == pageNum) {
            /* 要求されたページがバッファにあったので、そこにpageの内容をコピーする */
            memcpy(buf->page, page, PAGE_SIZE);
            
            /* 編集済みフラグを立てる */
            buf->modified = MODIFIED;
    
            /* アクセスされたバッファを、リストの先頭に移動させる */
            moveBufferToListHead(buf);
            
            return OK;
        }
    }
    
    /* バッファの空きを探す */
    for (buf = bufferListHead; buf != NULL; buf = buf->next) {
        /* 途中で空きバッファが見つかったらそれをemptyBufに */
        if(buf->file == NULL){
            emptyBuf = buf;
            break;
        }
        
        /* 最後まで空きバッファが見つからなかったら */
        if(buf->next == NULL && buf->file != NULL){
            /* 末端バッファをファイルに書き戻す */
            if(lseek(buf->file->desc, buf->pageNum*PAGE_SIZE, SEEK_SET)  == -1){
                return NG;
            }
            if (write(buf->file->desc, buf->page, PAGE_SIZE) == -1) {
                return NG;
            }
            
            /* 末端バッファを初期化してemptyBufに */
            buf->file = NULL;
            buf->pageNum = 0;
            memset(buf->page, 0, PAGE_SIZE);
            
            emptyBuf = buf;
            
        }
    }
    
    /*
     * lseekとwriteシステムコールで空きバッファにファイルの内容を読み込み、
     * Buffer構造体に保存する
     */
    
    /* 引数のpageをバッファにコピー */
    memcpy(emptyBuf->page, page, PAGE_SIZE);
    
    /* Buffer構造体(emptyBuf)への各種情報の設定 */
    emptyBuf->file = file;
    emptyBuf->modified = MODIFIED;
    emptyBuf->pageNum = pageNum;
    
    /* アクセスされたバッファ(emptyBuf)を、リストの先頭に移動させる */
    moveBufferToListHead(emptyBuf);
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
int getNumPages(char *filename){
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


/*
 * printBufferList -- バッファのリストの内容の出力(テスト用)
 */
void printBufferList()
{
    Buffer *buf;
    
    printf("Buffer List:");
    
    /* それぞれのバッファの最初の3バイトだけ出力する */
    for (buf = bufferListHead; buf != NULL; buf = buf->next) {
        if (buf->file == NULL) {
            printf("(empty) ");
        } else {
//            printf("    %c%c%c ", buf->page[0], buf->page[1], buf->page[2]);
            printf(" %s(%d) ", buf->file->name, buf->pageNum);
        }
    }
    
    printf("\n");
}

