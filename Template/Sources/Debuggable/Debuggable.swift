import Foundation

#if os(Linux)
import Glibc
#else
import Darwin
#endif

enum CommandOption: CustomStringConvertible {
    var description: String {
        switch self {
        case .normal: return ""
        case .output: return ""
        case .quiet: return "-q"
        case .develop: return "-d"
        }
    }
    case output
    case normal
    case quiet
    case develop
}

class Debuggable {
    
    internal init(
        testcase: String,
        pipePath1: String = "./cmd/pipe/tester_to_cmd",
        pipePath2: String = "./cmd/pipe/cmd_to_tester"
    )
    {
        self.testcase = testcase
        self.pipePath1 = pipePath1
        self.pipePath2 = pipePath2
        self.connector = PipeConnector(inputPipePath: pipePath1, outputPipePath: pipePath2)
    }
    
    let testcase: String
    
    let pipePath1: String
    let pipePath2: String

    var connector: PipeConnector
    
    var runnerOutputPath: String { "cmd/pipe/pipe.\(testcase).txt" }
    var testerOutputPath: String { "Output/\(testcase).output.txt" }
    
    func command(commandOption: CommandOption) -> String { "sh debuggable.sh \(testcase) \(pipePath1) \(pipePath2) \(runnerOutputPath) \(testerOutputPath) \(commandOption)" }
    
    func connect() {
        connector.connectInputPipe()
        connector.connectOutputPipe()
    }
    
    func disconnect() {
        connector.restoreStdinStdout()
    }

    func run(commandOption: CommandOption = .normal,_ body: () -> Void) {

        let currentDirectoryPath = FileManager.default.currentDirectoryPath

        let command = command(commandOption: commandOption)
        
        print("[Swift] Working Directory: \(currentDirectoryPath)")
        print("[Building] ....")
        print("[Build]", shell("sh build.sh").output)
        print("[Swift] Launch `\(command)`")

        var result: (task: Process, output: String)?

        OperationQueue().addOperation {
            result = shell(command)
            print("[Swift] Result = \(result!.output)")
        }

        var st: stat = stat()
        while stat(pipePath1, &st) != 0 { Thread.sleep(forTimeInterval: 0.1) }
        while stat(pipePath2, &st) != 0 { Thread.sleep(forTimeInterval: 0.1) }

        connect()

        body()

        disconnect()

        Thread.sleep(forTimeInterval: 0.1)

        if commandOption != .output {
            print(shell("cat \(runnerOutputPath)").output)
        } else {
            print(shell("cat \(testerOutputPath)").output)
        }

        if let result, result.task.terminationStatus != 0 {
            print("[Swift] Status = \(result.task.terminationStatus)")
            print("[Swift] Final Result = \(result.output)")
        }
    }
}

struct PipeConnector {
    
    let inputPipePath: String
    let outputPipePath: String
    var originalStdinFd: Int32 = 0
    var originalStdoutFd: Int32 = 0
    
    init(inputPipePath: String, outputPipePath: String) {
        self.inputPipePath = inputPipePath
        self.outputPipePath = outputPipePath
    }

    func createNamedPipeIfNeeded(at path: String, mode: mode_t) {
        var st: stat = stat()
        let result = stat(path, &st)
        if result == 0 {
            if (st.st_mode & S_IFMT) != S_IFIFO {
                NSLog("[Swift] \(path) is not a FIFO\n")
            }
        } else if errno == ENOENT {
            if mkfifo(path, mode) != 0 {
                NSLog("[Swift] mkfifo")
                NSLog("[Swift] Created named pipe: \(path)")
            }
        } else {
            NSLog("[Swift] stat")
        }
    }

    mutating func connectInputPipe() {
        createNamedPipeIfNeeded(at: inputPipePath, mode: 0666)
        
        originalStdinFd = dup(STDIN_FILENO)
        let inputPipeFd = open(inputPipePath, O_RDONLY)
        guard inputPipeFd != -1 else {
            NSLog("[Swift] Failed to open input named pipe")
            exit(EXIT_FAILURE)
        }

        if dup2(inputPipeFd, STDIN_FILENO) == -1 {
            NSLog("[Swift] Failed to duplicate file descriptor to stdin")
            exit(EXIT_FAILURE)
        }

        close(inputPipeFd)
    }

    mutating func connectOutputPipe() {
        createNamedPipeIfNeeded(at: inputPipePath, mode: 0666)

        originalStdoutFd = dup(STDOUT_FILENO)
        let outputPipeFd = open(outputPipePath, O_WRONLY)
        guard outputPipeFd != -1 else {
            NSLog("[Swift] Failed to open output named pipe")
            exit(EXIT_FAILURE)
        }

        if dup2(outputPipeFd, STDOUT_FILENO) == -1 {
            NSLog("[Swift] Failed to duplicate file descriptor to stdout")
            exit(EXIT_FAILURE)
        }

        close(outputPipeFd)
    }
    
    func restoreStdinStdout() {
        if dup2(originalStdinFd, STDIN_FILENO) == -1 {
            NSLog("[Swift] Failed to restore stdin")
        }
        
        if dup2(originalStdoutFd, STDOUT_FILENO) == -1 {
            NSLog("[Swift] Failed to restore stdout")
        }
        
        close(originalStdinFd)
        close(originalStdoutFd)
    }
}

func shell(_ command: String) -> (task: Process, output: String) {
    let task = Process()
    let pipe = Pipe()
    
    task.standardOutput = pipe
    task.standardError = pipe
    task.arguments = ["-c", command]
    task.launchPath = "/bin/zsh"
    task.standardInput = nil
    task.launch()
    
    let data = pipe.fileHandleForReading.readDataToEndOfFile()
    let output = String(data: data, encoding: .utf8)!
    
    return (task, output)
}
