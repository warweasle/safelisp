# SafeLisp

A small, embeddable Lisp designed to safely run untrusted code.

Safe by default. Unsafe only when explicitly enabled.

Otherwise, as simple as I can make it.

---

## Goals

- Safe and Simple for embedding.
- Memory safe (GC managed)
- Bignums!
- Access Security
- Tiny runtime, easy to embed in C/C++
- Good for scripting, config, DSLs, and automation

---

## Features

- Minimal Lisp core (symbols, lists, numbers, strings)
- Tree/map structures (RB-tree maps)
- Closures
- Simple unhygienic macros (MACRO)
- A minimal restart system for structured recovery (WITH-RESTART / INVOKE-RESTART)
- Embeddable interpreter
- Small dependency footprint
- Separate environments (local / global / system)
- (TODO) Optional unsafe/volatile mode (for trusted code.)
- (TODO) Foreign function interface (restricted)

---

## Syntax

SafeLisp uses C Style comments like // or /* */

Symbols are compared without regard to case.

This is a work in progress, please look at the safelisp_parser.l for a list of current functions.

```C

// Quote
'

// QUASIQUOTING
`(a b c) => (A B C)
`(a ,(list hello world) c) => (A (HELLO WORLD) C)
`(a ,@(list hello world) c) => (A HELLO WORLD C)

// Variables (single namespace for functions and data)
// Let behaves as let*, where previous values are available
// as soon as they are declared.

(LET ((A 1)
      (B (+ A 1)))
  (PRINT B)) 

// SET takes the target name unevaluated (bare, not quoted) and evaluates
// its value argument.
(SET location 'value)

// SETQ (a non-evaluating variant) is not yet implemented.

// True
TRUE

// False
NULL
// FALSE or Nil 
(! NULL) or (! 0) or (! 0.0) == TRUE
// Everything else is TRUE. 

// IF statement
(? predicate
   if-true)

(? predicate
   if-true
   if-false)

// Not if statement.
(!? predicate
    if-false
    if-true)

(!? predicate
    if-false)

// cond statement -- one argument: a list of (test branch) pairs.
(?... (((test1 branch1) (test2 branch2))))

// When and unless.
(?? predicate
    code...)

(!?? predicate
     code...)

// Equality (there is no '=' keyword, so there are never any '=' vs '==' errors)
(== a b)

// Inequality
(!= a b)
(< a b)
(> a b)
(<= a b)
(>= a b)

// AND
(&& true true true... )

// OR
(|| true true true... )

// NOT (acts as null?)
(! TRUE)

// Loops are surrounded by <>
// BREAK exits the loop with a value. There is no continue yet.
(<>
   (PRINT "FOREVER!"))

(<> (?... (((== N 3) (BREAK N)))))

(<?> predicate
     code)

// Read Eval Print Loop
(<> (PRINT (EVAL (READ))))

// return last in block and first in block.
(... code)
(1... code)

// lambda
(LAMBDA (ARGS) CODE)

(LET ((a 1)
      (b 2)
      (l (lambda (c d) (print (+ c d)))))
   (l a b))

// Simple unhygienic macros -- macro params bind to the caller's raw,
// unevaluated forms; the body produces an expansion form, which is then
// evaluated once more in the caller's environment.
(MACRO (ARGS) CODE)

// FUN/MAC/FLET/MLET are provided as prelude.safe macros built on MACRO
// itself (see prelude.safe), not native C forms:

// Defining a global function
(FUN FUNC-NAME (ARGS) CODE)

// Defining a global macro
(MAC MACRO-NAME (ARGS) CODE)

// Local functions
(FLET ((F1 (ARGS) CODE)
       (F2 (ARGS) CODE))
  (F1 ...)
  (F2 ...))

// Local macros
(MLET ((M1 (ARGS) CODE))
  (M1 ...))

// A minimal restart system -- not a full condition system, just a named
// dynamic-extent recovery point. INVOKE-RESTART transfers control back to
// the matching WITH-RESTART and runs its recovery lambda with that value.
// Ordinary ERROR values are untouched -- this is for structured recovery,
// not for representing failures.
(WITHRESTART NAME RECOVERY-LAMBDA CODE...)
(INVOKERESTART NAME VALUE)

// Call a function with an already-built list of argument values.
(APPLY FUNC ARG-LIST)

// Fold a list into a single value.
(REDUCE FUNC LIST INITIAL)

// Keep only the elements a predicate accepts.
(FILTER PRED LIST)

```
---

## Architecture

The parser is made in FLEX and BISON and creates the objects as soon as the text is parsed.
Unlike traditional lisps, there are no reader macros since it's a security nightmare and rarely used.
SafeLisp uses garbage collection and bignums to minimize errors.
It also uses short, easy to read functions for minimal mental load.
It is a scheme-1 which means there is only one symbol space for both functions and variables.

Evaluation is simple left to right, from inside out.
Special forms excluded. 

The idea is to make a simple, fast lisp that is familiar to C programmers.

## TODO

- Implement SETQ (non-evaluating SET)
- Add string escape sequences
- Add char types
- Add nativeC types
- Full condition/restart system beyond the minimal WITH-RESTART/INVOKE-RESTART
  primitives currently provided (no condition classes, no signal/handler-bind)