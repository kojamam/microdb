/*
* datamanip.c -- データ操作モジュール
*/

#include "microdb.h"

/*
* DATA_FILE_EXT -- データファイルの拡張子
*/
#define DATA_FILE_EXT ".dat"

/*
* initializeDataManipModule -- データ操作モジュールの初期化
*
* 引数:
*	なし
*
* 返り値;
*	成功ならOK、失敗ならNGを返す
*/
Result initializeDataManipModule()
{
    return OK;
}

/*
* finalizeDataManipModule -- データ操作モジュールの終了処理
*
* 引数:
*	なし
*
* 返り値;
*	成功ならOK、失敗ならNGを返す
*/
Result finalizeDataManipModule()
{
    return OK;
}

/*
* getRecordSize -- 1レコード分の保存に必要なバイト数の計算
*
* 引数:
*  recordData: レコードの情報
*	tableData: データ定義情報を収めた構造体
*
* 返り値:
*	そのレコードを格納するのに必要なバイト数
*	必要なバイト数
*/
static int getRecordSize(RecordData *recordData, TableInfo *tableInfo)
{
    int total = 0;
    int i;


    for (i=0; i < tableInfo->numField; i++) {
        /* i番目のフィールドがINT型かSTRING型か調べる */
        /* INT型ならtotalにsizeof(int)を加算 */
        /* STRING型ならtotalにfieldDataのstrlen+1を加算 */
        switch (tableInfo->fieldInfo[i].dataType) {
            case TYPE_INTEGER:
            total += sizeof(int);
            break;
            case TYPE_STRING:
            /* stlen +文字数格納のためのint + null文字 */
            total += strlen(recordData->fieldData[i].stringValue) + sizeof(int) + 1;
            break;
            default:
            /* ここにくることはないはず */
            freeTableInfo(tableInfo);
            free(recordData);
            return NG;
        }

    }

    return total;
}

