# Compiler Internals — LLVM, Clang, Linking & CUDA

Deep dive for NVIDIA compiler team interview. Covers the full compilation pipeline, LLVM IR, AST, linking, and NVIDIA's CUDA/NVPTX toolchain. Connects to your Clang LibTooling AST tracer experience at Cisco.

---

## Table of Contents

1. [Compilation Pipeline](#1-compilation-pipeline)
2. [LLVM Ecosystem](#2-llvm-ecosystem)
3. [Abstract Syntax Trees (Clang AST)](#3-abstract-syntax-trees-clang-ast)
4. [Linking](#4-linking)
5. [NVIDIA: CUDA, NVCC, PTX](#5-nvidia-cuda-nvcc-ptx)
6. [Build System Integration](#6-build-system-integration)
7. [Hands-On Exercises](#7-hands-on-exercises)
8. [Interview Questions](#8-interview-questions)

---

## 1. Compilation Pipeline

### End-to-End Flow

```
Source (.c / .cpp)
       │
       ▼
┌──────────────┐
│ PREPROCESSOR │  #include, #define, #if expansion
│              │  Output: .i (preprocessed translation unit)
└──────┬───────┘
       ▼
┌──────────────┐
│    LEXER     │  Character stream → tokens (keywords, identifiers, literals)
│ (tokenizer)  │  Maximal munch rule, source locations tracked
└──────┬───────┘
       ▼
┌──────────────┐
│   PARSER     │  Tokens → AST (Abstract Syntax Tree)
│              │  Checks grammar, builds tree structure
└──────┬───────┘
       ▼
┌──────────────┐
│  SEMANTIC    │  Type checking, overload resolution, template instantiation
│  ANALYSIS    │  Symbol tables, scope resolution, implicit conversions
└──────┬───────┘
       ▼
┌──────────────┐
│  IR GEN      │  AST → LLVM IR (SSA form)
│              │  Platform-independent intermediate representation
└──────┬───────┘
       ▼
┌──────────────┐
│ OPTIMIZATION │  Analysis passes (alias, dominators, loops)
│   PASSES     │  Transform passes (inline, DCE, vectorize, unroll)
└──────┬───────┘
       ▼
┌──────────────┐
│   BACKEND    │  Instruction selection, register allocation
│  (CodeGen)   │  Scheduling, calling conventions, debug info
└──────┬───────┘
       ▼
┌──────────────┐
│   LINKER     │  Symbol resolution, relocations
│              │  Static/dynamic libraries → executable
└──────────────┘
```

### 1.1 Preprocessor

Runs before the "real" compiler:
- Pastes `#include` headers
- Expands macros
- Evaluates `#if` / `#ifdef` conditional compilation

```bash
clang -E file.c              # show preprocessed output
clang -save-temps=obj ...    # keep intermediates (.i, .bc, .s)
```

**Build/DevOps angle:** ccache keys include defines and include paths. A macro change must not reuse stale cached objects.

### 1.2 Lexical Analysis (Tokenizer)

Converts character stream into tokens with source locations.

```c
int main(void) { return 0; }
```

| Lexeme | Token Kind |
|--------|-----------|
| `int` | keyword |
| `main` | identifier |
| `(` | punctuation |
| `return` | keyword |
| `0` | literal |

- Implemented with finite automata (regex → NFA → DFA)
- **Maximal munch:** prefer longest valid token (`>>` vs two `>`)
- Clang tracks spelling vs expansion locations for macros

### 1.3 Syntax Analysis (Parser → AST)

Proves tokens satisfy a context-free grammar. Builds an **Abstract Syntax Tree**.

- **Parse tree** mirrors grammar productions (verbose)
- **AST** drops punctuation: nodes = constructs (`IfStmt`, `CallExpr`)
- Clang uses **recursive descent** for C++ (good error messages + recovery)

**C++ pitfall:** "Most vexing parse" — `T a();` parsed as function declaration, not variable initialization.

### 1.4 Semantic Analysis

Attaches meaning: types, scopes, overload resolution, template instantiation.

```cpp
void f(int);
void g() { f("x"); }  // error: no viable conversion from 'const char*' to 'int'
```

- **Symbol tables:** Nested scopes, linkage, visibility
- **Overload resolution:** Viable functions ranked; ambiguity = error
- **Templates:** Instantiation; interacts with ODR and linking

### 1.5 IR Generation and Optimization

**LLVM IR** is the bridge between frontend and backend. It's in **SSA form** (Static Single Assignment).

**Analysis passes** compute facts: alias sets, dominator trees, loop nests, call graphs.

**Transform passes** rewrite IR: inlining, dead code elimination, vectorization.

```bash
opt -passes='mem2reg,instcombine,dce' in.ll -S -o out.ll
```

Key: `mem2reg` promotes stack allocas to SSA registers — standard early pipeline step. Pass ordering matters because transforms invalidate analyses.

### 1.6 Backend (Code Generation)

IR → target instructions through:
1. **Instruction selection** — map IR ops to machine instructions
2. **Scheduling** — order for pipeline efficiency
3. **Register allocation** — map virtual to physical registers
4. **Emission** — object code or assembly + debug info

### 1.7 End-to-End Example

**Source (`add.c`):**

```c
int add(int a, int b) {
    return a + b;
}
```

**LLVM IR:**

```llvm
define i32 @add(i32 %a, i32 %b) {
entry:
    %t = add nsw i32 %a, %b
    ret i32 %t
}
```

**x86-64 Assembly:**

```asm
add:
    leal    (%rdi,%rsi), %eax
    ret
```

**Artifact chain:**

```
.c ──(clang -E)──▶ .i ──(clang -emit-llvm -S)──▶ .ll ──(llc)──▶ .s ──(as)──▶ .o ──(ld)──▶ a.out
```

---

## 2. LLVM Ecosystem

### 2.1 What LLVM Is

LLVM is a **modular compiler infrastructure**: optimizers + code generators + tooling libraries. It is NOT a single compiler — **Clang** is the C-family frontend that emits LLVM IR.

**Why NVIDIA cares:** NVPTX backend, Clang CUDA, LTO/ThinLTO, lld, and the APIs that power clang-tidy/static analysis.

### 2.2 Clang Driver vs `-cc1`

`clang` is the **driver** (argument parsing, toolchain selection). The actual compilation is a `-cc1` subprocess.

```bash
clang -### -c foo.c    # show actual commands without running them
clang -v -c foo.c      # verbose: shows -cc1 line and search paths
```

### 2.3 LLVM IR Deep Dive

**SSA (Static Single Assignment):** Each variable is assigned exactly once. **Phi nodes** select between incoming values at control flow merge points.

**Basic block:** Straight-line code ending in a **terminator** (`br`, `switch`, `ret`, `invoke`, `unreachable`).

**Example — `max` function:**

```c
int max(int x, int y) {
    return (x > y) ? x : y;
}
```

```llvm
define i32 @max(i32 %x, i32 %y) {
entry:
    %cmp = icmp sgt i32 %x, %y
    br i1 %cmp, label %then, label %else
then:
    br label %merge
else:
    br label %merge
merge:
    %result = phi i32 [ %x, %then ], [ %y, %else ]
    ret i32 %result
}
```

**`getelementptr` (GEP):** Computes a pointer offset from a base + indices. Does **NOT** access memory — just address arithmetic. Confusing GEP with `load` is a classic LLVM interview filter question.

### 2.4 Viewing IR

```bash
clang -emit-llvm -S -O0 ex.c -o ex-O0.ll   # unoptimized IR (human-readable)
clang -emit-llvm -S -O2 ex.c -o ex-O2.ll   # optimized IR
diff ex-O0.ll ex-O2.ll                       # see what optimizer did

clang -emit-llvm -c ex.c -o ex.bc           # bitcode (binary IR)
llvm-dis ex.bc -o ex.ll                      # disassemble to text
```

### 2.5 Optimization Levels

| Flag | Behavior |
|------|----------|
| `-O0` | Minimal optimization; fast compile; best source correlation for debugging |
| `-O1` | Some speed/size wins |
| `-O2` | Standard "release" for most projects |
| `-O3` | Aggressive inlining, vectorization, unrolling |
| `-Os` | Optimize for binary size |
| `-Oz` | Aggressively optimize for size |

Exact pass pipelines change every LLVM release.

### 2.6 Common Optimizations

| Optimization | What it does |
|--------------|-------------|
| **DCE (Dead Code Elimination)** | Remove computations with no observable effect |
| **Constant Folding** | Evaluate `2+3` at compile time |
| **CSE / GVN** | Reuse already-computed values |
| **Inlining** | Replace call sites with callee body |
| **Loop Unrolling** | Duplicate loop body; fewer branches, more ILP |
| **LICM** | Hoist loop-invariant computations out of loops |
| **Vectorization** | Use SIMD (AVX, NEON) for data-parallel loops |
| **Strength Reduction** | `* 8` → shift left 3 |

### 2.7 TBAA (Type-Based Alias Analysis)

LLVM IR can carry `!tbaa` metadata on loads/stores. Tells the optimizer "these accesses have different types and cannot alias."

If you lie via `reinterpret_cast` in C++, you can get **miscompiles** under strict aliasing — a classic systems interview topic.

```bash
clang -fno-strict-aliasing ...   # disable TBAA if needed
```

### 2.8 Backends

| Backend | Target |
|---------|--------|
| **x86-64** | Desktop/server |
| **AArch64** | ARM servers, mobile |
| **NVPTX** | NVIDIA GPUs (emits PTX) |
| **AMDGPU** | AMD GPUs |

---

## 3. Abstract Syntax Trees (Clang AST)

### 3.1 What is an AST?

A tree of language constructs — functions, statements, expressions, types — used for semantic analysis, codegen, and source-level tools.

```
TranslationUnitDecl
  └── FunctionDecl: main
       └── CompoundStmt
            └── ReturnStmt
                 └── IntegerLiteral: 0
```

### 3.2 Clang AST Node Families

| Family | Examples |
|--------|---------|
| **Decl** | `VarDecl`, `FunctionDecl`, `CXXRecordDecl`, `ParmVarDecl` |
| **Stmt** | `CompoundStmt`, `IfStmt`, `WhileStmt`, `ReturnStmt` |
| **Expr** | `BinaryOperator`, `CallExpr`, `CXXMemberCallExpr` |
| **Type** | `BuiltinType`, `PointerType`, `TypedefType` |

Important: In Clang's AST, `Expr` inherits from `Stmt` — expressions are statements.

### 3.3 Dumping the AST

```bash
clang -Xclang -ast-dump -fsyntax-only example.cpp
clang -Xclang -ast-dump-filter=myfn -fsyntax-only example.cpp   # filter to one function
```

**Reading dumps:** Indentation shows hierarchy. Hex addresses identify nodes.

```
`-VarDecl col:5 used x 'int' cinit
  `-BinaryOperator 'int' '+'
    |-IntegerLiteral 'int' 1
    `-IntegerLiteral 'int' 2
```

This shows: `int x = 1 + 2;`

### 3.4 LibTooling (Your AST Tracer Context)

LibTooling runs Clang on real translation units with the same flags as builds (via `compile_commands.json`). You get:

- Full `ASTContext` and `Decl` graph
- Diagnostics engine
- SourceManager for locations

This is how **clang-tidy checks**, static analyzers, and your AST tracer work.

**Talking about your tracer in interviews:** Describe inputs (`compile_commands.json`), what you matched (e.g., all `CallExpr` to security-sensitive APIs), what you emitted (JSON/SARIF), and how you validated (golden tests). Tie to determinism (same flags as build) and performance (filter TUs, skip system headers).

### 3.5 AST Matchers

Declarative patterns with callbacks:

```
callExpr(callee(functionDecl(hasName("malloc"))))
```

**clang-query REPL:**

```bash
clang-query file.cpp
# set output dump
# match callExpr(callee(functionDecl(hasName("malloc"))))
```

### 3.6 RecursiveASTVisitor

Walk the entire AST tree, override `Visit*` methods:

```cpp
class MyVisitor : public clang::RecursiveASTVisitor<MyVisitor> {
public:
    bool VisitCallExpr(clang::CallExpr *E) {
        // Inspect callee, args, source ranges
        return true;  // continue traversal
    }
};
```

**Matchers vs Visitor:** Matchers = query specific subgraph patterns. Visitor = walk everything (unless you prune).

### 3.7 Skeleton Clang Tool

```cpp
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"

class MyVisitor : public clang::RecursiveASTVisitor<MyVisitor> {
public:
    bool VisitFunctionDecl(clang::FunctionDecl *FD) {
        if (FD->hasBody()) { /* record signature, location */ }
        return true;
    }
};

class MyConsumer : public clang::ASTConsumer {
    MyVisitor V;
public:
    void HandleTranslationUnit(clang::ASTContext &Ctx) override {
        V.TraverseDecl(Ctx.getTranslationUnitDecl());
    }
};
```

Full tools add `ClangTool`, `CommonOptionsParser`, and CMake `find_package(Clang)`.

---

## 4. Linking

### 4.1 Static vs Dynamic Linking

| | Static (`.a`) | Dynamic (`.so` / `.dylib` / `.dll`) |
|---|--------------|-------------------------------------|
| **Link time** | Code copied into output | References recorded; loaded at runtime |
| **Deploy** | Fewer moving parts; larger binary | Must ship compatible `.so` or use rpath |
| **Patching** | Rebuild/relink consumers | Replace shared lib (watch ABI!) |

### 4.2 Symbol Visibility

```bash
# Reduce exported symbols → faster linking, smaller binary
clang++ -fvisibility=hidden -c lib.cpp
```

Only explicitly exported symbols appear in the dynamic symbol table.

### 4.3 PLT and GOT (Dynamic Linking on ELF)

- **GOT (Global Offset Table):** Contains addresses of external symbols
- **PLT (Procedure Linkage Table):** Trampolines for lazy function resolution

First call goes through PLT → resolver → patches GOT. Subsequent calls go directly through patched GOT.

**Hardening:** `BIND_NOW` (resolve all at load time), `RELRO` (read-only GOT), `PIE` (position-independent executable).

### 4.4 C++ Name Mangling

Encodes namespace, class, parameters, const/volatile in the symbol name.

```bash
clang++ -c t.cpp && llvm-nm t.o | c++filt   # demangle symbols
```

`extern "C"` gives C linkage — predictable names for `dlsym`/FFI.

### 4.5 Linker Scripts

GNU `ld` scripts control section placement, set ENTRY point, assert memory layouts. Common in firmware and kernel linking.

### 4.6 LTO (Link-Time Optimization)

**Full LTO:** All IR merged into one module at link time. Maximum optimization but very memory-heavy.

**ThinLTO:** Per-module IR + summaries. Parallel backend codegen. Much faster, incrementally friendly.

```bash
clang -flto=thin -O2 a.c b.c -o prog
```

Requires compatible linker (`lld`, or `gold` with plugin). Don't mix compiler versions across TUs.

### 4.7 Linkers Comparison

| Linker | Notes |
|--------|-------|
| **BFD ld** | Traditional GNU; slower on large C++ |
| **gold** | Faster ELF; GCC LTO plugin support |
| **lld** | LLVM's linker; great with ThinLTO |
| **mold** | Very fast ELF linker (`-fuse-ld=mold`) |

---

## 5. NVIDIA: CUDA, NVCC, PTX

### 5.1 NVCC as a Compiler Driver

`nvcc` is NOT a compiler — it's a driver that splits host and device compilation:

```
┌──────────┐
│  .cu     │
│  source  │
└────┬─────┘
     │
     ▼
┌──────────┐
│   nvcc   │  (driver: splits host and device code)
└──┬────┬──┘
   │    │
   ▼    ▼
┌──────┐  ┌────────────────────┐
│ Host │  │   Device pipeline  │
│ gcc/ │  │ cicc → ptxas →     │
│clang │  │ fatbinary          │
└──┬───┘  └──────┬─────────────┘
   │             │
   ▼             ▼
┌──────┐    ┌─────────┐
│host.o│    │ fatbin  │
└──┬───┘    └────┬────┘
   │             │
   ▼             ▼
┌─────────────────────┐
│      Linker         │
│    → executable     │
└─────────────────────┘
```

See the subtools with `nvcc -v` or `nvcc --keep` (keeps intermediates).

### 5.2 PTX and SASS

- **PTX:** Textual, versioned virtual ISA. JIT'd by the driver to SASS for the actual GPU.
- **SASS:** Architecture-specific machine code (e.g., sm_80 for A100).

```bash
nvcc -ptx kernel.cu -o kernel.ptx   # generate PTX only
```

Shipping PTX improves forward compatibility on newer GPUs. Shipping SASS is faster to load (no JIT).

### 5.3 Fatbin

A **fatbin** bundles multiple code images (e.g., sm_80 SASS + PTX fallback). The runtime selects the best match at load time.

```bash
nvcc -gencode arch=compute_80,code=sm_80 \
     -gencode arch=compute_90,code=sm_90 \
     -gencode arch=compute_90,code=compute_90 \   # PTX fallback
     kernel.cu -o kernel
```

### 5.4 nvlink

Links relocatable device code from **separate compilation** (`-rdc=true`). Needed when device code spans multiple translation units.

### 5.5 LLVM NVPTX Backend and Clang CUDA

LLVM's **NVPTX** backend lowers LLVM IR to PTX. **Clang** can compile CUDA directly:

```bash
clang++ -x cuda --cuda-path=/usr/local/cuda \
        --cuda-gpu-arch=sm_80 kernel.cu -o kernel
```

NVIDIA's production stack combines proprietary and LLVM-derived pieces. In interviews, acknowledge version alignment matters.

### 5.6 Build/CI Notes for CUDA

- Pin CUDA toolkit version, driver minimum, and `-gencode` flags
- Fatbin size and JIT time matter for deployment
- `nvcc --keep` is invaluable for debugging compilation stages
- Address spaces in NVPTX IR: global, shared, local, constant — correctness requires matching loads/stores

---

## 6. Build System Integration

### 6.1 `compile_commands.json`

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
```

Consumed by clangd, clang-tidy, and LibTooling tools. Mirrors exact CI/local build flags — critical for your AST tracer to produce correct results.

### 6.2 CMake Toolchain Files

`CMAKE_TOOLCHAIN_FILE` encodes cross compilers, sysroots, CUDA toolchains. Keeps CI builds hermetic.

### 6.3 ccache / sccache

Hash preprocessor output + flags + compiler identity. `sccache` supports remote shared cache (S3, GCS).

**Pitfalls:** `__DATE__`/`__TIME__` break caching. Environment differences can cause false cache hits if sloppiness settings are too loose.

### 6.4 Distributed Compilation

- **distcc / icecream:** Distribute compilation after preprocessing
- Often paired with ccache
- LTO generally stays local (requires all IR at link time)

---

## 7. Hands-On Exercises

### Exercise 1: Compare LLVM IR at Different Optimization Levels

```bash
clang -emit-llvm -S -O0 loop.c -o loop-O0.ll
clang -emit-llvm -S -O3 loop.c -o loop-O3.ll
diff loop-O0.ll loop-O3.ll
```

Look for vectorized types, unrolled bodies, eliminated dead code. Try `-Rpass-missed=loop-vectorize` to see what the optimizer couldn't do and why.

### Exercise 2: AST Dump and clang-query

Dump a `.cpp` with templates. Use `clang-query` to match `callExpr(callee(functionDecl(hasName("foo"))))`.

### Exercise 3: Static vs Shared Library

Create `libfoo` as both `.a` and `.so`. Compare with:

```bash
nm -C libfoo.a
readelf -W --dyn-syms libfoo.so
```

### Exercise 4: ThinLTO

Build a two-file project with `-flto=thin` and `lld`. Break it on purpose by mixing compiler versions. Observe the error.

### Exercise 5: CUDA Intermediates

```bash
nvcc --keep kernel.cu -o kernel
```

Read the `.ptx` file. Find the `.entry` directive and correlate with your kernel function.

---

## 8. Interview Questions

**Q1: Explain the phases from .c to executable.**
Preprocess → tokenize → parse → semantic analysis → IR gen → optimize → codegen → assemble → link.

**Q2: What is SSA and why phi nodes?**
Single assignment per value simplifies dataflow analysis. Phi selects which incoming definition is live at a control flow merge.

**Q3: What does GEP do?**
Pointer arithmetic from a base + typed indices. Does NOT access memory. Common gotcha in LLVM interviews.

**Q4: Analysis vs transform pass?**
Analysis computes facts (alias, dominators). Transform mutates IR and may invalidate cached analyses.

**Q5: ThinLTO vs full LTO?**
ThinLTO uses summaries + distributed codegen — much faster and less memory. Full LTO merges everything into one module for maximum optimization.

**Q6: Why does `compile_commands.json` matter?**
Reproduces exact include paths, defines, and standards so tools (clangd, clang-tidy, AST tools) see the same code the build sees.

**Q7: Static vs dynamic linking tradeoffs?**
Static: simpler deploy, larger binaries. Dynamic: shared memory, upgrade path, but ABI compatibility complexity.

**Q8: What is name mangling?**
C++ encodes overload info in linker symbol names. `extern "C"` disables it for C ABI compatibility.

**Q9: What is a basic block terminator?**
Last instruction of a basic block: `br`, `switch`, `ret`, `invoke`, `unreachable`.

**Q10: What is PTX?**
NVIDIA's portable virtual GPU ISA. JIT'd to architecture-specific SASS by the driver at load time.

**Q11: What does NVCC orchestrate?**
Splits host/device compilation. Host goes to gcc/clang. Device goes through cicc → ptxas → fatbinary. Links everything together.

**Q12: Matchers vs RecursiveASTVisitor?**
Matchers = declarative pattern queries with callbacks. Visitor = walk entire tree with override hooks.

**Q13: What breaks LTO in CI?**
Mixed compiler versions, missing LTO plugin, wrong linker, inconsistent `-flto` flags across objects.

**Q14: What is TBAA?**
Type-based alias analysis. LLVM metadata that tells optimizer two accesses have different types and can't alias. Broken by unsafe type punning.

**Q15: Why does aliasing matter for optimization?**
If stores might alias loads, the optimizer can't reorder or eliminate them safely. Less aliasing = more optimization.

**Q16: What is the ODR?**
One Definition Rule — most entities need exactly one definition across all translation units. Violations = UB or linker errors.

**Q17: PLT/GOT?**
Indirection for dynamic symbol resolution on ELF. PLT provides trampolines; GOT holds resolved addresses. Affects call performance.

**Q18: `-O3` vs `-O2`?**
Generally more aggressive inlining, vectorization, unrolling heuristics. Exact difference is LLVM-version-specific.

**Q19: How to cut link times for a huge CMake monorepo?**
Use lld or mold, split static libs, split DWARF, avoid unnecessary LTO, fix overlinking.

**Q20: ccache correctness risk?**
Cache hit when preprocessor-visible inputs differ but hash misses a factor (wrong sloppiness, environment variables, `__DATE__`).

**Q21: What is a fatbin?**
NVIDIA container holding multiple device code images (different sm_XX SASS + PTX fallback). Runtime picks the best match.

**Q22: When is nvlink used?**
Linking relocatable GPU objects from separate compilation before final host link.

**Q23: Clang driver vs -cc1?**
`clang` is the driver (argument parsing, toolchain). `-cc1` is the actual compiler invocation. `-###` reveals the real commands.

**Q24: What is RVO?**
Return Value Optimization — elides copies by constructing return object directly in the caller's slot.

**Q25: Address spaces in NVPTX?**
Global, shared, local, constant memory. Each has distinct address space in IR. Loads/stores must use the correct space.
