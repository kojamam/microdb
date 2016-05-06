/*
 * microdb.h - 共通定義ファイル
 */

/*
 * Result -- 成功/失敗を返す返り値
 */
typedef enum { OK, NG } Result;

/*
 * PAGE_SIZE -- ファイルアクセスの単位(バイト数)
 */
#define PAGE_SIZE 4096

/*
 * MAX_FILENAME -- オープンするファイルの名前の長さの上限
 */
#define MAX_FILENAME 256

/*
 * File - オープンしたファイルの情報を保持する構造体
 */
typedef struct File File;
struct File {
    int desc;                           /* ファイルディスクリプタ */
    char name[MAX_FILENAME];            /* ファイル名 */
};

/*
 * MAX_FIELD -- 1レコードに含まれるフィールド数の上限
 */
#define MAX_FIELD 40

/*
 * MAX_FIELD_NAME -- フィールド名の長さの上限(バイト数)
 */
#define MAX_FIELD_NAME 20

/*
 * dataType -- データベースに保存するデータの型
 */
typedef enum DataType DataType;
enum DataType {
    TYPE_UNKNOWN = 0,			/* データ型不明 */
    TYPE_INTEGER = 1,			/* 整数型 */
    TYPE_STRING = 2			/* 文字列型 */
};

/*
 * FieldInfo -- フィールドの情報を表現する構造体
 */
typedef struct FieldInfo FieldInfo;
struct FieldInfo {
    char name[MAX_FIELD_NAME];		/* フィールド名 */
    DataType dataType;			/* フィールドのデータ型 */
};

/*
 * TableInfo -- テーブルの情報を表現する構造体
 */
typedef struct TableInfo TableInfo;
struct TableInfo {
    int numField;				/* フィールド数 */
    FieldInfo fieldInfo[MAX_FIELD];		/* フィールド情報の配列 */
};

/*
 * file.cに定義されている関数群
 */
extern Result initializeFileModule();
extern Result finalizeFileModule();
extern Result createFile(char *);
extern Result deleteFile(char *);
extern File *openFile(char *);
extern Result closeFile(File *);
extern Result readPage(File *, int, char *);
extern Result writePage(File *, int, char *);
extern int getNumPages(char *);

/*
 * datadef.cに定義されている関数群
 */
extern Result initializeDataDefModule();
extern Result finalizeDataDefModule();
extern Result createTable(char *, TableInfo *);
extern ?????? dropTable(??????);
extern TableInfo *getTableInfo(char *);
extern void freeTableInfo(??????);
