// swift-tools-version: 5.8.1
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "AtCoderHeuristic",
    platforms: [.macOS(.v13)],
    products: [
        // Products define the executables and libraries a package produces, making them visible to other packages.
        .library(
            name: "AtCoderHeuristic",
            targets: ["AtCoderHeuristic"]),
    ],
    targets: [
        // Targets are the basic building blocks of a package, defining a module or a test suite.
        // Targets can depend on other targets in this package and products from dependencies.
        .target(
            name: "AtCoderHeuristic"),
        .target(
            name: "AtCoderHeuristicHelper"),
        .testTarget(
            name: "AtCoderHeuristicTests",
            dependencies: ["AtCoderHeuristic"]),
        .executableTarget(
            name: "cmd",
            dependencies: [ "AtCoderHeuristic" ]
        )
    ]
)