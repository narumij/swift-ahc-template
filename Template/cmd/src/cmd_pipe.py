import sys
import os
import fcntl
import threading
import argparse
from datetime import datetime

def set_non_blocking(fd):
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

def ensure_named_pipe_exists(pipe_path):
    if not os.path.exists(pipe_path):
        os.mkfifo(pipe_path)

def format_log_message(direction, message, is_developer_mode, pipe_path=""):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    if is_developer_mode:
        action = f"[{pipe_path}] {direction}"
    else:
        action = direction
    return f"{action} : {message}"

def tester_to_cmd(write_pipe_path, stop_event, quiet, is_developer_mode):
    try:
        ensure_named_pipe_exists(write_pipe_path)
        set_non_blocking(sys.stdin.fileno())

        with open(write_pipe_path, 'w') as pipe:
            while not stop_event.is_set():
                try:
                    line = sys.stdin.readline()
                    if line:
                        print(line, end='', file=pipe, flush=True)
                        if not quiet:
                            direction = "→"
                            log_message = format_log_message(direction, line.strip(), is_developer_mode, write_pipe_path)
                            print(log_message, file=sys.stderr)
                except BlockingIOError:
                    continue
    except BrokenPipeError:
        return
    finally:
        stop_event.set()

def cmd_to_tester(read_pipe_path, stop_event, quiet, is_developer_mode):
    ensure_named_pipe_exists(read_pipe_path)

    with open(read_pipe_path, 'r') as pipe:
        while not stop_event.is_set():
            line = pipe.readline()
            if line == '':
                if not quiet:
                    log_message = format_log_message("EOF", "No more data", is_developer_mode, read_pipe_path)
                    print(log_message, file=sys.stderr)
                break
            print(line, end='')
            sys.stdout.flush()
            if not quiet:
                direction = "←"
                log_message = format_log_message(direction, line.strip(), is_developer_mode, read_pipe_path)
                print(log_message, file=sys.stderr)
    stop_event.set()
    if not quiet:
        info_message = "Processing complete" if not is_developer_mode else "System processing complete"
        print(format_log_message("Info", info_message, is_developer_mode), file=sys.stderr)

def parse_arguments():
    parser = argparse.ArgumentParser(description="Inter-process communication using named pipes.")
    parser.add_argument('write_pipe_path', type=str, help='Path to the named pipe for writing.')
    parser.add_argument('read_pipe_path', type=str, help='Path to the named pipe for reading.')
    parser.add_argument('-q', '--quiet', action='store_true', help='Suppress error output.')
    parser.add_argument('-d', '--developer', action='store_true', help='Enable developer mode for detailed logs.')
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_arguments()

    stop_event = threading.Event()

    write_thread = threading.Thread(target=tester_to_cmd, args=(args.write_pipe_path, stop_event, args.quiet, args.developer))
    write_thread.start()

    cmd_to_tester(args.read_pipe_path, stop_event, args.quiet, args.developer)
        
    write_thread.join()
