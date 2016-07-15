/*
* datamanip.c -- データ操作モジュール
*/

#include "../include/microdb.h"

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
    char *recordString;
    char filename[MAX_FILENAME];
    File *file;
    int numPage;
    char page[PAGE_SIZE];
    int recordSize;
    int numRecordSlot;
    RecordSlot *recordSlot;
    int i,j;
    char *q; //pageのポインタ

    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        return NG; //エラー処理
    }

    recordSize = getRecordSize(recordData, tableInfo);

    if((recordString = createRecordString(tableInfo, recordData, recordSize)) == NULL){
     return NG;
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
            free(recordString); //エラー処理
            return NG;
        }

        /* スロット個数を読み込む */
        memcpy(&numRecordSlot, page, sizeof(int));
        q = page + sizeof(int);

        /* スロットを見て空きを探す */
        for (j=0; j<numRecordSlot; ++j) {

            recordSlot = readSlotFromPage(page, j);

            /* 十分な空きがあったら後ろから詰めてrecordを書き込み */
            if(recordSlot->flag == 0 && recordSlot->size >= recordSize){
                memcpy(page + recordSlot->offset + recordSlot->size - recordSize, recordString, recordSize);

                /* 新しいスロット */
                RecordSlot *newRecordSlot = (RecordSlot*)malloc(sizeof(RecordSlot));
                newRecordSlot->num = numRecordSlot;
                newRecordSlot->flag = 0;
                newRecordSlot->offset = recordSlot->offset;
                newRecordSlot->size = recordSlot->size - recordSize;


                /* 元のスロットの更新 */
                recordSlot->flag = 1;
                recordSlot->offset = recordSlot->offset + recordSlot->size -  recordSize;
                recordSlot->size = recordSize;
                

                writeSlotToPage(page, recordSlot);

                /* 空きが残っていたらスロットを追加 */
                if(newRecordSlot->size > 0){
                    writeSlotToPage(page, newRecordSlot);
                    changeNumSlot(page, 1);
                }
                
                

                writePage(file, i, page);

                return OK;

            }

            /* 次のスロットを見る */
            q += sizeof(char) + sizeof(int) * 2;
        }/* スロット繰り返し */

    }/* ページ繰り返し */

    /* 空きがなかったら新規ページ作成 */
    initializePage(page);
    recordSlot = readSlotFromPage(page, 0);
    memcpy(page + recordSlot->offset + recordSlot->size - recordSize, recordString, recordSize);

    RecordSlot *newRecordSlot = (RecordSlot*)malloc(sizeof(RecordSlot));
    newRecordSlot->num = 1;
    newRecordSlot->flag = 0;
    newRecordSlot->offset = recordSlot->offset;
    newRecordSlot->size = recordSlot->size - recordSize;

    recordSlot->flag = 1;
    recordSlot->offset = PAGE_SIZE - recordSize;
    recordSlot->size = recordSize;

    writeSlotToPage(page, recordSlot);
    writeSlotToPage(page, newRecordSlot);
    
    changeNumSlot(page, 1);
    

    writePage(file, numPage, page);

    /* TODO エラー処理 */
    return OK;
}

/*
* createRecordString -- ページに書き込むレコード文字列の作成
*
* 引数:
*	tableInfo: レコードを挿入するテーブルの情報
*	recordData: 挿入するレコードのデータ
*
* 返り値:
*	挿入に成功したらレコード文字列、失敗したらNULLを返す
*/
char* createRecordString(TableInfo *tableInfo, RecordData *recordData, int recordSize){
    char *recordString;
    char *p;
    int i;

    /* レコードを治めるために必要なバイト数を計算 */
    recordSize = getRecordSize(recordData, tableInfo);

    /* レコード文字列のメモリの確保 */
    if((recordString = (char *)malloc(sizeof(char) * recordSize)) == NULL){
        return NULL; //エラー処理
    }


    /* recordの先頭アドレスををpに代入 */
    p = recordString;

    /* 確保したメモリ領域に、フィールド数分だけ、順次データを埋め込む */
    for (i = 0; i < tableInfo->numField; i++) {
        int stringLen;
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
                p += stringLen+1; // '\0'の分も
                break;
            default:
                /* ここにくることはないはず */
                freeTableInfo(tableInfo);
                free(recordString);
                return NULL;
        }
    }

    return recordString;
}

