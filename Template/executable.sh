#!/bin/sh

# Output ディレクトリを作成（すでに存在する場合はそのまま）
mkdir -p Output

# cargoを使ってtesterバイナリを実行し、
# 結果をswift runコマンドでExecutableをdebugモードで実行する
# 入力はtools/in/${1}.txtから受け取り、出力はOutput/${1}.Output.txtに保存
cargo run --manifest-path tools/Cargo.toml --release --bin tester swift run Executable -c debug \
 < tools/in/${1}.txt \
 > Output/${1}.Output.txt
