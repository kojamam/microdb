
CC = gcc
CFLAGS = -g -O0

SOURCEDR = src
INCLUDEDR = include
BINDR = bin
TESTDR = bin/test

# # すべてのプログラムを作るルール
# all: $(BINDR)/microdb $(TESTDR)/all-test
#
# # すべてのテストプログラムを作るルール
# all-test: $(TESTDR)/test-file

# すべてのテストプログラムを実行するルール
do-test: $(TESTDR)/test-file
	$(TESTDR)/test-file

# 「microdb」を作成するためのルールは、今後追加される予定
microdb:

$(BINDR)/test-file: $(SOURCEDR)/test-file.o $(SOURCEDR)/file.o
	$(CC) -o $(TESTDR)/test-file $(CFLAGS) $(SOURCEDR)/test-file.o $(SOURCEDR)/file.o

$(SOURCEDR)/%.o: $(SOURCEDR)/%.c $(INCLUDEDR)/microdb.h
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	-rm -f $(SOURCEDR)/*.o
