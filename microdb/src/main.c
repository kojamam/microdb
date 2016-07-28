/*
 * main.c -- メインモジュール
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../include/microdb.h"

/*
 * MAX_INPUT -- 入力行の最大文字数
 */
#define MAX_INPUT 256

/*
 * inputString -- 字句解析中の文字列を収める配列
 *
 * 大きさを「3 * MAX_INPUT」とするのは、入力行のすべての文字が区切り記号でも
 * バッファオーバーフローを起こさないようにするため。
 */
static char inputString[3 * MAX_INPUT];

/*
 * nextPosition -- 字句解析中の場所
 */
static char *nextPosition;

/*
 * setInputString -- 字句解析する文字列の設定
 *
 * 引数:
 *	string: 解析する文字列
 *
 * 返り値:
 *	なし
 */
static void setInputString(char *string)
{
    char *p = string;
    char *q = inputString;
    
    /* 入力行をコピーして保存する */
    while (*p != '\0') {
        /* 引用符の場合には、次の引用符まで読み飛ばす */
        if (*p == '\'' || *p == '"') {
            char *quote;
            quote = p;
            *q++ = *p++;
            
            /* 閉じる引用符(または文字列の最後)までコピーする */
            while (*p != *quote && *p != '\0') {
                *q++ = *p++;
            }
            
            /* 閉じる引用符自体もコピーする */
            if (*p == *quote) {
                *q++ = *p++;
            }
            continue;
        }
        
        /* 区切り記号の場合には、その前後に空白文字を入れる */
        if (*p == ',' || *p == '(' || *p == ')' || *p == '.') {
            *q++ = ' ';
            *q++ = *p++;
            *q++ = ' ';
        } else {
            *q++ = *p++;
        }
    }
    
    *q = '\0';
    
    /* getNextToken()の読み出し開始位置を文字列の先頭に設定する */
    nextPosition = inputString;
}

/*
 * getNextToken -- setInputStringで設定した文字列からの字句の取り出し
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	次の字句の先頭番地を返す。最後までたどり着いたらNULLを返す。
 */
static char *getNextToken()
{
    char *start;
    char *end;
    char *p;
    
    /* 空白文字が複数続いていたら、その分nextPositionを移動させる */
    while (*nextPosition == ' ') {
        nextPosition++;
    }
    start = nextPosition;
    
    /* nextPositionの位置が文字列の最後('\0')だったら、解析終了 */
    if (*nextPosition == '\0') {
        return NULL;
    }
    
    /* nextPositionの位置以降で、最初に見つかる空白文字の場所を探す */
    p = nextPosition;
    while (*p != ' ' && *p != '\0') {
        /* 引用符の場合には、次の引用符まで読み飛ばす */
        if (*p == '\'' || *p == '"') {
            char *quote;
            quote = p;
            p++;
            
            /* 閉じる引用符(または文字列の最後)まで読み飛ばす */
            while (*p != *quote && *p != '\0') {
                p++;
            }
            
            /* 閉じる引用符自体も読み飛ばす */
            if (*p == *quote) {
                p++;
            }
        } else {
            p++;
        }
        
    }
    end = p;
    
    /*
     * 空白文字を終端文字で置き換えるとともに、
     * 次回のgetNextToken()の呼び出しのために
     * nextPositionをその次の文字に移動する
     */
    if (*end != '\0') {
        *end = '\0';
        nextPosition = end + 1;
    } else {
        /*
         * (*end == '\0')の場合は文字列の最後まで解析が終わっているので、
         * 次回のgetNextToken()の呼び出しのときにNULLが返るように、
         * nextPositionを文字列の一番最後に移動させる
         */
        nextPosition = end;
    }
    
    /* 字句の先頭番地を返す */
    return start;
}

/*
 * callCreateTable -- create文の構文解析とcreateTableの呼び出し
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	なし
 *
 * create tableの書式:
 *	create table テーブル名 ( フィールド名 データ型, ... )
 */
