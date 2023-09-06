# Cobalt
Cobalt is a Lowlevel fork of Lua 5.4 which includes:
- SDL bindings
- interpreter, compiler, and JIT FFI
- compile to LLVM IR if you have LLVM capable version of cobalt
- more libraries/bindings
- improved syntax
- type system (typechecker in preprocessor)
- preprocessor
- allocation tracker (track bytes and pool allocator stats)
- `uwait`, `swait`, `mwait` for system sleep
- new std functions
- `unix`, `win`, and `core` (core is cross plat) for lowlevel system calls
- AOT compiler byte->C or byte->LLVMIR if you have LLVM capable version of cobalt
- libclang bindings (compile C code from Cobalt)
- cURL capabilites
- regex support (not full regex only a small subset), use `unix.regex` for POSIX regex
- 2x speed improved gc (optional) using libgc
- improved vm
- Easy interface to get device information `device`, `device.specs().CPU`, etc
- LLVM JIT compiler
- `minicobalt` interpreter without alot of std functions and runs on bare bones
- bytecode optimizer
- C++ bridge
- 30% increase with pool allocator
- lpeg-labels built in
- `$`, `@`, `&` symbols for pairs, ipairs, table.unpack
- more operators
- tenary operator
- CMake build system