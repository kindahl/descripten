# Introduction
Descripten is an [ECMAScript](http://en.wikipedia.org/wiki/ECMAScript) to C
compiler. It consists of two major components: a compiler that compiles
ECMAScript source code into C, and a run-time library that's linked into the
generated C program.

Ultimately the project goal is to compile ECMAScript into
the [LLVM](http://llvm.org) language. Integration with LLVM will allow
Descripten to utilize the various tools for optimization while also providing
a compiler from ECMAScript to machine code without invoking an external C
compiler.

# Conformance
Descripten implements ECMAScript 5.1 and is continuously evaluated against the
[test262](http://test262.ecmascript.org/) test suite. Currently more than 99.6%
of all (11000+) tests passes (tested against revision 309).

# Building
Building is supported on GNU/Linux and Mac OS X. Windows is untested at this
moment. The following 3rd party libraries are required to build Descripten:
* [PCRE](http://pcre.org)
* [Boehm-Demers-Weiser Conservative Garbage Collector](http://hboehm.info/gc/)

You'll need to build the garbage collector from source because it needs to be
configured to find tagged pointers. Descripten uses 64-bit NaN-boxing and the
garbage collector should use the following pointer mask:
`0xffffffffffff`.

There is a patch available in `patches/gc/pointer_mask.patch`, but simply
putting the following into `gcconfig.h` should work:
```cpp
#define POINTER_MASK ((GC_word)0xffffffffffff)
```

You'll also need autotools, pkg-config and a C++11 compliant compiler. To
build, simply do the following:
```sh
./reconfigure
./configure
make -j8
```

# Using
Simply run the `compiler` binary to compile ECMAScript source files into C.
Then compile the generated C files with your favorite C compiler and link with
the *common*, *parser* and *run-time* libraries.

```sh
./compiler file1.js file2.js -o out.c
```

There is also a utility script that will perform all steps for you:
```sh
./run.py program.js
```
It will produce `program.c`, `program.o` and `program` which is the final
compiled binary. As the script name suggests it will also run the program for
you.

# Overview
Below are a few technical highlights that someone might find interesting.
#### NaN-boxing
In ECMAScript a value can be of many different types. To represent a value
internally Descripten uses a technique called NaN-boxing. By utilizing the fact
that double precision floating point (IEEE 754-1985) Not-a-Number values can
hide a 52-bit payload, Descripten can essentially represent all ECMAScript
value types using only 64-bits.
#### Property cache
The naive way of looking up a property on an object is to search the object
properties at run-time, using a hash map or similar. Property lookups are
however rather common which calls for a more efficient approach. Descripten
tries to classify objects at run-time, giving them an internal type in order to
use a property cache for fast property access.
#### Interpreter
Since the ECMAScript language contains facilities for evaluating the language
itself (the `eval` function), Descripten also contains an ECMAScript
interpreter. This interpreter is very rudimentary and not much effort has been
put into making it fast. It evaluates the AST using a standard visitor pattern.
It has merely been added for completeness and to be able to run the test262
test suite.

# Future
This project is a work in progress. Below you'll find a few ideas that I would
like to see implemented.
#### Use static single assignment form for internal code representation
Descripten compiles ECMAScript into an internal intermediate representation
before producing the C code. The intermediate representation does not currently
conform to [Single Static Assignment](http://en.wikipedia.org/wiki/Static_single_assignment_form)
form. However, it would be beneficial if it did since it would make program
analysis and optimization easier.
#### Perform static type inference
One benefit of ahead-of-time compilers is that they can afford spending more
time on static program analysis. Descripten should try to analyze the program
and infer value types. This would result in a great performance boost: values
can be unboxed and faster specialized routines can be used when operating on
the values.
#### Implement a generational, moving and precise garbage collector
Descripten currently uses a generic conservative garbage collector designed for
C and C++. It's not well tuned for this project. By using a generational and
moving garbage collector it should be possible to achieve both faster memory
allocations and faster collection cycles.
