import sys
import os
import fcntl
import threading

def set_non_blocking(fd):
    """
    ファイルディスクリプタをノンブロッキングモードに設定する
    """
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

def tester_to_cmd(write_pipe_path, stop_event):
    try:
        # 標準入力をノンブロッキングモードに設定
        set_non_blocking(sys.stdin.fileno())

        with open(write_pipe_path, 'w') as pipe:
            while not stop_event.is_set():
                try:
                    line = sys.stdin.readline()
                    if line:
                        # 名前付きパイプに書き込む
                        print(line, end='', file=pipe, flush=True)
                        # 標準エラーに出力
                        print(f"Written to {write_pipe_path} (tester_to_cmd): {line.strip()}", file=sys.stderr)
                except BlockingIOError:
                    # データが利用可能でない場合はここに入るが、待機せずにループを続行
                    continue
    finally:
        stop_event.set()  # 終了フラグをセット

# 注意: このコードは Unix 系オペレーティングシステムでのみ動作します。
# Windowsでは、fcntl モジュールは使用できません。


def cmd_to_tester(read_pipe_path, stop_event):
    if not os.path.exists(read_pipe_path):
        print(f"Error: The named pipe at {read_pipe_path} does not exist.", file=sys.stderr)
        sys.exit(1)

    with open(read_pipe_path, 'r') as pipe:
        while not stop_event.is_set():
            line = pipe.readline()
            if line == '':  # EOFが検出されたら終了
                print(f"Read from {read_pipe_path} (cmd_to_tester): EOF", file=sys.stderr)  # 標準エラーに出力
                break
            print(line, end='')  # 標準出力に出力
            sys.stdout.flush()
            print(f"Read from {read_pipe_path} (cmd_to_tester): {line.strip()}", file=sys.stderr)  # 標準エラーに出力
    stop_event.set()  # 読み取りが終了したら終了フラグをセット
    print(f"cmd_to_tester ended.", file=sys.stderr)  # 標準エラーに出力

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py /path/to/write_named_pipe /path/to/read_named_pipe", file=sys.stderr)
        sys.exit(1)

    write_pipe_path = sys.argv[1]
    read_pipe_path = sys.argv[2]
    stop_event = threading.Event()

    # tester_to_cmdをスレッドで実行
    write_thread = threading.Thread(target=tester_to_cmd, args=(write_pipe_path, stop_event))
    write_thread.start()
    #read_thread = threading.Thread(target=cmd_to_tester, args=(read_pipe_path, stop_event))
    #read_thread.start()
    
    # cmd_to_testerをメインスレッドで実行
    cmd_to_tester(read_pipe_path, stop_event)
    # tester_to_cmd(write_pipe_path, stop_event)
    print(f"cmd_to_tester ended.", file=sys.stderr)  # 標準エラーに出力
    #print(f"tester_to_cmd ended.", file=sys.stderr)  # 標準エラーに出力

    # 書き込みスレッドの終了を待つ
    write_thread.join()
    #read_thread.join()