/*
* insertRecord -- レコードの挿入
*
* 引数:
*	tableName: レコードを挿入するテーブルの名前
*	recordData: 挿入するレコードのデータ
*
* 返り値:
*	挿入に成功したらOK、失敗したらNGを返す
*/
Result insertRecord(char *tableName, RecordData *recordData)
{
    TableInfo *tableInfo;
    int recordSize;
    char *record; //レコード文字列
    char *p, *q; //現在アドレス
    int i, j;
    int stringLen;
    char filename[MAX_FILENAME];
    File *file;
    int numPage;
    char page[PAGE_SIZE];
    int numRecordSlot;
    RecordSlot recordSlot;
    RecordSlot newRecordSlot;

    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        return NG; //エラー処理
    }

    /* レコードを治めるために必要なバイト数を計算 */
    recordSize = getRecordSize(recordData, tableInfo);

    /* レコード文字列のメモリの確保 */
    if((record = (char *)malloc(sizeof(char) * recordSize)) == NULL){
        return NG; //エラー処理
    }

    /* recordの先頭アドレスををpに代入 */
    p = record;

    /* 確保したメモリ領域に、フィールド数分だけ、順次データを埋め込む */
    for (i = 0; i < tableInfo->numField; i++) {
        switch (tableInfo->fieldInfo[i].dataType) {
            case TYPE_INTEGER:
            /* 整数の時、そのままコピーしてポインタを進める */
            memcpy(p, &recordData->fieldData[i].intValue, sizeof(int));
            p += sizeof(int);
            break;
            case TYPE_STRING:
            /* 文字列の時、文字列の長さを先頭に格納してから文字列を格納 */
            stringLen = (int)strlen(recordData->fieldData[i].stringValue);
            memcpy(p, &stringLen, sizeof(int));
            p += sizeof(int);
            strcpy(p, recordData->fieldData[i].stringValue);
            p += stringLen+1;
            break;
            default:
            /* ここにくることはないはず */
            freeTableInfo(tableInfo);
            free(record);
            return NG;
        }
    }

    /* ファイルオープン */
    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
    file = openFile(filename);

    /* データファイルのページ数を調べる */
    numPage = getNumPages(filename);

    /* ページごとにデータを挿入できる空きを探す */
    for (i = 0; i < numPage; i++) {
        
        
        /* 1ページ分のデータを読み込む */
        if (readPage(file, i, page) != OK) {
            free(record); //エラー処理
            return NG;
        }

        /* スロット個数 */
        memcpy(&numRecordSlot, page, sizeof(int));
        q = page + sizeof(int);

        /* スロットを見て空きを探す */
        for (j=0; j<numRecordSlot; ++j) {
            memcpy(&recordSlot.flag, q, sizeof(char));
            memcpy(&recordSlot.size, q+sizeof(char), sizeof(int));
            memcpy(&recordSlot.offset, q+sizeof(char)+sizeof(int), sizeof(int));

            /* 十分な空きがあったら後ろから詰めてrecordを書き込み */
            if(recordSlot.flag == 0 && recordSlot.size >= recordSize){
                memcpy(page + recordSlot.offset + recordSlot.size - recordSize, record, recordSize);

                /* 新しいスロット */
                newRecordSlot.flag = 0;
                newRecordSlot.size = recordSlot.size - recordSize;
                newRecordSlot.offset = recordSlot.offset;

                /* 元のスロットの更新 */
                recordSlot.flag = 1;
                recordSlot.offset += recordSlot.size - recordSize;
                recordSlot.size = recordSize;

                memcpy(q, &recordSlot.flag, sizeof(char));
                memcpy(q+sizeof(char), &recordSlot.size, sizeof(int));
                memcpy(q+sizeof(char)+sizeof(int), &recordSlot.offset, sizeof(int));

                q = page + sizeof(int) + numRecordSlot * (sizeof(char) + sizeof(int) * 2);

                /* 空きが残っていたらスロットを追加 */
                if(newRecordSlot.size > 0){
                    memcpy(q, &newRecordSlot.flag, sizeof(char));
                    memcpy(q+sizeof(char), &newRecordSlot.size, sizeof(int));
                    memcpy(q+sizeof(char)+sizeof(int), &newRecordSlot.offset, sizeof(int));
                    numRecordSlot++;
                    memcpy(page, &numRecordSlot, sizeof(int));

                }
                
                writePage(file, i, page);

                return OK;

            }

            /* 次のスロットを見る */
            q += sizeof(char) + sizeof(int) * 2;
        }/* スロット繰り返し */

    }/* ページ繰り返し */

    /* 空きがないとき新規ページ作成 */
    q = page;
    
    /* 0で初期化 */
    memset(page, 0, PAGE_SIZE);
    
    /* スロットの数を書き込み */
    numRecordSlot = 2;
    memcpy(q, &numRecordSlot, sizeof(int));
    
    q = page + sizeof(char);

    /* 書き込むレコードのスロットを書き込み */
    recordSlot.flag = 1;
    recordSlot.size = recordSize;
    recordSlot.offset = PAGE_SIZE - recordSize;
    
    memcpy(q, &recordSlot.flag, sizeof(char));
    memcpy(q+sizeof(char), &recordSlot.size, sizeof(int));
    memcpy(q+sizeof(char)+sizeof(int), &recordSlot.offset, sizeof(int));
    
    q += (sizeof(char) + sizeof(int) * 2);
    
    /* 空き容量のスロットを書き込み */
    newRecordSlot.flag = 0;
    newRecordSlot.size = PAGE_SIZE - recordSize;
    newRecordSlot.offset = sizeof(char) + (sizeof(char) + sizeof(int)*2) * 2;
    
    memcpy(q, &newRecordSlot.flag, sizeof(char));
    memcpy(q+sizeof(char), &newRecordSlot.size, sizeof(int));
    memcpy(q+sizeof(char)+sizeof(int), &newRecordSlot.offset, sizeof(int));
    
    /* レコードを書き込み */
    q = page + PAGE_SIZE - recordSize;
    memcpy(q, record, recordSize);
    
    writePage(file, numPage, page);


    /* TODO エラー処理 */
    return OK;
}


/*
* checkCondition -- レコードが条件を満足するかどうかのチェック
*
* 引数:
*	recordData: チェックするレコード
*	condition: チェックする条件
*
* 返り値:
*	レコードrecordが条件conditionを満足すればOK、満足しなければNGを返す
*/
static Result checkCondition(RecordData *recordData, Condition *condition)
{
    return OK;
}