/*
 * readSlotFromPage -- ページからスロットを1つ読み込み
 *
 * 引数:
 *	page: スロットを読み込むページ
 *	n: 先頭から何番目のスロットを読み込むか
 *
 * 返り値:
 *	読み込んだRecordSlot構造体のポインタ。失敗したらNULL
 */
RecordSlot* readSlotFromPage(char *page, int n){
    RecordSlot *slot;
    char *p;

    if((slot = (RecordSlot*)malloc(sizeof(RecordSlot))) == NULL){
        return NULL;
    }

    slot->num = n;

    /* スロットがある位置までポインタをすすめる */
    p = page;
    p += sizeof(int) + (sizeof(char) + sizeof(int) + sizeof(int)) * n;

    /* スロットを構造体にコピー */
    memcpy(&slot->flag, p, sizeof(char));
    memcpy(&slot->offset, p+sizeof(char), sizeof(int));
    memcpy(&slot->size, p+sizeof(char)+sizeof(int), sizeof(int));

    return slot;
}

/*
 * writeSlotToPage -- ページにスロットを1つ書き込む
 *
 * 引数:
 *	page: スロットを書き込むpage
 *	slot: 書き込むスロットのデータ
 *
 * 返り値:
 *	書き込みに成功したらOK、失敗したらNGを返す
 */
Result writeSlotToPage(char *page, RecordSlot *slot){
    char *p;

    p = page;
    p += sizeof(int) + (sizeof(char) + sizeof(int) + sizeof(int)) * slot->num;

    memcpy(p, &slot->flag, sizeof(char));
    memcpy(p+sizeof(char), &slot->offset, sizeof(int));
    memcpy(p+sizeof(char)+sizeof(int), &slot->size, sizeof(int));

    free(slot);

    return OK;
}


/*
 * changeNumSlot -- ページのスロット数記録を変更する
 *
 * 引数:
 *	page: スロットを書き込むpage
 *	delta: スロット個数の差分
 *
 * 返り値:
 *	書き込みに成功したら変更後の総数、失敗したらNULL
 */
int changeNumSlot(char *page, int delta){
    int num;

    memcpy(&num, page, sizeof(int));
    num += delta;
    memcpy(page, &num, sizeof(int));

    return num;

}

/*
 * initializePage -- ページの初期化
 *
 * 引数:
 *	page: 初期化を行うpage
 *
 * 返り値:
 *	初期化に成功したらOK, 失敗したらNG
 */
