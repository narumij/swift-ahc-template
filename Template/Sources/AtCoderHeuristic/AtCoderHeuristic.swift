import Foundation
// このカタチにすることで、このファイル自体を提出可能に保ちつつ、
// AtCoderHeuristicMain()を、ライブラリ的に参照することもできるようになる。

fileprivate let main: () = {
    do { 
        try AtCoderHeuristicMain()
    }
    catch {
    }
}()

public func AtCoderHeuristicMain() throws {
    print("# Hello, AtCoder Heuristic!")
    // 解法を書く部分。
    
    // 以下はAHC030のサンプル相当版です。
    // 利用の際は、toolsフォルダをAHC030のtesterに差し替える必要があります。
    let components = readLine()!.components(separatedBy: " ")
    let (N,M,_) = (Int(components[0])!, Int(components[1])!, Double(components[2])!)
    for _ in 0..<M { _ = readLine() }
    var pos: [(Int,Int)] = []
    for i in 0..<N {
        for j in 0..<N {
            print("q", 1, i, j)
            fflush(stdout)
            let str = readLine()
            if Int(str!)! == 1 {
                pos.append((i,j))
            }
        }
    }
    print("a", pos.count, pos.flatMap{ [$0,$1] }.map(\.description).joined(separator: " "))
    _ = readLine()
}

