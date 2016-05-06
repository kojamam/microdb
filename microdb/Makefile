
COMPILER = gcc
CFLAGS = -g

SOURCEDR = src
INCLUDEDR = include
BINDR = bin
TESTDR = bin

# # すべてのプログラムを作るルール
# all: $(BINDR)/microdb $(TESTDR)/all-test
#
# # すべてのテストプログラムを作るルール
# all-test: $(TESTDR)/test-file $(TESTDR)/test-datadef

# すべてのテストプログラムを実行するルール
do-test: $(TESTDR)/test-file $(TESTDR)/test-datadef
	$(TESTDR)/test-file
	$(TESTDR)/test-datadef

# 「microdb」を作成するためのルールは、今後追加される予定
# microdb:

$(BINDR)/test-file: $(SOURCEDR)/test-file.o $(SOURCEDR)/file.o
	$(COMPILER) -o $(TESTDR)/test-file $(CFLAGS) $(SOURCEDR)/test-file.o $(SOURCEDR)/file.o

$(BINDR)/test-datadef: $(SOURCEDR)/test-datadef.o $(SOURCEDR)/datadef.o $(SOURCEDR)/file.o
	$(COMPILER) -o $(TESTDR)/test-datadef $(CFLAGS) $(SOURCEDR)/test-datadef.o $(SOURCEDR)/datadef.o $(SOURCEDR)/file.o

$(SOURCEDR)/%.o: $(SOURCEDR)/%.c $(INCLUDEDR)/microdb.h
	$(COMPILER) -o $@ $(CFLAGS) -c $<

clean:
	-rm -f $(SOURCEDR)/*.o
