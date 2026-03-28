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

// SET evaluates its first argument. SETQ does not. (currently broken)
(SET 'location 'value)

// Or for convenience
(SETQ location 'value)

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

// cond statement 
(??? (test branch)
     (test branch)
     t ...)

// When and unless are here too.. (not yet implemented)
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
// |> break
// <| continue (Not yet implemented!)
(<>
   (PRINT "FOREVER!"))

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

//----COMING SOON!!!

// Defining global functions
(FUN (FUNC-NAME ARGS ARGS ...) CODE)

// Defining global macros
(MAC (MACRO-NAME ARGS ARGS ...) CODE)

// Local functions
(FLET ((func1 (args args) code))
        ((func2 (args args) code)))

   (func1 ...)
   (func2 ...))

// Local macros
(MLET ((macro1 (args args) code))
       ((macro2 (args args) code)))

   (macro1 ...)
   (macro2 ...))


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

- Improve RB-tree structure for key/value pairs
- Implement SET / SETQ
- Add string escape sequences
- Expand control flow forms (in safelisp)
- Add char types
- Add nativeC types