# SafeLisp

A small, embeddable, lisp designed to run untrusted code safely. 

Safe by default. Unsafe only when explicitly enabled.

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
- (TODO) Separate environments (local / global / system)
- (TODO) Optional unsafe/volatile mode (for trusted code.)
- (TODO) Foreign function interface (restricted)
- Tree/map structures (RB-tree maps)
- (TODO) Closures
- Embeddable interpreter
- Small dependency footprint

---

## Syntax

SafeLisp uses C Style comments like // or /* */

Symbols are compared without regard to case.

This is a work in progress, please look at the safelisp_parser.l for a list of current functions.

```lisp

"Quote"
'

"True"
TRUE

"FALSE or Nil"
NULL

"IF statement"
(? predicate
   if-true)

(? predicate
   if-true
   if-false)

"COND"
(?? (test branch)
    (test branch)
    ...)

"Equality"
(== a b)

"Setting location"
(= location 'value)

"AND"
(&& true true true... )

"OR"
(|| true true true... )


"Read Eval Print Loop"
(loop (print (eval (read))))

```
---

## Architecture

The parser is made in FLEX and BISON and creates the objects as soon as the text is parsed.
Unlike tradional lisps, there are no reader macros since it's a security nightmare are rarely used.
SafeLisp uses garbage collection and bignums to minimize errors.
It also uses short, easy to read functions for minimal mental load.
It is a scheme-1 which means there is only one symbol space for both functions and variables.

The idea is to make a simple, fast lisp that is familiar to C programmers.