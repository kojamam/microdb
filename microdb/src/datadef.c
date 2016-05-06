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
Result dropTable(TableInfo *tableInfo)
{
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
}
