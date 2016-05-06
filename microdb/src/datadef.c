/*
 * datadef.c - データ定義モジュール
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/microdb.h"

/*
 * DEF_FILE_EXT -- データ定義ファイルの拡張子
 */
#define DEF_FILE_EXT ".def"

/*
 * initializeDataDefModule -- データ定義モジュールの初期化
 *
 * 引数:
 *	なし
 *
 * 返り値;
 *	成功ならOK、失敗ならNGを返す
 */
Result initializeDataDefModule()
{
    return OK;
}

/*
 * finalizeDataDefModule -- データ定義モジュールの終了処理
 *
 * 引数:
 *	なし
 *
 * 返り値;
 *	成功ならOK、失敗ならNGを返す
 */
Result finalizeDataDefModule()
{
    return OK;
}

/*
 * createTable -- 表(テーブル)の作成
 *
 * 引数:
 *	tableName: 作成する表の名前
 *	tableInfo: データ定義情報
 *
 * 返り値:
 *	成功ならOK、失敗ならNGを返す
 *
 * データ定義ファイルの構造(ファイル名: tableName.def)
 *   +-------------------+----------------------+-------------------+----
 *   |フィールド数       |フィールド名          |データ型           |
 *   |(sizeof(int)バイト)|(MAX_FIELD_NAMEバイト)|(sizeof(int)バイト)|
 *   +-------------------+----------------------+-------------------+----
 * 以降、フィールド名とデータ型が交互に続く。
 */
Result createTable(char *tableName, TableInfo *tableInfo)
{
    File *file;
    char filename[MAX_FIELD_NAME];
    char page[PAGE_SIZE];
    char *p;
    int i;

    /* ページの内容をクリアする */
    if(strcmp(memset(page, 0, PAGE_SIZE), "") == 0){return NG;}
    p = page;

    /* ページの先頭にフィールド数を保存する */ //TODO strcmpはおかしい？
    if(strcmp(memcpy(p, &tableInfo->numField, sizeof(tableInfo->numField)), "") == 0){return NG;}
    p += sizeof(tableInfo->numField);

    //他の情報(フィールド名、データ型など)を、配列pageにmemcpyでコピーする...
    for(i=0; i<(tableInfo->numField); ++i){
        if(strcmp(memcpy(p, tableInfo->fieldInfo[i].name, sizeof(tableInfo->fieldInfo[i].name)), "") == 0){return NG;}
        p += sizeof(tableInfo->fieldInfo[i].name);

        if(strcmp(memcpy(p, &tableInfo->fieldInfo[i].dataType, sizeof(tableInfo->fieldInfo[i].dataType)), "") == 0){return NG;}
        p += sizeof(tableInfo->fieldInfo[i].dataType);

    }

    //ファイルを作成
    sprintf(filename, "%s%s", tableName, DEF_FILE_EXT);
    createFile(filename);

    //ファイルをオープン
    if((file = openFile(filename)) == NULL){return NG;}

    /* ファイルの先頭ページ(ページ番号0)に1ページ分のデータを書き込む */
    if(writePage(file, 0, page) == NG){return NG;}

    if(closeFile(file) == NG){return NG;}

    return OK;
}

/*
 * dropTable -- 表(テーブル)の削除
 *
 * 引数:
 *	tableInfo: 削除するテーブル定義情報
 *
 * 返り値:
 *	成功ならOK、失敗ならNGを返す
 */
Result dropTable(char *tableName)
{
    char filename[MAX_FILENAME];

    //テーブル定義情報ファイルの削除
    sprintf(filename, "%s%s", tableName, DEF_FILE_EXT);
    if(deleteFile(filename) == NG){return NG;}

    return OK;
}

/*
 * getTableInfo -- 表のデータ定義情報を取得する関数
 *
 * 引数:
 *	tableName: 情報を表示する表の名前
 *
 * 返り値:
 *	tableNameのデータ定義情報を返す
 *	エラーの場合には、NULLを返す
 *
 * ***注意***
 *	この関数が返すデータ定義情報を収めたメモリ領域は、不要になったら
 *	必ずfreeTableInfoで解放すること。
 */
TableInfo *getTableInfo(char *tableName)
{
    TableInfo *tableInfo;
    File *file;
    char filename[MAX_FILENAME];
    char page[PAGE_SIZE];
    char *p;
    int i;


    //テーブル情報のメモリ確保
    if((tableInfo = (TableInfo*)malloc(sizeof(TableInfo))) == NULL){return NULL;}

    //ファイルのオープン
    sprintf(filename, "%s%s", tableName, DEF_FILE_EXT);
    if((file = openFile(filename)) == NULL){return NULL;}

    //0ページ目を読み込み
    if(readPage(file, 0, page) == NG){return NULL;}
    p = page;

    //フィールド数を取得
    memcpy(&tableInfo->numField, p, sizeof(tableInfo->numField));
    if(tableInfo->numField < 0){return NULL;}
    p += sizeof(tableInfo->numField);

    //フィールド名とフィールドタイプを個数分読み込み
    for(i=0; i<(tableInfo->numField); ++i){
        if(strcmp(memcpy(tableInfo->fieldInfo[i].name, p, sizeof(tableInfo->fieldInfo[i].name)), "") ==0){return NULL;}
        p += sizeof(tableInfo->fieldInfo[i].name);

        memcpy(&tableInfo->fieldInfo[i].dataType, p, sizeof(tableInfo->fieldInfo[i].name));
        p += sizeof(tableInfo->fieldInfo[i].dataType);
    }

    //ファイルのクローズ
    if(closeFile(file) == NG){return NULL;}

    return tableInfo;
}

/*
 * freeTableInfo -- データ定義情報を収めたメモリ領域の解放
 *
 * 引数:
 *	tableInfo: 開放するテーブル定義情報
 *
 * 返り値:
 *	なし
 *
 * ***注意***
 *	関数getTableInfoが返すデータ定義情報を収めたメモリ領域は、
 *	不要になったら必ずこの関数で解放すること。
 */
void freeTableInfo(TableInfo *tableInfo)
{
    free(tableInfo);
}