/*
* selectRecord -- レコードの検索
*
* 引数:
*	tableName: レコードを検索するテーブルの名前
*	condition: 検索するレコードの条件
*
* 返り値:
*	検索に成功したら検索されたレコード(の集合)へのポインタを返し、
*	検索に失敗したらNULLを返す。
*	検索した結果、該当するレコードが1つもなかった場合も、レコードの
*	集合へのポインタを返す。
*
* ***注意***
*	この関数が返すレコードの集合を収めたメモリ領域は、不要になったら
*	必ずfreeRecordSetで解放すること。
*/
RecordSet *selectRecord(char *tableName, Condition *condition)
{
    RecordSet *recordSet;
    char filename[MAX_FILENAME];
    File *file;
    int numPage;
    TableInfo *tableInfo;
    int i, j, k;
    char page[PAGE_SIZE];
    int numRecordSlot;
    char *p, *q;
    RecordSlot recordSlot;
    
    recordSet = (RecordSet*)malloc(sizeof(RecordSet));
    
    sscanf(filename, "%s%s", tableName, DATA_FILE_EXT);
    file = openFile(filename);
    
    numPage = getNumPages(filename);
    
    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        return NULL; //エラー処理
    }

    /* ページ数分だけ繰り返す */
    for (i=0; i<numPage; ++i) {
        readPage(file, i, page);
        
        /*スロットの数*/
        memcpy(&numRecordSlot, page, sizeof(int));
        
        p = page + sizeof(int);
        
        /* スロットを見て空きを探す */
        for (j=0; j<numRecordSlot; ++j) {
            memcpy(&recordSlot.flag, p, sizeof(char));
            memcpy(&recordSlot.size, p+sizeof(char), sizeof(int));
            memcpy(&recordSlot.offset,p+sizeof(char)+sizeof(int), sizeof(int));
            
            q = page + recordSlot.offset;
            
            /* レコードがあったら読み込み */
            if(recordSlot.flag == 1){
                RecordData *recordData = (RecordData*)malloc(sizeof(RecordData));
                int stringLen;

                for (k = 0; k < tableInfo->numField; k++) {
                    strcpy(recordData->fieldData[i].name, tableInfo->fieldInfo[i].name);
                    switch (tableInfo->fieldInfo[k].dataType) {
                        case TYPE_INTEGER:
                            /* 整数の時、そのままコピーしてポインタを進める */
                            memcpy(&recordData->fieldData[i].intValue, q, sizeof(int));
                            q += sizeof(int);
                            break;
                        case TYPE_STRING:
                            /* 文字列の時、文字列の長さを先頭に格納してから文字列を格納 */
                            memcpy(&stringLen, q, sizeof(int));
                            q += sizeof(int);
                            strcpy(recordData->fieldData[i].stringValue, q);
                            q += stringLen+1; // '\0'の分も進む
                            break;
                        default:
                            /* ここにくることはないはず */
                            freeTableInfo(tableInfo);
                            free(recordData);
                            return NULL;
                    }
                }/* レコードの読み込み終わり */
                
                /*条件を満足するかを確認して満たしていたらRecordSetの末尾に追加*/
                if(checkCondition(recordData, condition) == OK){
                    recordSet->numRecord++;
                    if(recordSet->recordData == NULL){
                        recordSet->recordData = recordData;
                    }else{
                        RecordData *tmpRecordData = recordSet->recordData;
                        while (tmpRecordData->next != NULL) {
                            tmpRecordData = tmpRecordData->next;
                        }
                        tmpRecordData->next = recordData;
                    }
                }else{
                    free(recordData);
                }
            }
            
            /* 次のスロットを見る */
            p += sizeof(char) + sizeof(int) * 2;
        }/* スロット繰り返し */
        
    }/*ページ繰り返し*/

    return recordSet;
}

/*
* freeRecordSet -- レコード集合の情報を収めたメモリ領域の解放
*
* 引数:
*	recordSet: 解放するメモリ領域
*
* 返り値:
*	なし
*
* ***注意***
*	関数selectRecordが返すレコードの集合を収めたメモリ領域は、
*	不要になったら必ずこの関数で解放すること。
*/
void freeRecordSet(RecordSet *recordSet)
{
    RecordData *prevRecordData, *nextRecordData;
    
    prevRecordData = recordSet->recordData;
    free(recordSet);
    
    while(nextRecordData != NULL){
        nextRecordData = prevRecordData->next;
        free(prevRecordData);
    }
}

/*
* deleteRecord -- レコードの削除
*
* 引数:
*	tableName: レコードを削除するテーブルの名前
*	condition: 削除するレコードの条件
*
* 返り値:
*	削除に成功したらOK、失敗したらNGを返す
*/
Result deleteRecord(char *tableName, Condition *condition)
{
    return OK;
}

/*
* createDataFile -- データファイルの作成
*
* 引数:
*	tableName: 作成するテーブルの名前
*
* 返り値:
*	作成に成功したらOK、失敗したらNGを返す
*/
Result createDataFile(char *tableName)
{
    return OK;
}

/*
* deleteDataFile -- データファイルの削除
*
* 引数:
*	tableName: 削除するテーブルの名前
*
* 返り値:
*	削除に成功したらOK、失敗したらNGを返す
*/
Result deleteDataFile(char *tableName)
{
    return OK;
}

/*
* printRecordSet -- レコード集合の表示
*
* 引数:
*	recordSet: 表示するレコード集合
*
* 返り値:
*	なし
*/
void printRecordSet(RecordSet *recordSet)
{
}
