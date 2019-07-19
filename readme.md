# swift-repl is an implementation of Swift playgrounds on Windows

## Building
First, build Swift on Windows by following the instructions [here](https://github.com/apple/swift/blob/master/docs/WindowsBuild.md). Only steps 1-8 are necessary.
Next, install the latest [CMake](https://cmake.org/download/)
Now go back to your swift-repl directory and do the following.
```
mkdir build
cd build
"c:\Program Files\CMake\bin\cmake.exe" .. -G Ninja ^
            -DLLVM_DIR=S:\b\llvm\lib\cmake\llvm ^
            -DSwift_DIR=S:\b\swift\lib\cmake\swift ^
            -DClang_DIR=S:\b\llvm\lib\cmake\clang ^
            -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
            -DCMAKE_CXX_COMPILER=S:/b/llvm/bin/clang-cl.exe
ninja
```