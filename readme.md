# Orcus
**current version: 6.1**

Orcus is an interpreted stack-based programming language focused on being simple, reasonably fast, and putting responsibility in the hands of programmers.

Some features include:
* integers and floats as distinct types
* true OS-level multithreading
* runtime code compiling
* logical XOR
* strong dynamic typing
* fine variable scope control
* lambdas / anonymous functions
* automatic garbage collection
* multiple function return values
* mostly unrestricted goto

```mrc
print("hello, world!")
```

## how to build

### windows
1. Use visual studio to open /Orcus/Orcus.sln
2. run batch build. select the proper configuration for your platform, and ensure that 1 build of Orcus and 1 build of Orcus_Runtime is selected.
3. Build. The output will be in ./out/windows[configuration]

### unix-like systems
1. ensure your system has the g++ package installed
2. run compile.sh (or compile_debug.sh)
3. The output will be in ./out/linux[_debug/_release]


### not compiling with libraries
to not compile with a library, modify orcus.cpp so that it does not include the library's header. next, remove the library's files from the build. On windows, this is done by removing them from the solution. On unix-like systems, this is done by removing them from the file list in the compile script.


## how to contribute
* expanding documentation
* enhancing stability
* adding new library functions
* increasing runtime speed
* reporting bugs
