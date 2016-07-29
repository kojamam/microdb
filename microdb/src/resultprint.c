/*
 * resultprint.c -- 結果表示モジュール
 */

#include "../include/microdb.h"

/*
 * DATA_FILE_EXT -- データファイルの拡張子
 */
#define DATA_FILE_EXT ".dat"

/*
 * COLUMN_WIDTH -- テーブル表示時のカラムの幅
 */
#define COLUMN_WIDTH 12

/*
 * insertLine -- レコード表示時の罫線を室力
 *
 * 引数:
 *	n: レコード数
 *  m: カラム幅
 */
static void insertLine(int n, int m){
    int i;
    for (i = 0; i < n*m+1; i++) {
        if(i%m == 0){printf("+");}
        else{printf("-");}
    }
    printf("\n");
}

/*
 * printTableHeader -- テーブルのヘッダを表示
 *
 * 引数:
 *	tableInfo: テーブル情報
 */
static void printTableHeader(TableInfo *tableInfo, FieldList *fieldList){
    int i, j;
    int isIncluded = 0;

    /*表ヘッダの出力*/
    if(fieldList->numField > 0){
        insertLine(fieldList->numField, COLUMN_WIDTH);
    }else{
        insertLine(tableInfo->numField, COLUMN_WIDTH);
    }
    printf("|");

    for (i = 0; i < tableInfo->numField; i++) {

        if(fieldList->numField > 0){
            for(j=0; j < fieldList->numField; j++){
                if(strcmp(fieldList->name[j], tableInfo->fieldInfo[i].name) == 0){
                    isIncluded = 1;
                    break;
                }else{
                    isIncluded = 0;
                }
            }
        }else{
            isIncluded = 1;
        }

        if (isIncluded == 1) {
            printf("%10s |", tableInfo->fieldInfo[i].name);
        }

    }
    printf("\n");
    /*罫線の挿入*/
    if(fieldList->numField > 0){
        insertLine(fieldList->numField, COLUMN_WIDTH);
    }else{
        insertLine(tableInfo->numField, COLUMN_WIDTH);
    }
}

/*
 * printTableData -- すべてのデータの表示(テスト用)
 *
 * 引数:
 *	tableName: データを表示するテーブルの名前
 */
void printTableData(char *tableName){
    RecordSet *recordSet;
    char filename[MAX_FILENAME];
    File *file;
    int numPage, numRecord = 0;
    TableInfo *tableInfo;
    int i, j, k;
    char page[PAGE_SIZE];
    int numRecordSlot;
    char *p, *q;
    RecordSlot recordSlot;
    FieldList fieldList;
    
    fieldList.numField = 0;

    recordSet = (RecordSet*)malloc(sizeof(RecordSet));

    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
    file = openFile(filename);

    numPage = getNumPages(filename);

    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        return; //エラー処理
    }

    /*表ヘッダの出力*/
    printTableHeader(tableInfo, &fieldList);

    /* ページ数分だけ繰り返す */
    for (i=0; i<numPage; ++i) {
        readPage(file, i, page);

        /*スロットの数*/
        memcpy(&numRecordSlot, page, sizeof(int));

        p = page + sizeof(int);

        /* スロットを見ていく */
        for (j=0; j<numRecordSlot; ++j) {
            int intValue;
            double doubleValue;
            char stringVal[MAX_STRING];
            int stringLen;

            memcpy(&recordSlot.flag, p, sizeof(char));
            memcpy(&recordSlot.offset, p+sizeof(char), sizeof(int));
            memcpy(&recordSlot.size,p+sizeof(char)+sizeof(int), sizeof(int));

            q = page + recordSlot.offset;

            /* レコードがあったら表示 */
            if(recordSlot.flag == 1){
                numRecord++;

                printf("|");
                /*フィールドを見ていく*/
                for (k = 0; k < tableInfo->numField; k++) {
                    switch (tableInfo->fieldInfo[k].dataType) {
                        case TYPE_INT:
                            /* 整数の時、表示 */
                            memcpy(&intValue, q, sizeof(int));
                            printf("%10d |", intValue);
                            q += sizeof(int);
                            break;
                        case TYPE_DOUBLE:
                            /* 浮動小数点のとき表示 */
                            memcpy(&doubleValue, q, sizeof(double));
                            printf("%10f |", doubleValue);
                            q += sizeof(double);
                            break;
                        case TYPE_VARCHAR:
                            /* 文字列の時、表示 */
                            memcpy(&stringLen, q, sizeof(int));
                            q += sizeof(int);
                            strcpy(stringVal, q);
                            printf("%10s |", stringVal);
                            q += stringLen + 1; // '\0'の分も進む
                            break;
                        default:
                            /* ここにくることはないはず */
                            freeTableInfo(tableInfo);
                            return ;
                    }

                }/* レコードの読み込み終わり */

                printf("\n");
            }


            /* 次のスロットを見る */
            p += sizeof(char) + sizeof(int) * 2;
        }/* スロット繰り返し */

    }/*ページ繰り返し*/

    if(numRecord > 0){
        insertLine(tableInfo->numField, COLUMN_WIDTH);
    }
    printf("%d rows in set\n", numRecord);

    return;

}

