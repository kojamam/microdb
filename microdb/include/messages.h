/*
 * errormsg.c -- 表示メッセージ管理ヘッダファイル
 */

#ifndef INCLUDE_MESSAGE_H
#define INCLUDE_MESSAGE_H

/* システムメッセージ番号 */
typedef enum SystemMessageNo {
    SYS_MSG_START,
    SYS_MSG_PROMPT,
    SYS_MSG_QUIT,
    SYS_MSG_INVALID_INPUT,
    SYS_MSG_TRY_AGAIN,
    SYS_MSG_TOO_MANY_FIELDS,
    SYS_MSG_SUCCESS_CREATE,
    SYS_MSG_SUCCESS_INSERT,
    SYS_MSG_SUCCESS_DROP,
    SYS_MSG_INVALID_ARG,
    SYS_MSG_INVALID_COND,
    SYS_MSG_TABLE_NOT_EXIST,
    SYS_MSG_FIELD_NOT_EXIST,
    SYS_MSG_NUM_RECORD_FOUND
} SystemMessageNo;

/* システムメッセージ */
static char *systemMessage[] = {
    "マイクロDBMSを起動しました。",
    "DDLまたはDMLを入力してください。",
    "マイクロDBMSを終了します。",
    "入力に間違いがあります。",
    "もう一度入力し直してください。",
    "フィールド数が上限を超えています。",
    "テーブルを作成しました。",
    "レコードを挿入しました。",
    "テーブルを削除しました。",
    "値が不正です。",
    "条件式の指定に間違いがあります。",
    "指定したテーブルは存在しません。",
    "指定したフィールドが存在しません。",
    "件見つかりました。"
};

/* エラーメッセージ番号 */
typedef enum ErrorMessageNo {
    ERR_MSG_INITIALIZE_FILE_MODULE,
    ERR_MSG_INITIALIZE_DATADEF_MODULE,
    ERR_MSG_INITIALIZE_DATAMANIP_MODULE,
    ERR_MSG_CREATE,
    ERR_MSG_DROP,
    ERR_MSG_INSERT,
    ERR_MSG_SELECT,
    ERR_MSG_DELETE,
    ERR_MSG_UNKNOWN_TYPE
} ErrorMessageNo;

/* エラーメッセージ */
static char *errorMessage[] = {
    "Cannot initialize file module.",
    "Cannot initialize data definition module.",
    "Cannot initialize data manipulation module.",
    "Cannot create table.",
    "Cannot drop table.",
    "Cannot insert record",
    "Cannot select record",
    "Cannot delete record",
    "Unknown data type found."
};

#endif