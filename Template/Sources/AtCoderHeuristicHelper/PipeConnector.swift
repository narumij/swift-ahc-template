import Foundation

#if os(Linux)
import Glibc
#else
import Darwin
#endif

struct PipeConnector {
    
    let inputPipePath: String
    let outputPipePath: String
    var originalStdinFd: Int32 = 0
    var originalStdoutFd: Int32 = 0
    
    init(inputPipePath: String, outputPipePath: String) {
        self.inputPipePath = inputPipePath
        self.outputPipePath = outputPipePath
    }

    mutating func connectInputPipe() {
        originalStdinFd = dup(STDIN_FILENO)
        let inputPipeFd = open(inputPipePath, O_RDONLY)
        guard inputPipeFd != -1 else {
            perror("Failed to open input named pipe")
            exit(EXIT_FAILURE)
        }

        if dup2(inputPipeFd, STDIN_FILENO) == -1 {
            perror("Failed to duplicate file descriptor to stdin")
            exit(EXIT_FAILURE)
        }

        close(inputPipeFd)
    }

    mutating func connectOutputPipe() {
        originalStdoutFd = dup(STDOUT_FILENO)
        let outputPipeFd = open(outputPipePath, O_WRONLY)
        guard outputPipeFd != -1 else {
            perror("Failed to open output named pipe")
            exit(EXIT_FAILURE)
        }

        if dup2(outputPipeFd, STDOUT_FILENO) == -1 {
            perror("Failed to duplicate file descriptor to stdout")
            exit(EXIT_FAILURE)
        }

        close(outputPipeFd)
    }
    
    func restoreStdinStdout() {
        if dup2(originalStdinFd, STDIN_FILENO) == -1 {
            perror("Failed to restore stdin")
        }
        
        if dup2(originalStdoutFd, STDOUT_FILENO) == -1 {
            perror("Failed to restore stdout")
        }
        
        close(originalStdinFd)
        close(originalStdoutFd)
    }
}
