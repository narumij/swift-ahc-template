# swift-ahc-template

## 目的

SwiftでAHCをやる際の、デバッガ起動の追加サポートを行うこと。

### 前提

対象は、macOS 14以降、IDEはXcode、Swift PackageでAtCoderのAHC向けの開発をする作戦でも大丈夫な方です。

Xcodeには「Edit Scheme...」を選んだ場合に出るダイアログの左ペインでRunを選んだ右ペインに「Wait for the excecutable to be launched」というオプションがあり、これを選んでからAHCのtesterを起動するコマンドをたたくと、アタッチしようとしてくれます。

例えばこのような。
```
cargo run --manifest-path tools/Cargo.toml --bin tester swift run -c debug < tools/in/0000.txt > Log/0000.output.txt
```

そうすると、残念ながらAHC030ではこうなりました。
```
thread 'main' panicked at src/lib.rs:643:14:
called `Result::unwrap()` on an `Err` value: Os { code: 10, kind: Uncategorized, message: "No child processes" }
note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace
```

運良くスムーズに動く場合もあるでしょう。そういう場合、ここに書かれている内容は不要です。お帰りください。

### 解決策

Templateディレクトリのパッケージをお試しください。
現在はAHC030用になっており、toolsディレクトリを配布のテスターで差し替える必要があります。

スキームの修正も必要です。
ビルドターゲットDebuggableのRunのOptionsを開いてください。
OptionsのWorking Directoryにチェックを入れ、パッケージのルートフォルダを選択してください。

準備ができたら、ビルドターゲットRunを実行し、実行ログとスコアがコンソールペインに表示されていればOKです。