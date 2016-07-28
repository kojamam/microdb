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
Result initializeDataManipModule(){
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
Result finalizeDataManipModule(){
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
*/
static int getRecordSize(RecordData *recordData, TableInfo *tableInfo){
    int total = 0;
    int i;


    for (i=0; i < tableInfo->numField; i++) {
        /* i番目のフィールドがINT型かSTRING型か調べる */
        /* INT型ならtotalにsizeof(int)を加算 */
        /* STRING型ならtotalにfieldDataのstrlen+1を加算 */
        switch (tableInfo->fieldInfo[i].dataType) {
            case TYPE_INT:
                total += sizeof(int);
                break;
            case TYPE_DOUBLE:
                total += sizeof(double);
                break;
            case TYPE_VARCHAR:
                /* stlen +文字数格納のためのint + null文字 */
                total += strlen(recordData->fieldData[i].val.stringVal) + sizeof(int) + 1;
                break;
            default:
                /* ここにくることはないはず */
                freeTableInfo(tableInfo);
                free(recordData);
                return -1;
        }

    }

    return total;
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
static char* createRecordString(TableInfo *tableInfo, RecordData *recordData, int recordSize){
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
            case TYPE_INT:
                /* 整数の時、そのままコピーしてポインタを進める */
                memcpy(p, &recordData->fieldData[i].val.intVal, sizeof(int));
                p += sizeof(int);
                break;
            case TYPE_DOUBLE:
                /* 小数の時、そのままコピーしてポインタを進める */
                memcpy(p, &recordData->fieldData[i].val.doubleVal, sizeof(double));
                p += sizeof(double);
                break;
            case TYPE_VARCHAR:
                /* 文字列の時、文字列の長さを先頭に格納してから文字列を格納 */
                stringLen = (int)strlen(recordData->fieldData[i].val.stringVal);
                memcpy(p, &stringLen, sizeof(int));
                p += sizeof(int);
                strcpy(p, recordData->fieldData[i].val.stringVal);
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
static RecordSlot* readSlotFromPage(char *page, int n){
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
static Result writeSlotToPage(char *page, RecordSlot *slot){
    char *p;

    p = page;
    p += sizeof(int) + (sizeof(char) + sizeof(int) + sizeof(int)) * slot->num;

    memcpy(p, &slot->flag, sizeof(char));
    memcpy(p+sizeof(char), &slot->offset, sizeof(int));
    memcpy(p+sizeof(char)+sizeof(int), &slot->size, sizeof(int));

    /*スロットの解放*/
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
static int changeNumSlot(char *page, int delta){
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
static Result initializePage(char *page){
    int numSlot = 1;
    RecordSlot *slot;

    /* メモリの確保 */
    if((slot = (RecordSlot*)malloc(sizeof(RecordSlot))) == NULL){
        return NG;
    }

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
    if(writeSlotToPage(page, slot) != OK){
        return NG;
    }

    return OK;
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
Result insertRecord(char *tableName, RecordData *recordData){
    assert(strcmp(tableName, "") != 0);
    assert(recordData != NULL);

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

    if((recordSize = getRecordSize(recordData, tableInfo)) < 0){
        return NG;
    }

    if((recordString = createRecordString(tableInfo, recordData, recordSize)) == NULL){
        return NG;
    }

    /* ファイルオープン */
    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
    if((file = openFile(filename)) == NULL){
        free(tableInfo);
        free(recordString); //エラー処理
        return NG;
    }

    /* データファイルのページ数を調べる */
    if((numPage = getNumPages(filename)) < 0){
        free(tableInfo);
        free(recordString); //エラー処理
        closeFile(file);
        return NG;
    }

    /* ページごとにデータを挿入できる空きを探す */
    for (i = 0; i < numPage; i++) {

        /* 1ページ分のデータを読み込む */
        if (readPage(file, i, page) != OK) {
            free(tableInfo);
            free(recordString); //エラー処理
            closeFile(file);
            return NG;
        }

        /* スロット個数を読み込む */
        memcpy(&numRecordSlot, page, sizeof(int));
        q = page + sizeof(int);

        /* スロットを見て空きを探す */
        for (j=0; j<numRecordSlot; ++j) {

            if((recordSlot = readSlotFromPage(page, j)) == NULL){
                free(tableInfo);
                free(recordString); //エラー処理
                closeFile(file);
                return NG;
            }

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


                if(writeSlotToPage(page, recordSlot) != OK){
                    free(tableInfo);
                    free(recordString);
                    free(recordSlot);
                    closeFile(file);
                    return NG;
                }

                /* 空きが残っていたらスロットを追加 */
                if(newRecordSlot->size > 0){
                    if(writeSlotToPage(page, newRecordSlot) != OK
                       || changeNumSlot(page, 1) < 0){
                        free(tableInfo);
                        free(recordString);
                        free(recordSlot);
                        closeFile(file);
                        return NG;
                    }
                }

                if(writePage(file, i, page) != OK
                   || closeFile(file) != OK){
                    free(tableInfo);
                    free(recordString);
                    free(recordSlot);
                    closeFile(file);
                    return NG;
                }

                return OK;

            }

            /* 次のスロットを見る */
            q += sizeof(char) + sizeof(int) * 2;
        }/* スロット繰り返し */

    }/* ページ繰り返し */

    /* 空きがなかったら新規ページ作成 */
    if(initializePage(page) != OK){
        free(tableInfo);
        free(recordString);
        closeFile(file);
        return NG;
    }
    
    /*先頭のスロットを読み込み*/
    if((recordSlot = readSlotFromPage(page, 0)) == NULL){
        free(tableInfo);
        free(recordString);
        closeFile(file);
        return NG;
    }
    /*ページにレコードを書き込み*/
    memcpy(page + recordSlot->offset + recordSlot->size - recordSize, recordString, recordSize);

    /* レコードが増えたので空き領域を表す新しいスロットを作成 */
    RecordSlot *newRecordSlot = (RecordSlot*)malloc(sizeof(RecordSlot));
    newRecordSlot->num = 1;
    newRecordSlot->flag = 0;
    newRecordSlot->offset = recordSlot->offset;
    newRecordSlot->size = recordSlot->size - recordSize;

    /*元のスロットを更新*/
    recordSlot->flag = 1;
    recordSlot->offset = PAGE_SIZE - recordSize;
    recordSlot->size = recordSize;

    /*スロットを書き込み*/
    if(writeSlotToPage(page, recordSlot) != OK
       || writeSlotToPage(page, newRecordSlot) != OK
       || changeNumSlot(page, 1) < 0
       || writePage(file, numPage, page) != OK
       || closeFile(file) != OK){
        return NG;
    }

    return OK;
}

/*
 * checkDuplication -- レコードが重複しているかをチェック
 *
 * 引数:
 *	recordChecked: チェックするレコード
 *  tableInfo:  テーブル情報
 *  recordSet:  重複のチェック先
 *
 * 返り値:
 *	重複していなければOK, 重複していればNG, エラーはNULL
 */
static Result checkDuplication(RecordData *recordChecked, RecordSet *recordSet){
    assert(recordChecked != NULL);
    assert(recordSet != NULL);

    typedef enum { SAME = 0, DIFFERNT = 1 } Flag;

    int i, j;
    RecordData *record;
    Flag fieldFlag;

    record = recordSet->recordData;

    for (i=0; i<recordSet->numRecord; ++i) {
        for (j=0; j<record->numField; ++j) {
            switch (record->fieldData[j].dataType) {
                case TYPE_INT:
                    /* 整数の時、比較して違っていたら次へ */
                    if(record->fieldData[j].val.intVal == recordChecked->fieldData[j].val.intVal){
                        fieldFlag = SAME;
                    }else{
                        fieldFlag = DIFFERNT;
                    }
                    break;
                case TYPE_DOUBLE:
                    /* 小数の時、比較して違っていたら次へ */
                    if(record->fieldData[j].val.doubleVal == recordChecked->fieldData[j].val.doubleVal){
                        fieldFlag = SAME;
                    }else{
                        fieldFlag = DIFFERNT;
                    }
                    break;
                case TYPE_VARCHAR:
                    /* 文字列の時、比較して違っていたらOKを返す */
                    if (strcmp(record->fieldData[j].val.stringVal, recordChecked->fieldData[j].val.stringVal) == 0) {
                        fieldFlag = SAME;
                    }else{
                        fieldFlag = DIFFERNT;
                    }
                    break;
                default:
                    /* ここにくることはないはず */
                    freeRecordSet(recordSet);
                    return NG;
            }

            /*フィールド値が違っていたらその後のレコードを読むのをやめる*/
            if(fieldFlag == DIFFERNT){
                break;
            }

            /*最後のフィールドまで読んで、同じだったらレコードが重複しているのでNGを返す*/
            if(j == record->numField - 1){
                return NG;
            }

        }/* レコードの読み込み終わり */

        /*次のrecordを読む */
        record = record->next;

    }

    /* ここまで来たら重複なし */
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
static Result checkCondition(RecordData *recordData, Condition *condition){
    assert(recordData != NULL);
    assert(condition != NULL);
    
    int i;
    OperatorType opType = condition->operator;
    int diff;

    for(i=0; i<recordData->numField; ++i){
        if(strcmp(recordData->fieldData[i].name, condition->name) == 0){
            switch (recordData->fieldData[i].dataType) {
                case TYPE_INT:
                    diff = recordData->fieldData[i].val.intVal - condition->val.intVal;
                    break;
                case TYPE_DOUBLE:
                    diff = recordData->fieldData[i].val.doubleVal - condition->val.doubleVal;
                    break;
                case TYPE_VARCHAR:
                    diff = strcmp(recordData->fieldData[i].val.stringVal, condition->val.stringVal);
                    break;
                default:
                    /*ここに来ることはないはず*/
                    diff = 0;
                    break;
            }

            if((opType == OPR_EQUAL && diff == 0)
               || (opType == OPR_NOT_EQUAL && diff != 0)
               || (opType == OPR_GREATER_THAN && diff > 0)
               || (opType == OPR_OR_GREATER_THAN && diff >= 0)
               || (opType == OPR_LESS_THAN && diff < 0)
               || (opType == OPR_OR_LESS_THAN && diff <= 0) ){
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
RecordSet *selectRecord(char *tableName, FieldList *fieldList, Condition *condition){
    assert(strcmp(tableName, "") != 0);
    assert(condition != NULL);
    assert(fieldList != NULL);

    RecordSet *recordSet;
    char filename[MAX_FILENAME];
    File *file;
    int numPage;
    TableInfo *tableInfo;
    int i, j, k, l, m, n;
    char page[PAGE_SIZE];
    int numRecordSlot;
    char *q;
    RecordSlot *recordSlot;
    int isIncluded = 0;

    /* recordSetを初期化 */
    recordSet = (RecordSet*)malloc(sizeof(RecordSet));
    recordSet->numRecord = 0;
    recordSet->recordData = NULL;

    /*ファイルをオープン*/
    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
    if((file = openFile(filename)) == NULL){
        return NULL;
    }

    if((numPage = getNumPages(filename)) < 0){
        closeFile(file);
        return NULL;
    }

    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        closeFile(file);
        return NULL;
    }

    /* ページ数分だけ繰り返す */
    for (i=0; i<numPage; ++i) {
        if(readPage(file, i, page) != OK){
            closeFile(file);
            freeTableInfo(tableInfo);
            return NULL;
        }

        /*スロットの数を取得*/
        memcpy(&numRecordSlot, page, sizeof(int));

        /* スロットを見ていく */
        for (j=0; j<numRecordSlot; ++j) {
            
            /* ページからスロットを読み込み */
            if((recordSlot = readSlotFromPage(page, j)) == NULL){
                closeFile(file);
                freeTableInfo(tableInfo);
                return NULL;
            }

            q = page + recordSlot->offset;

            /* レコードがあったら読み込み */
            if(recordSlot->flag == 1){
                RecordData *recordData1 = (RecordData*)malloc(sizeof(RecordData));
                recordData1->numField = 1;
                recordData1->next = NULL;

                RecordData *recordData2 = (RecordData*)malloc(sizeof(RecordData));
                recordData2->next = NULL;
                int stringLen;

                /* k tableInfo->fieldData[k]
                 * l recordData1->fieldData[l]
                 * m recordData2->fieldData[m]
                 * n fieldList->name[n]
                */
                l = m = 0;
                /* フィールド繰り返し */
                for (k = 0; k < tableInfo->numField; k++) {
                    /*コンディションがあって、対象のフィールドであるときrecordData1に格納 */
                    if(condition != NULL && strcmp(condition->name, tableInfo->fieldInfo[k].name) == 0){
                        strcpy(recordData1->fieldData[l].name, tableInfo->fieldInfo[k].name);
                        recordData1->fieldData[l].dataType = tableInfo->fieldInfo[k].dataType;
                        switch (tableInfo->fieldInfo[k].dataType) {
                            case TYPE_INT:
                                /* 整数の時、そのままコピー */
                                memcpy(&recordData1->fieldData[l].val.intVal, q, sizeof(int));
                                break;
                            case TYPE_DOUBLE:
                                /* 小数の時、そのままコピー */
                                memcpy(&recordData1->fieldData[l].val.doubleVal, q, sizeof(double));
                                break;
                            case TYPE_VARCHAR:
                                /* 文字列の時、文字列長を飛ばして文字列を格納 */
                                strcpy(recordData1->fieldData[l].val.stringVal, q+sizeof(int));
                                break;
                            default:
                                /* ここにくることはないはず */
                                closeFile(file);
                                freeTableInfo(tableInfo);
                                free(recordSlot);
                                free(recordData1);
                                free(recordData2);
                                return NULL;
                        }
                        l++;
                    }

                    /* fieldListがあって、そのフィールドが存在するかを確認してisIncludedフラグを立てる */
                    if(fieldList->numField > 0){
                        for(n=0; n < fieldList->numField; n++){
                            if(strcmp(fieldList->name[n], tableInfo->fieldInfo[k].name) == 0){
                                isIncluded = 1;
                                break;
                            }else{
                                isIncluded = 0;
                            }
                        }
                    }else{
                        isIncluded = 1;
                    }

                    /* fieldListにデータが含まれていたらrecordData2に格納 */
                    if(isIncluded){
                        strcpy(recordData2->fieldData[m].name, tableInfo->fieldInfo[k].name);
                        recordData2->fieldData[m].dataType = tableInfo->fieldInfo[k].dataType;
                    }
                    switch (tableInfo->fieldInfo[k].dataType) {
                        case TYPE_INT:
                            /* 整数の時、そのままコピーしてポインタを進める */
                            if(isIncluded){
                                memcpy(&recordData2->fieldData[m++].val.intVal, q, sizeof(int));
                            }
                            q += sizeof(int);
                            break;
                        case TYPE_DOUBLE:
                            /* 小数の時、そのままコピーしてポインタを進める */
                            if(isIncluded){
                                memcpy(&recordData2->fieldData[m++].val.doubleVal, q, sizeof(double));
                            }
                            q += sizeof(double);
                            break;
                        case TYPE_VARCHAR:
                            /* 文字列の時、文字列の長さを先頭に格納してから文字列を格納 */
                            memcpy(&stringLen, q, sizeof(int));
                            q += sizeof(int);
                            if(isIncluded){
                                strcpy(recordData2->fieldData[m++].val.stringVal, q);
                            }
                            q += stringLen+1; // '\0'の分も進む
                            break;
                        default:
                            /* ここにくることはないはず */
                            closeFile(file);
                            freeTableInfo(tableInfo);
                            free(recordSlot);
                            free(recordData1);
                            free(recordData2);
                            return NULL;
                    }
                }/* レコードの読み込み終わり */

                recordData2->numField = m;

                /*条件が無い時か、あって満足するかを確認して満たしていたら*/
                if(strcmp(condition->name, "") == 0 || checkCondition(recordData1, condition) == OK){
                    /*DISTINCTが無い時か、あって重複してないならRecordSetの末尾に追加*/
                    if (condition->distinct == NOT_DISTINCT || checkDuplication(recordData2, recordSet) == OK) {
                        recordSet->numRecord++;
                        if(recordSet->recordData == NULL){
                            recordSet->recordData = recordData2;
                        }else{
                            RecordData *tmpRecordData = recordSet->recordData;
                            while (tmpRecordData->next != NULL) {
                                tmpRecordData = tmpRecordData->next;
                            }
                            tmpRecordData->next = recordData2;
                        }
                    }else{
                        free(recordData2);
                    }
                }else{
                    free(recordData2);
                }
                free(recordData1);
                free(recordSlot);
            }/* レコード読み込みおわり */

        }/* スロット繰り返し */

    }/*ページ繰り返し*/
    
    freeTableInfo(tableInfo);

    if(closeFile(file) != OK){
        return NULL;
    }
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
void freeRecordSet(RecordSet *recordSet){
    RecordData *recordData, *tmp;

    recordData = recordSet->recordData;
    free(recordSet);

    if(recordData == NULL){
        return;
    }

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
Result deleteRecord(char *tableName, Condition *condition){

    assert(strcmp(tableName, "") != 0);
    assert(condition != NULL);

    char filename[MAX_FILENAME];
    File *file;
    int numPage;
    TableInfo *tableInfo;
    int i, j, k;
    char page[PAGE_SIZE];
    int numRecordSlot;
    char *q;
    RecordSlot *recordSlot;


    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);
    if((file = openFile(filename)) == NULL){
        return NG;
    }

    if((numPage = getNumPages(filename)) < 0){
        closeFile(file);
        return NG;
    }
    
    /*テーブル情報の取得*/
    if((tableInfo = getTableInfo(tableName)) == NULL){
        closeFile(file);
        return NG;
    }

    /* ページ数分だけ繰り返す */
    for (i=0; i<numPage; ++i) {
        if(readPage(file, i, page) != OK){
            closeFile(file);
            freeTableInfo(tableInfo);
            return NG;
        }

        /*スロットの数*/
        memcpy(&numRecordSlot, page, sizeof(int));

        /* スロットを見ていく */
        for (j=0; j<numRecordSlot; ++j) {
            if((recordSlot = readSlotFromPage(page, j)) == NULL){
                closeFile(file);
                freeTableInfo(tableInfo);
                return NG;
            }

            q = page + recordSlot->offset;

            /* レコードがあったら読み込み */
            if(recordSlot->flag == 1){
                RecordData *recordData = (RecordData*)malloc(sizeof(RecordData));
                recordData->numField = tableInfo->numField;
                int stringLen;

                for (k = 0; k < tableInfo->numField; k++) {
                    strcpy(recordData->fieldData[k].name, tableInfo->fieldInfo[k].name);
                    recordData->fieldData[k].dataType = tableInfo->fieldInfo[k].dataType;
                    switch (tableInfo->fieldInfo[k].dataType) {
                        case TYPE_INT:
                            /* 整数の時、そのままコピーしてポインタを進める */
                            memcpy(&recordData->fieldData[k].val.intVal, q, sizeof(int));
                            q += sizeof(int);
                            break;
                        case TYPE_DOUBLE:
                            /* 小数の時、そのままコピーしてポインタを進める */
                            memcpy(&recordData->fieldData[k].val.doubleVal, q, sizeof(double));
                            q += sizeof(double);
                            break;
                        case TYPE_VARCHAR:
                            /* 文字列の時、文字列の長さを先頭に格納してから文字列を格納 */
                            memcpy(&stringLen, q, sizeof(int));
                            q += sizeof(int);
                            strcpy(recordData->fieldData[k].val.stringVal, q);
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
                if(strcmp(condition->name, "") == 0 || checkCondition(recordData, condition) == OK){
                    /* 0埋め */
                    memset(page+recordSlot->offset, 0, recordSlot->size);
                    /* スロットの更新*/
                    recordSlot->flag = 0;
                    if(writeSlotToPage(page, recordSlot) != OK){
                        closeFile(file);
                        freeTableInfo(tableInfo);
                        free(recordSlot);
                        return NG;
                    }
                }/* TODO 隣のスロットと統合 */
                free(recordData);
            }

        }/* スロット繰り返し */

        if(writePage(file, i, page)  != OK){
            closeFile(file);
            freeTableInfo(tableInfo);
            return NG;
        }

    }/*ページ繰り返し*/
    
    freeTableInfo(tableInfo);


    if(closeFile(file) != OK){
        closeFile(file);
        return NG;
    }
    
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
Result createDataFile(char *tableName){
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
Result deleteDataFile(char *tableName){
    char filename[MAX_FILENAME];

    sprintf(filename, "%s%s", tableName, DATA_FILE_EXT);

    if(deleteFile(filename) != OK){
        return NG;
    }

    return OK;
}
