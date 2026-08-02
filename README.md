# SafeLisp

A small, embeddable Lisp designed to safely run untrusted code.

> **Safe by default. Unsafe only when explicitly enabled.**
>
> Otherwise, as simple as I can make it.

SafeLisp is intended for scripting, configuration, automation, and domain-specific languages inside C and C++ applications. It combines a compact Lisp runtime with garbage collection, arbitrary-precision numbers, macros, structured recovery, maps, higher-order functions, and multiple values.

SafeLisp is its own language rather than an attempt to exactly implement Common Lisp or Scheme.

---

## Goals

- Safe and simple embedding
- Memory management through garbage collection
- Arbitrary-precision numbers
- Explicit access control
- A tiny runtime that is easy to embed in C or C++
- Predictable evaluation with as little hidden behavior as possible
- Useful for scripting, configuration, DSLs, and automation
- Powerful language features without a large runtime

---

## Current Features

### Core language

- Case-insensitive symbols
- Strings, symbols, lists, dotted pairs, integers, and floating-point numbers
- `TRUE` and `NULL`
- Lexical closures and first-class procedures
- A Lisp-1 namespace: functions and values share the same symbol space
- `LET` bindings with `let*` behavior
- Assignment with `SET`
- Quoting, quasiquoting, unquoting, and splicing
- Simple unhygienic macros with `MACRO`
- Sequencing with `...` and `1...`
- Left-to-right evaluation, except where special forms define otherwise

### Collections and functional programming

- Cons cells and generalized `C[AD]+R` accessors
- Lists and dotted lists
- RB-tree-backed maps
- `MAP`, `FILTER`, `REDUCE`, and `APPLY`
- String construction and manipulation
- Multiple return values

### Control flow

- If and negative-if forms
- Cond-style branching
- When and unless
- Infinite loops and predicate-controlled loops
- `BREAK` with a return value
- A minimal named restart system for structured recovery

### Runtime and embedding

- Garbage-collected memory
- GMP-backed arbitrary-precision arithmetic
- Separate local, global, and system environments
- Flex/Bison parser
- Embeddable C runtime
- Small dependency footprint

---

## Syntax Basics

SafeLisp supports C-style comments:

```lisp
// Single-line comment

/*
   Multi-line comment
*/
```

Symbols are case-insensitive:

```lisp
hello
HELLO
HeLlO
```

All three names refer to the same symbol.

Strings use double quotes:

```lisp
"Hello, world"
```

The current string reader does not yet implement escape sequences.

---

## Truth

`TRUE` is the canonical true value.

`NULL`, integer zero, and floating-point zero are false. Everything else is true.

```lisp
(! NULL)   // TRUE
(! 0)      // TRUE
(! 0.0)    // TRUE
(! TRUE)   // NULL
```

---

## Quoting

```lisp
'hello
'(a b c)
```

SafeLisp also supports quasiquote, unquote, and splice:

```lisp
`(a b c)
`(a ,(list hello world) c)
`(a ,@(list hello world) c)
```

Example results:

```lisp
`(a b c)
=> (A B C)

`(a ,(list hello world) c)
=> (A (HELLO WORLD) C)

`(a ,@(list hello world) c)
=> (A HELLO WORLD C)
```

---

## Variables and Binding

SafeLisp has one namespace for procedures and values.

`LET` behaves like `let*`: each binding is available to the bindings that follow it.

```lisp
(LET ((A 1)
      (B (+ A 1)))
  (PRINT B))
```

`SET` takes its target name unevaluated and evaluates the value:

```lisp
(SET LOCATION 'VALUE)
```

`SETQ`, a non-evaluating variant, is not yet implemented.

---

## Procedures

Create an anonymous procedure with `LAMBDA`:

```lisp
(LAMBDA (X Y)
  (+ X Y))
```

Procedures are first-class values:

```lisp
(LET ((A 1)
      (B 2)
      (ADD (LAMBDA (X Y) (+ X Y))))
  (ADD A B))
```

`APPLY` calls a procedure with an already-built list of argument values:

```lisp
(APPLY FUNC ARG-LIST)
```

`PROCEDURE?` tests whether an object can be called.

---