Result initializePage(char *page){
    int numSlot = 1;
    RecordSlot *slot;

    /* メモリの確保 */
    slot = (RecordSlot*)malloc(sizeof(RecordSlot));

    /* 0埋め */
    memset(page, 0, PAGE_SIZE);

    /* 先頭にスロット数を書き込み */
    memcpy(page, &numSlot, sizeof(int));

    /* 一個目のスロットの構造体を用意 */
    slot->num = 0;
    slot->flag = 0;
    slot->offset = sizeof(int) + (sizeof(char) + sizeof(int) * 2);
    slot->size = PAGE_SIZE - slot->offset;
    /* スロットの書き込み */
    if(writeSlotToPage(page, slot) == NG){ return NG; }

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
    int i;
    OperatorType opType = condition->operator;
    int diff;
    
    for(i=0; i<recordData->numField; ++i){
        if(strcmp(recordData->fieldData[i].name, condition->name) == 0){
            if(recordData->fieldData[i].dataType == TYPE_INTEGER){
                diff = recordData->fieldData[i].intValue - condition->intValue;
            }else if(recordData->fieldData[i].dataType == TYPE_STRING){
                diff = strcmp(recordData->fieldData[i].stringValue, condition->stringValue);
            }else{
                /*ここに来ることはないはず*/
                diff = 0;
            }
            
            if((opType == OPR_EQUAL &&  diff == 0)
               || (opType == OPR_NOT_EQUAL &&  diff != 0)
               || (opType == OPR_GREATER_THAN &&  diff > 0)
               || (opType == OPR_OR_GREATER_THAN &&  diff >= 0)
               || (opType == OPR_LESS_THAN &&  diff < 0)
               || (opType == OPR_OR_LESS_THAN &&  diff <= 0) ){
                return OK;
            } else {
                return NG;
            }
        }
    }
    return NG;
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
    recordSet->numRecord = 0;


    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
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

        /* スロットを見ていく */
        for (j=0; j<numRecordSlot; ++j) {
            memcpy(&recordSlot.flag, p, sizeof(char));
            memcpy(&recordSlot.offset, p+sizeof(char), sizeof(int));
            memcpy(&recordSlot.size,p+sizeof(char)+sizeof(int), sizeof(int));

            q = page + recordSlot.offset;

            /* レコードがあったら読み込み */
            if(recordSlot.flag == 1){
                RecordData *recordData = (RecordData*)malloc(sizeof(RecordData));
                recordData->numField = tableInfo->numField;
                int stringLen;

                for (k = 0; k < tableInfo->numField; k++) {
                    strcpy(recordData->fieldData[k].name, tableInfo->fieldInfo[k].name);
                    recordData->fieldData[k].dataType = tableInfo->fieldInfo[k].dataType;
                    switch (tableInfo->fieldInfo[k].dataType) {
                        case TYPE_INTEGER:
                            /* 整数の時、そのままコピーしてポインタを進める */
                            memcpy(&recordData->fieldData[k].intValue, q, sizeof(int));
                            q += sizeof(int);
                            break;
                        case TYPE_STRING:
                            /* 文字列の時、文字列の長さを先頭に格納してから文字列を格納 */
                            memcpy(&stringLen, q, sizeof(int));
                            q += sizeof(int);
                            strcpy(recordData->fieldData[k].stringValue, q);
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
    RecordData *recordData, *tmp;

    recordData = recordSet->recordData;
    free(recordSet);

    do{
        tmp = recordData;
        recordData = recordData->next;
        free(tmp);
    }while(recordData != NULL);
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

    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
    file = openFile(filename);

    numPage = getNumPages(filename);

    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        return NG; //エラー処理
    }

    /* ページ数分だけ繰り返す */
    for (i=0; i<numPage; ++i) {
        readPage(file, i, page);

        /*スロットの数*/
        memcpy(&numRecordSlot, page, sizeof(int));

        p = page + sizeof(int);

        /* スロットを見ていく */
        for (j=0; j<numRecordSlot; ++j) {
            memcpy(&recordSlot.flag, p, sizeof(char));
            memcpy(&recordSlot.offset, p+sizeof(char), sizeof(int));
            memcpy(&recordSlot.size,p+sizeof(char)+sizeof(int), sizeof(int));
            
            q = page + recordSlot.offset;
            
            /* レコードがあったら読み込み */
            if(recordSlot.flag == 1){
                RecordData *recordData = (RecordData*)malloc(sizeof(RecordData));
                recordData->numField = tableInfo->numField;
                int stringLen;
                
                for (k = 0; k < tableInfo->numField; k++) {
                    strcpy(recordData->fieldData[k].name, tableInfo->fieldInfo[k].name);
                    recordData->fieldData[k].dataType = tableInfo->fieldInfo[k].dataType;
                    switch (tableInfo->fieldInfo[k].dataType) {
                        case TYPE_INTEGER:
                            /* 整数の時、そのままコピーしてポインタを進める */
                            memcpy(&recordData->fieldData[k].intValue, q, sizeof(int));
                            q += sizeof(int);
                            break;
                        case TYPE_STRING:
                            /* 文字列の時、文字列の長さを先頭に格納してから文字列を格納 */
                            memcpy(&stringLen, q, sizeof(int));
                            q += sizeof(int);
                            strcpy(recordData->fieldData[k].stringValue, q);
                            q += stringLen+1; // '\0'の分も進む
                            break;
                        default:
                            /* ここにくることはないはず */
                            freeTableInfo(tableInfo);
                            free(recordData);
                            return NG;
                    }
                }/* レコードの読み込み終わり */

                /*条件を満足するかを確認して満たしていたら削除 */
                if(condition == NULL || checkCondition(recordData, condition) == OK){
                    /* 0埋め */
                    memset(page+recordSlot.offset, 0, recordSlot.size);
                    /* スロットの更新*/
                    recordSlot.flag = 0;
                    memcpy(p, &recordSlot.flag, sizeof(char));
                    memcpy(p+sizeof(char), &recordSlot.offset, sizeof(int));
                    memcpy(p+sizeof(char)+sizeof(int), &recordSlot.size, sizeof(int));
                }/* TODO 隣のスロットと統合 */
                free(recordData);
            }

            /* 次のスロットを見る */
            p += sizeof(char) + sizeof(int) * 2;
        }/* スロット繰り返し */
        
        writePage(file, i, page);

    }/*ページ繰り返し*/

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
    char filename[MAX_FILENAME];

    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);

    if(createFile(filename) != OK){
        return NG;
    }

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
    char filename[MAX_FILENAME];

    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);

    if(deleteFile(filename) != OK){
        return NG;
    }

    return OK;
}

/*
 * printTableData -- すべてのデータの表示(テスト用)
 *
 * 引数:
 *	tableName: データを表示するテーブルの名前
 */
void printTableData(char *tableName)
{
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

    recordSet = (RecordSet*)malloc(sizeof(RecordSet));

    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
    file = openFile(filename);

    numPage = getNumPages(filename);

    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        return; //エラー処理
    }
    
    insertLine(tableInfo->numField, COLUMN_WIDTH);
    printf("|");
    for (k = 0; k < tableInfo->numField; k++) {
        switch (tableInfo->fieldInfo[k].dataType) {
            case TYPE_INTEGER:
                printf("%10s |", tableInfo->fieldInfo[k].name);
                break;
            case TYPE_STRING:
                printf("%10s |", tableInfo->fieldInfo[k].name);
                break;
            default:
                /* ここにくることはないはず */
                freeTableInfo(tableInfo);
                return;
                break;
        }
        
    }
    printf("\n");
    insertLine(tableInfo->numField, COLUMN_WIDTH);


    /* ページ数分だけ繰り返す */
    for (i=0; i<numPage; ++i) {
        readPage(file, i, page);

        /*スロットの数*/
        memcpy(&numRecordSlot, page, sizeof(int));

        p = page + sizeof(int);

        /* スロットを見ていく */
        for (j=0; j<numRecordSlot; ++j) {
            int intValue;
            char stringValue[MAX_STRING];
            int stringLen;

            memcpy(&recordSlot.flag, p, sizeof(char));
            memcpy(&recordSlot.offset, p+sizeof(char), sizeof(int));
            memcpy(&recordSlot.size,p+sizeof(char)+sizeof(int), sizeof(int));

            q = page + recordSlot.offset;

            /* レコードがあったら表示 */
            if(recordSlot.flag == 1){
                numRecord++;

                printf("|");

                for (k = 0; k < tableInfo->numField; k++) {
                    switch (tableInfo->fieldInfo[k].dataType) {
                        case TYPE_INTEGER:
                            /* 整数の時、表示 */
                            memcpy(&intValue, q, sizeof(int));
                            printf("%10d |", *q);
                            q += sizeof(int);
                            break;
                        case TYPE_STRING:
                            /* 文字列の時、表示 */
                            memcpy(&stringLen, q, sizeof(int));
                            q += sizeof(int);
                            strcpy(stringValue, q);
                            printf("%10s |", stringValue);
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

    insertLine(tableInfo->numField, 12);
    printf("%d rows in set\n", numRecord);

    return;

}

/*
 * insertLine -- レコード表示時の罫線を室力
 *
 * 引数:
 *	n: レコード数
 *  m: カラム幅
 */
void insertLine(int n, int m){
    int i;
    for (i = 0; i < n*m+1; i++) {
        if(i%m == 0){printf("+");}
        else{printf("-");}
    }
    printf("\n");
}

/*
 * printRecordSet -- レコード集合の表示
 *
 * 引数:
 *	recordSet: 表示するレコード集合
 */
void printRecordSet(RecordSet *recordSet)
{
    RecordData *record;
    int i;

    /* レコード数の表示 */
    printf("Number of Records: %d\n", recordSet->numRecord);
    record = recordSet->recordData;
    /* レコードを1つずつ取りだし、表示する */
    for (; record != NULL; record = record->next) {
        /* すべてのフィールドのフィールド名とフィールド値を表示する */
        for (i = 0; i < record->numField; i++) {
            printf("Field %s = ", record->fieldData[i].name);

            switch (record->fieldData[i].dataType) {
                case TYPE_INTEGER:
                    printf("%d\n", record->fieldData[i].intValue);
                    break;
                case TYPE_STRING:
                    printf("%s\n", record->fieldData[i].stringValue);
                    break;
                default:
                    /* ここに来ることはないはず */
                    return;
            }
        }

        printf("\n");
    }
}
