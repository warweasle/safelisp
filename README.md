# SafeLisp

A small, embeddable, lisp designed to run untrusted code safely. 

Safe by default. Unsafe only when explicitly enabled.

Also an environment to test language ideas. 

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
- (TODO) Separate environments (local / global / system)
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

// cond statement 
(??? (test branch)
     (test branch)
     t ...)

// Equality (there is no = keyword, so there are never any = vs == errors)
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
// <| continue
(<>
   (PRINT "FOREVER!"))

(<?> predicate
     code)

// Read Eval Print Loop
(<> (print (eval (read))))

// return last in block and first in block.
(... code)
(1... code)

// lambda
(lambda (args) code)

(let ((a 1)
      (b 2)
      (l (lambda (c d) (print (+ c d)))))
   (l a b))
   

TODO:

// I need to reconfigure my rbtree to work with data pairs.
// Setting location 
(SET 'location 'value)
// Or for convenience
(SETQ location 'value)

// Just need to implement them as while and unless
(?? ... only true path..)

(!?? ....)

// Define function
(fun name args code)



//Add escapes for strings.

```
---

## Architecture

The parser is made in FLEX and BISON and creates the objects as soon as the text is parsed.
Unlike tradional lisps, there are no reader macros since it's a security nightmare and rarely used.
SafeLisp uses garbage collection and bignums to minimize errors.
It also uses short, easy to read functions for minimal mental load.
It is a scheme-1 which means there is only one symbol space for both functions and variables.

The idea is to make a simple, fast lisp that is familiar to C programmers.