## Macros

`MACRO` creates a simple unhygienic macro.

Macro parameters are bound to the caller's raw, unevaluated forms. The macro body produces an expansion form, and that expansion is evaluated in the caller's environment.

```lisp
(MACRO (ARGS)
  CODE)
```

The standard prelude builds convenient definition forms on top of `MACRO`. These are SafeLisp macros, not native C forms:

```lisp
(FUN FUNCTION-NAME (ARGS)
  CODE)

(MAC MACRO-NAME (ARGS)
  CODE)

(FLET ((F1 (ARGS) CODE)
       (F2 (ARGS) CODE))
  (F1 ...)
  (F2 ...))

(MLET ((M1 (ARGS) CODE))
  (M1 ...))
```

---

## Conditionals

### If

`?` evaluates its predicate and then selects a branch:

```lisp
(? PREDICATE
   IF-TRUE)

(? PREDICATE
   IF-TRUE
   IF-FALSE)
```

### Negative if

`!?` reverses the branch order:

```lisp
(!? PREDICATE
    IF-FALSE)

(!? PREDICATE
    IF-FALSE
    IF-TRUE)
```

### Cond

`?...` accepts one list containing `(test branch)` pairs:

```lisp
(?... ((TEST1 BRANCH1)
        (TEST2 BRANCH2)))
```

### When and unless

```lisp
(?? PREDICATE
    CODE...)

(!?? PREDICATE
     CODE...)
```

---

## Logic and Comparison

```lisp
(== A B)
(!= A B)

(< A B)
(> A B)
(<= A B)
(>= A B)

(&& VALUE...)
(|| VALUE...)
(! VALUE)
```

SafeLisp deliberately uses `==` for equality. There is no ambiguous assignment-versus-comparison use of `=`.

---

## Loops and Sequencing

### Infinite loop

```lisp
(<>
  (PRINT "FOREVER"))
```

### While loop

```lisp
(<?> PREDICATE
  CODE...)
```

### Breaking from a loop

`BREAK` exits the current loop and may return a value:

```lisp
(<>
  (?... ((((== N 3) (BREAK N))))))
```

There is currently no `continue` form.

### Blocks

`...` evaluates a block and returns its last value:

```lisp
(... FORM1 FORM2 FORM3)
```

`1...` evaluates a block and returns its first value:

```lisp
(1... FORM1 FORM2 FORM3)
```

---

## Lists

```lisp
(CONS A B)
(CAR LIST)
(CDR LIST)
(LIST A B C)
(APPEND LIST1 LIST2)
```

Generalized accessors are recognized directly:

```lisp
CADR
CADDR
CAADR
CDDDDR
```

Any valid `C[AD]+R` combination is supported.

Useful list operations include:

```lisp
(LEN VALUE)
(MAP FUNC LIST)
(FILTER PRED LIST)
(REDUCE FUNC LIST INITIAL)
(APPLY FUNC ARG-LIST)
```

`NULL?` tests for `NULL`, and `CONS?` tests for a cons cell.

---

## Maps

Maps are backed by red-black trees.

Native map operations are:

```lisp
MAPMAKE
MAPADD
MAPGET
MAPSET
MAPDEL
```

`MAP` is the higher-order sequence operation and is separate from the map data structure.

---

## Strings

`CAT` concatenates text-like values:

```lisp
(CAT "Hello " 'WORLD "!")
=> "Hello WORLD!"
```

String operations currently include:

```lisp
CAT
LEN
SUBSTR
STRREF
STRUPPER
STRLOWER
STREQ
TOSTRING
```

`PRINT` writes an object, while `TOSTRING` produces its string representation.

---

## Arithmetic

Basic arithmetic:

```lisp
(+ VALUE...)
(- VALUE...)
(* VALUE...)
(/ VALUE...)
```

Integer division and remainder operations:

```lisp
(MOD A B)
(QUOTIENT A B)
(REMAINDER A B)
```

Rounding operations:

```lisp
(FLOOR VALUE)
(CEILING VALUE)
(ROUND VALUE)
(TRUNCATE VALUE)
```

Additional numeric operations:

```lisp
(ABS VALUE)
(SQRT VALUE)
(EXPT BASE POWER)
(MIN VALUE...)
(MAX VALUE...)
(GCD VALUE...)
(LCM VALUE...)
```

Numeric predicates:

```lisp
(EXACT? VALUE)
(INEXACT? VALUE)
```

Integer and floating-point literals are read directly by the parser and stored using GMP-backed numeric objects.

---

## Types and Predicates

```lisp
(TYPE VALUE)
(TYPE? VALUE TYPE)
(NULL? VALUE)
(CONS? VALUE)
(PROCEDURE? VALUE)
(EXACT? VALUE)
(INEXACT? VALUE)
```

---

## Multiple Values

SafeLisp supports multiple return values through:

```lisp
VALUES
NTHVALUE
MULTIPLEVALUELIST
```

These allow a procedure to return more than one logical result without constructing a temporary user-visible list.

---

## Structured Recovery

SafeLisp provides a deliberately small restart system.

A restart is a named, dynamic-extent recovery point. `INVOKERESTART` transfers control to the matching `WITHRESTART` and calls its recovery procedure with the supplied value.

```lisp
(WITHRESTART NAME RECOVERY-LAMBDA
  CODE...)

(INVOKERESTART NAME VALUE)
```

Available restart names can be inspected with:

```lisp
(AVAILABLERESTARTS)
```

This is not yet a full Common Lisp-style condition system. Ordinary error values are unchanged; restarts provide an explicit control-transfer mechanism for recoverable situations.

---

## Evaluation and I/O

```lisp
(READ)
(EVAL FORM)
(PRINT VALUE)
(TOSTRING VALUE)
(TYPE VALUE)
```

A minimal read-eval-print loop can be written in SafeLisp:

```lisp
(<>
  (PRINT (EVAL (READ))))
```

---

## Native Language Surface

The lexer currently recognizes these native forms and operations:

### Core

```text
SET LET LAMBDA MACRO
QUOTE / '  BACKTICK / `  COMMA / ,  SPLICE / ,@
EVAL READ PRINT TOSTRING TYPE TYPE?
... 1...
```

### Control and recovery

```text
? !? ?... ?? !??
<> <?> BREAK
WITHRESTART INVOKERESTART AVAILABLERESTARTS
```

### Lists and higher-order operations

```text
CONS CAR CDR C[AD]+R LIST APPEND
MAP FILTER REDUCE APPLY LEN
```

### Maps

```text
MAPMAKE MAPADD MAPGET MAPSET MAPDEL
```

### Logic and comparison

```text
== != < > <= >=
&& || !
NULL? CONS? PROCEDURE?
```

### Numbers

```text
+ - * /
MOD QUOTIENT REMAINDER
FLOOR CEILING ROUND TRUNCATE
ABS SQRT EXPT MIN MAX GCD LCM
EXACT? INEXACT?
```

### Strings

```text
CAT SUBSTR STRREF STRUPPER STRLOWER STREQ
```

### Multiple values

```text
VALUES NTHVALUE MULTIPLEVALUELIST
```

---

## Architecture

SafeLisp is implemented in C.

The reader/parser is built with Flex and Bison and creates runtime objects as input is parsed. SafeLisp intentionally does not provide user-defined reader macros: they increase the trusted surface, complicate embedding, and make sandboxing harder.

The runtime uses garbage collection and arbitrary-precision arithmetic to remove common sources of application-level errors. The implementation favors short, direct functions and explicit behavior to keep the language understandable from top to bottom.

SafeLisp is a Lisp-1: functions and variables occupy the same namespace.

Evaluation is left-to-right and inside-out, except for special forms that control evaluation themselves.

The language is intentionally compact and designed to feel approachable to C programmers without giving up Lisp's ability to extend itself.

---

## Planned Work

- Implement `SETQ`
- Add string escape sequences
- Add character types
- Complete native C data types
- Add an optional restricted foreign-function interface
- Add an explicitly enabled unsafe/volatile mode for trusted code
- Expand the minimal restart mechanism into a fuller condition system
  - condition classes
  - signaling
  - handlers
  - richer restart metadata

---

## Project

Source code:

https://github.com/warweasle/safelisp
