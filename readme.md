# swift-repl is an implementation of Swift playgrounds on Windows

## Building
First, build Swift on Windows by following the instructions [here](https://github.com/apple/swift/blob/master/docs/WindowsBuild.md). Only steps 1-8 are necessary.
Next, install the latest [CMake](https://cmake.org/download/)
Finally, compile [SwiftWin32](https://github.com/compnerd/swift-win32)
Now go back to your swift-repl directory and do the following.
```
mkdir build
cd build
"c:\Program Files\CMake\bin\cmake.exe" .. -G Ninja ^
            -DLLVM_DIR=S:\b\llvm\lib\cmake\llvm ^
            -DSwift_DIR=S:\b\swift\lib\cmake\swift ^
            -DClang_DIR=S:\b\llvm\lib\cmake\clang ^
            -DSwiftWin32_DIR=S:\b\swift-win32 ^
            -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
            -DCMAKE_CXX_COMPILER=S:/b/llvm/bin/clang-cl.exe ^
            -DCMAKE_Swift_COMPILER=S:/b/swift/bin/swiftc.exe
ninja
```