void callCreateTable()
{
    char *token;
    char *tableName;
    int numField;
    TableInfo tableInfo;
    
    /* createの次のトークンを読み込み、それが"table"かどうかをチェック */
    token = getNextToken();
    if (token == NULL || strcmp(token, "table") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* テーブル名を読み込む */
    if ((tableName = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* 次のトークンを読み込み、それが"("かどうかをチェック */
    token = getNextToken();
    if (token == NULL || strcmp(token, "(") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /*
     * ここから、フィールド名とデータ型の組を繰り返し読み込み、
     * 配列fieldInfoに入れていく。
     */
    numField = 0;
    for(;;) {
        /* フィールド名の読み込み */
        if ((token = getNextToken()) == NULL) {
            /* 文法エラー */
            printf("入力行に間違いがあります。\n");
            return;
        }
        
        /* 読み込んだトークンが")"だったら、ループから抜ける */
        if (strcmp(token, ")") == 0) {
            break;
        }
        
        /* フィールド名を配列に設定 */
        strcpy(tableInfo.fieldInfo[numField].name, token);
        
        /* 次のトークン(データ型)の読み込み */
        token = getNextToken();
        
        /* データ型を配列に設定 */
        if(strcmp(token, "int") == 0){
            tableInfo.fieldInfo[numField].dataType = TYPE_INTEGER;
        }else if(strcmp(token, "string") == 0){
            tableInfo.fieldInfo[numField].dataType = TYPE_STRING;
        }else{
            tableInfo.fieldInfo[numField].dataType = TYPE_UNKNOWN;
        }
        
        /* フィールド数をカウントする */
        numField++;
        
        /* フィールド数が上限を超えていたらエラー */
        if (numField > MAX_FIELD) {
            printf("フィールド数が上限を超えています。\n");
            return;
        }
        
        /* 次のトークンの読み込み */
        token = getNextToken();
        
        /* 読み込んだトークンが")"だったら、ループから抜ける */
        if (strcmp(token, ")") == 0) {
            break;
        } else if (strcmp(token, ",") == 0) {
            /* 次のフィールドを読み込むため、ループの先頭へ */
            continue;
        } else {
            /* 文法エラー */
            printf("入力行に間違いがあります。\n");
            return;
        }
    }
    
    tableInfo.numField = numField;
    
    /* createTableを呼び出し、テーブルを作成 */
    if (createTable(tableName, &tableInfo) == OK) {
        printf("テーブルを作成しました。\n");
        printTableInfo(tableName);
    } else {
        printf("テーブルの作成に失敗しました。\n");
    }
}

/*
 * callDropTable -- drop文の構文解析とdropTableの呼び出し
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	なし
 *
 * drop tableの書式:
 *	drop table テーブル名
 */
void callDropTable()
{
    char *token;
    char *tableName;
    
    /* createの次のトークンを読み込み、それが"table"かどうかをチェック */
    token = getNextToken();
    if (token == NULL || strcmp(token, "table") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* テーブル名を読み込む */
    if ((tableName = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* createTableを呼び出し、テーブルを作成 */
    if (dropTable(tableName) == OK) {
        printf("テーブルを削除しました。\n");
    } else {
        printf("テーブルの削除に失敗しました。\n");
    }
}

/*
 * callInsertRecord -- insert文の構文解析とinsertRecordの呼び出し
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	なし
 *
 * insertの書式:
 *	insert into テーブル名 values ( フィールド値 , ... )
 */
void callInsertRecord()
{
    char *token;
    char *tableName;
    TableInfo *tableInfo;
    RecordData recordData;
    int i, j;
    
    /* insertの次のトークンを読み込み、それが"into"かどうかをチェック */
    token = getNextToken();
    if (token == NULL || strcmp(token, "into") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* テーブル名を読み込む */
    if ((tableName = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* tableInfoを読み込み */
    if((tableInfo = getTableInfo(tableName)) == NULL){
        /* 文法エラー */
        printf("指定したテーブルは存在しません。\n");
        return;
    }
    
    /* 次のトークンを読み込み、それが"("かどうかをチェック */
    token = getNextToken();
    if (token == NULL || strcmp(token, "(") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    recordData.numField = tableInfo->numField;

    for(i=0; i<tableInfo->numField; ++i){
        /*フィールド名とデータタイプを設定*/
        strcpy(recordData.fieldData[i].name, tableInfo->fieldInfo[i].name);
        recordData.fieldData[i].dataType = tableInfo->fieldInfo[i].dataType;

        /* フィールド値の読み込み */
        token = getNextToken();
        if (token == NULL || strcmp(token, ")") == 0) {
            /* 文法エラー */
            printf("入力行に間違いがあります。\n");
            return;
        }
        
        if (tableInfo->fieldInfo[i].dataType == TYPE_INTEGER) {
            /* トークンの文字列を整数値に変換して設定 */
            recordData.fieldData[i].intValue = atoi(token);
        }else if(tableInfo->fieldInfo[i].dataType == TYPE_STRING){
            char stringVal[MAX_STRING];
            /* はじめが'であるかをチェック */
            if(token == NULL || token[0] != '\''){
                /* 文法エラー */
                printf("条件式の指定に間違いがあります。\n");
                return;
            }
            
            /* 'の次からコピーしていく */
            for(j = 0; token[j+1] != '\0'; j++){
                stringVal[j] = token[j+1];
            }
            
            /* 'でしめられていたらそれを\0にしてコピーする */
            if(stringVal[j-1] == '\''){
                stringVal[j-1] = '\0';
                strcpy(recordData.fieldData[i].stringValue, stringVal);
            }else{
                printf("値が不正です。\n");
                return;
            }

        } else {
            /* ここに来ることはないはず */
            fprintf(stderr, "Unknown data type found.\n");
            exit(1);
        }
        
        /* 次のトークンを読み込み、それが","なら次のフィールドを読み込み */
        token = getNextToken();
        if (strcmp(token, ",") == 0) {
            continue;
        }
    }
    
    /* トークンが")"かどうかをチェック */
    if (token == NULL || strcmp(token, ")") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    recordData.next = NULL;
    
    insertRecord(tableName, &recordData);
    
}

/*
 * callSelectRecord -- select文の構文解析とselectRecordの呼び出し
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	なし
 *
 * selectの書式:
 *	select * from テーブル名 where 条件式
 *	select フィールド名 , ... from テーブル名 where 条件式 (発展課題)
 */
void callSelectRecord()
{
    char *token;
    char *tableName;
    TableInfo *tableInfo;
    FieldList fieldList;
    Condition cond;
    RecordSet *recordSet;
    int i, numField;
    
    /*fieldListを初期化*/
    fieldList.numField = -1;
    
    /*conditonを初期化*/
    strcpy(cond.name, "");
    cond.dataType = TYPE_UNKNOWN;
    cond.operator = OPR_UNKNOWN;
    strcpy(cond.stringValue, "");
    cond.distinct = NOT_DISTINCT;
    
    /* selectの次のトークンを読み込み、それが"*"かどうかをチェック */
    token = getNextToken();
    if (token == NULL) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    if(strcmp(token, "distinct")==0){
        cond.distinct = DISTINCT;
        token = getNextToken();
    }else{
        cond.distinct = NOT_DISTINCT;
    }
    
    if(strcmp(token, "*") != 0){
        numField = 0;
        for(;;) {
            
            /* フィールド名を配列に設定 */
            strcpy(fieldList.name[numField], token);
            
            /* フィールド数をカウントする */
            numField++;
            
            /* 次のトークンの読み込み */
            token = getNextToken();
            
            /* 読み込んだトークンが")"だったら、ループから抜ける */
            if (strcmp(token, "from") == 0) {
                break;
            } else if (strcmp(token, ",") == 0) {
                /* 次のフィールドを読み込むため、ループの先頭へ */
                token = getNextToken();
                continue;
            } else {
                /* 文法エラー */
                printf("入力行に間違いがあります。\n");
                return;
            }
        }

        fieldList.numField = numField;

    }else{
        /* 次のトークンを読み込み、それが"from"かどうかをチェック */
        token = getNextToken();
        if (token == NULL || strcmp(token, "from") != 0) {
            /* 文法エラー */
            printf("入力行に間違いがあります。\n");
            return;
        }
    }

    /* テーブル名を読み込む */
    if ((tableName = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* tableInfoを読み込み */
    if((tableInfo = getTableInfo(tableName)) == NULL){
        /* 文法エラー */
        printf("指定したテーブルは存在しません。\n");
        return;
    }
    
    /* 次のトークンを取得 */
    token = getNextToken();
    /* "select * from TABLENAME" のように条件句がない時 */
    if(token == NULL){
        /*selectRecoredの呼び出し*/
        if ((recordSet = selectRecord(tableName, &fieldList, &cond)) == NULL) {
            fprintf(stderr, "Cannot select records.\n");
            return;
        }
    }else{
        /*条件句があるとき*/
        /* それが"where"かどうかをチェック */
        if (strcmp(token, "where") != 0) {
            /* 文法エラー */
            printf("入力行に間違いがあります。\n");
            return;
        }

        /* 条件式のフィールド名を読み込む */
        if ((token = getNextToken()) == NULL) {
            /* 文法エラー */
            printf("入力行に間違いがあります。\n");
            
            /* 不要になったメモリ領域を解放する */
            freeTableInfo(tableInfo);
            
            return;
        }
        strcpy(cond.name, token);

        /* 条件式に指定されたフィールドのデータ型を調べる */
        cond.dataType = TYPE_UNKNOWN;
        for (i = 0; i < tableInfo->numField; i++) {
            if (strcmp(tableInfo->fieldInfo[i].name, cond.name) == 0) {
                /* フィールドのデータ型を構造体に設定してループを抜ける */
                cond.dataType = tableInfo->fieldInfo[i].dataType;
                break;
            }
        }

        /* 不要になったメモリ領域を解放する */
        freeTableInfo(tableInfo);

        /* フィールドのデータ型がわからなければ、文法エラー */
        if (cond.dataType == TYPE_UNKNOWN) {
            printf("指定したフィールドが存在しません。\n");
            return;
        }

        /* 条件式の比較演算子を読み込む */
        if ((token = getNextToken()) == NULL) {
            /* 文法エラー */
            printf("条件式の指定に間違いがあります。\n");
            return;
        }

        /* 条件式の比較演算子を構造体に設定 */
        if(strcmp(token, "=") == 0){
            cond.operator = OPR_EQUAL;
        }else if(strcmp(token, "!=") == 0){
            cond.operator = OPR_NOT_EQUAL;
        }else if(strcmp(token, ">") == 0){
            cond.operator = OPR_GREATER_THAN;
        }else if(strcmp(token, ">=") == 0){
            cond.operator = OPR_OR_GREATER_THAN;
        }else if(strcmp(token, "<") == 0){
            cond.operator = OPR_LESS_THAN;
        }else if(strcmp(token, ">") == 0){
            cond.operator = OPR_OR_LESS_THAN;
        }else{
            /* 文法エラー */
            printf("条件式の指定に間違いがあります。\n");
            return;
        }

        /* 条件式の値を読み込む */
        if ((token = getNextToken()) == NULL) {
            /* 文法エラー */
            printf("条件式の指定に間違いがあります。\n");
            return;
        }

        /* 条件式の値を構造体に設定 */
        if (cond.dataType == TYPE_INTEGER) {
            /* トークンの文字列を整数値に変換して設定 */
            cond.intValue = atoi(token);
        } else if (cond.dataType == TYPE_STRING) {
            char stringVal[MAX_STRING];
            /* はじめが'であるかをチェック */
            if(token == NULL || token[0] != '\''){
                /* 文法エラー */
                printf("条件式の指定に間違いがあります。\n");
                return;
            }
            
            /* 'の次からコピーしていく */
            for(i = 0; token[i+1] != '\0'; i++){
                stringVal[i] = token[i+1];
            }
            
            /* 'でしめられていたらそれを\0にしてコピーする */
            if(stringVal[i-1] == '\''){
                stringVal[i-1] = '\0';
                strcpy(cond.stringValue, stringVal);
            }else{
                printf("条件式の指定に間違いがあります。\n");
                return;
            }
            
        } else {
            /* ここに来ることはないはず */
            fprintf(stderr, "Unknown data type found.\n");
            exit(1);
        }

        /*selectRecoredの呼び出し*/
        if ((recordSet = selectRecord(tableName, &fieldList, &cond)) == NULL) {
            fprintf(stderr, "Cannot select records.\n");
            return;
        }
    }
    
    /* 結果を表示 */
    printRecordSet(tableName, recordSet, &fieldList);
    /* 結果を解放 */
    freeRecordSet(recordSet);

}

/*
 * callDeleteRecord -- delete文の構文解析とdeleteRecordの呼び出し
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	なし
 *
 * deleteの書式:
 *	delete from テーブル名 where 条件式
 */
void callDeleteRecord(){
    char *token;
    char *tableName;
    TableInfo *tableInfo;
    Condition cond;
    int i;
    
    /*conditionを初期化*/
    strcpy(cond.name, "");
    cond.dataType = TYPE_UNKNOWN;
    cond.operator = OPR_UNKNOWN;
    strcpy(cond.stringValue, "");
    cond.intValue = 0;
    
    /* deleteの次のトークンを読み込み、それが"from"かどうかをチェック */
    token = getNextToken();
    if (token == NULL || strcmp(token, "from") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* テーブル名を読み込む */
    if ((tableName = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* tableInfoを読み込み */
    if((tableInfo = getTableInfo(tableName)) == NULL){
        /* 文法エラー */
        printf("指定したテーブルは存在しません。\n");
        return;
    }
    
    /*次のトークンの読み込み*/
    token = getNextToken();
    /* "delete from TABLENAME" のように条件句がない時 */
    if(token == NULL){
        deleteRecord(tableName, &cond);
        return;
    }
    
    /* "where"かどうかをチェック */
    if (strcmp(token, "where") != 0) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        return;
    }
    
    /* 条件式のフィールド名を読み込む */
    if ((token = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("入力行に間違いがあります。\n");
        
        /* 不要になったメモリ領域を解放する */
        freeTableInfo(tableInfo);
        
        return;
    }
    strcpy(cond.name, token);
    
    /* 条件式に指定されたフィールドのデータ型を調べる */
    cond.dataType = TYPE_UNKNOWN;
    for (i = 0; i < tableInfo->numField; i++) {
        if (strcmp(tableInfo->fieldInfo[i].name, cond.name) == 0) {
            /* フィールドのデータ型を構造体に設定してループを抜ける */
            cond.dataType = tableInfo->fieldInfo[i].dataType;
            break;
        }
    }
    
    /* 不要になったメモリ領域を解放する */
    freeTableInfo(tableInfo);
    
    /* フィールドのデータ型がわからなければ、文法エラー */
    if (cond.dataType == TYPE_UNKNOWN) {
        printf("指定したフィールドが存在しません。\n");
        return;
    }
    
    /* 条件式の比較演算子を読み込む */
    if ((token = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("条件式の指定に間違いがあります。\n");
        return;
    }
    
    /* 条件式の比較演算子を構造体に設定 */
    if(strcmp(token, "=") == 0){
        cond.operator = OPR_EQUAL;
    }else if(strcmp(token, "!=") == 0){
        cond.operator = OPR_NOT_EQUAL;
    }else if(strcmp(token, ">") == 0){
        cond.operator = OPR_GREATER_THAN;
    }else if(strcmp(token, ">=") == 0){
        cond.operator = OPR_OR_GREATER_THAN;
    }else if(strcmp(token, "<") == 0){
        cond.operator = OPR_LESS_THAN;
    }else if(strcmp(token, ">") == 0){
        cond.operator = OPR_OR_LESS_THAN;
    }else{
        /* 文法エラー */
        printf("条件式の指定に間違いがあります。\n");
        return;
    }
    
    /* 条件式の値を読み込む */
    if ((token = getNextToken()) == NULL) {
        /* 文法エラー */
        printf("条件式の指定に間違いがあります。\n");
        return;
    }
    
    /* 条件式の値を構造体に設定 */
    if (cond.dataType == TYPE_INTEGER) {
        /* トークンの文字列を整数値に変換して設定 */
        cond.intValue = atoi(token);
    } else if (cond.dataType == TYPE_STRING) {
        char stringVal[MAX_STRING];
        /* はじめが'であるかをチェック */
        if(token == NULL || token[0] != '\''){
            /* 文法エラー */
            printf("条件式の指定に間違いがあります。\n");
            return;
        }
        
        /* 'の次からコピーしていく */
        for(i = 0; token[i+1] != '\0'; i++){
            stringVal[i] = token[i+1];
        }
        
        /* 'でしめられていたらそれを\0にしてコピーする */
        if(stringVal[i-1] == '\''){
            stringVal[i-1] = '\0';
            strcpy(cond.stringValue, stringVal);
        }else{
            printf("条件式の指定に間違いがあります。\n");
            return;
        }
        
     } else {
         /* ここに来ることはないはず */
         fprintf(stderr, "Unknown data type found.\n");
         exit(1);
     }
    
    /*distinct関係ないので*/
    cond.distinct = NOT_DISTINCT;
    
    deleteRecord(tableName, &cond);
}

/*
 * main -- マイクロDBシステムのエントリポイント
 */
int main()
{
    char input[MAX_INPUT];
    char *token;
    char *line;
    
    /* ファイルモジュールの初期化 */
    if (initializeFileModule() != OK) {
        fprintf(stderr, "Cannot initialize file module.\n");
        exit(1);
    }
    
    /* データ定義ジュールの初期化 */
    if (initializeDataDefModule() != OK) {
        fprintf(stderr, "Cannot initialize data definition module.\n");
        exit(1);
    }
    
    /* データ操作ジュールの初期化 */
    if (initializeDataManipModule() != OK) {
        fprintf(stderr, "Cannot initialize data manipulation module.\n");
        exit(1);
    }
    
    /* ウェルカムメッセージを出力 */
    printf("マイクロDBMSを起動しました。\n");
    
    /* 1行ずつ入力を読み込みながら、処理を行う */
    for(;;) {
        /* プロンプトを出力して、キーボード入力を1行読み込む */
        printf("\nDDLまたはDMLを入力してください。\n");
        line = readline("> ");
        
        /* EOFになったら終了 */
        if (line == NULL) {
            printf("マイクロDBMSを終了します。\n\n");
            break;
        }
        
        /* 字句解析するために入力文字列を設定する */
        strncpy(input, line, MAX_INPUT);
        setInputString(input);
        
        /* 入力の履歴を保存する */
        if (line && *line) {
            add_history(line);
        }
        
        free(line);
        
        /* 最初のトークンを取り出す */
        token = getNextToken();
        
        /* 入力が空行だったら、ループの先頭に戻ってやり直し */
        if (token == NULL) {
            continue;
        }
        
        /* 入力が"quit"だったら、ループを抜けてプログラムを終了させる */
        if (strcmp(token, "quit") == 0) {
            printf("マイクロDBMSを終了します。\n\n");
            break;
        }
        
        /* 最初のトークンが何かによって、呼び出す関数を決める */
        if (strcmp(token, "create") == 0) {
            callCreateTable();
        } else if (strcmp(token, "drop") == 0) {
            callDropTable();
        } else if (strcmp(token, "insert") == 0) {
            callInsertRecord();
        } else if (strcmp(token, "select") == 0) {
            callSelectRecord();
        } else if (strcmp(token, "delete") == 0) {
            callDeleteRecord();
        } else {
            /* 入力に間違いがあった */
            printf("入力に間違いがあります。\n");
            printf("もう一度入力し直してください。\n\n");
        }
    }
    
    /* 各モジュールの終了処理 */
    finalizeDataManipModule();
    finalizeDataDefModule();
    finalizeFileModule();
}
