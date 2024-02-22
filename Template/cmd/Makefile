CC=gcc
CFLAGS=-Wall -g
SRC_DIR=src
BIN_DIR=bin
BUILD_DIR=.build

# 実行ファイルのビルドルール
all: $(BIN_DIR)/cmd_spy $(BIN_DIR)/cmd_pipe

# cmd_spyのビルドルール
$(BIN_DIR)/cmd_spy: $(BUILD_DIR)/cmd_spy.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(CFLAGS)

# cmd_pipeのビルドルール
$(BIN_DIR)/cmd_pipe: $(BUILD_DIR)/cmd_pipe.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(CFLAGS)

# オブジェクトファイルのコンパイルルール
$(BUILD_DIR)/cmd_spy.o: $(SRC_DIR)/cmd_spy.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/cmd_pipe.o: $(SRC_DIR)/cmd_pipe.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

# クリーンルール
clean:
	rm -f $(BUILD_DIR)/*.o $(BIN_DIR)/cmd_spy $(BIN_DIR)/cmd_pipe

