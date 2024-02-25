#!/bin/bash

# 使用方法: ./debuggable <テストケース番号> <名前付きパイプ名その1> <名前付きパイプ名その2> <エラーログパス> <アウトプットパス> [起動オプション]
# 引数説明:
#   第一引数: テストケース番号
#   第二引数: 名前付きパイプ名その1
#   第三引数: 名前付きパイプ名その2
#   第四引数: エラーログパス
#   第五引数: アウトプットパス
#   第六引数: 起動オプション（任意）

# 引数の数をチェック
if [ "$#" -lt 5 ]; then
    echo "使用方法: $0 <テストケース番号> <名前付きパイプ名その1> <名前付きパイプ名その2> <エラーログパス> <アウトプットパス> [起動オプション]"
    exit 1
fi

# 必要なディレクトリを作成
mkdir -p cmd/pipe
mkdir -p Output

# 古いログファイルがあれば削除
rm -f cmd/pipe/pipe.${1}.txt

echo Launching

# cargoを使用してtesterバイナリを実行
# 標準入力はtools/in/${1}.txtから読み込み、
# 標準エラーは${4}に追記し、標準出力は${5}に書き出す
cargo run --manifest-path tools/Cargo.toml --release --bin tester python3 cmd/src/cmd_pipe.py ${2} ${3} ${6} < tools/in/${1}.txt 2>> ${4} > ${5}

# cargo run --manifest-path tools/Cargo.toml --release --bin tester cmd/bin/cmd_pipe ${6} ${2} ${3} < tools/in/${1}.txt 2>> ${4} > ${5}

# 一時ファイルを削除
rm -f ${2}
rm -f ${3}