/*
 * printRecordSet -- レコード集合の表示
 *
 * 引数:
 *	recordSet: 表示するレコード集合
 */
void printRecordSet(char *tableName, RecordSet *recordSet, FieldList *fieldList){
    RecordData *record;
    TableInfo *tableInfo;
    int i,j;

    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        return; //エラー処理
    }

    /*表ヘッダの出力*/
    printTableHeader(tableInfo, fieldList);

    record = recordSet->recordData;
    for (i=0; i<recordSet->numRecord; ++i) {
        printf("|");

        for (j = 0; j < record->numField; j++) {
            switch (record->fieldData[j].dataType) {
                case TYPE_INT:
                    /* 整数の時、表示 */
                    printf("%10d |", record->fieldData[j].val.intVal);
                    break;
                case TYPE_DOUBLE:
                    /* 浮動小数点の時、表示 */
                    printf("%10f |", record->fieldData[j].val.doubleVal);
                    break;
                case TYPE_VARCHAR:
                    /* 文字列の時、表示 */
                    printf("%10s |", record->fieldData[j].val.stringVal);
                    break;
                default:
                    /* ここにくることはないはず */
                    freeTableInfo(tableInfo);
                    freeRecordSet(recordSet);
                    return ;
            }

        }/* レコードの読み込み終わり */

        /*次のrecordを読む */
        record = record->next;

        printf("\n");
    }

    /*罫線の挿入*/
    if(recordSet->numRecord > 0){
        if(fieldList->numField > 0){
            insertLine(fieldList->numField, COLUMN_WIDTH);
        }else{
            insertLine(tableInfo->numField, COLUMN_WIDTH);
        }
    }

    /* レコード数の表示 */
    printf("%d rows in set\n", recordSet->numRecord);
}

/*
 * printTableInfo -- テーブルのデータ定義情報を表示する(動作確認用)
 *
 * 引数:
 *	tableName: 情報を表示するテーブルの名前
 *
 * 返り値:
 *	なし
 */
void printTableInfo(char *tableName){
    TableInfo *tableInfo;
    int i;

    /* テーブル名を出力 */
    printf("\nTable %s\n", tableName);

    /* テーブルの定義情報を取得する */
    if ((tableInfo = getTableInfo(tableName)) == NULL) {
        /* テーブル情報の取得に失敗したので、処理をやめて返る */
        return;
    }

    /* フィールド数を出力 */
    printf("number of fields = %d\n", tableInfo->numField);

    /* フィールド情報を読み取って出力 */
    for (i = 0; i < tableInfo->numField; i++) {
        /* フィールド名の出力 */
        printf("  field %d: name = %s, ", i + 1, tableInfo->fieldInfo[i].name);

        /* データ型の出力 */
        printf("data type = ");
        switch (tableInfo->fieldInfo[i].dataType) {
            case TYPE_INT:
                printf("int\n");
                break;
            case TYPE_DOUBLE:
                printf("double\n");
                break;
            case TYPE_VARCHAR:
                printf("varchar\n");
                break;
            default:
                printf("unknown\n");
        }
    }

    /* データ定義情報を解放する */
    freeTableInfo(tableInfo);

    return;
}
