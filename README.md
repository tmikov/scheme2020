# Scheme 2020

Scheme 2020 is intended to be a native compiler for the Scheme programming language, [R7RS-small](https://small.r7rs.org/). The goal is to complete a fully functional implementation in 48 working hours (or six work days).

An up to date time log is provided in [log.md](log.md).

Note 1: The number 48 is inspired by the great [Scheme48](http://s48.org/), which was famously implemented in [48 astronomical hours](http://mumble.net/~jar/s48/).

Note 2: The name "Scheme 2020" was picked after a quick poll of my friends, who were given two options: "QD Scheme" (from quick & dirty) and "Scheme 2020" (because I am doing this in 2020). 

## Introduction

I have always loved Scheme for its simplicity and elegance, and always wanted to implement a Scheme compiler. I made a couple of hobby attempts through the years, but somehow never got around to actually completing one (I guess that is not atypical for hobby projects).

Thinking back, these are common pitfalls:
- My plans were too ambitious and not very incremental. 
- I didn't really know what I was doing.
- I chose the wrong toolset.

The first one is certainly the most important. I believe that for almost any programming project project to be successful, it needs to be operational as soon as possible, and it needs to be kept operational in small and manageable increments. I also believe that good software engineering is frequently about making the right compromises at the right time.

So, in this project I have decided to force the issue by giving myself a very short and public deadline. Such a short deadline really gives one only very few options of achieving the goal, almost eliminating distractions and scope creep.

One immediate outcome of the deadline is that I have decided to use C++ as an implementation language instead of Scheme. While in some ways this is very disappointing (what can be cooler than a self-hosting compiler), it is the only realistic option for me, since I haven't actually written anything substantial in Scheme, and it would take more than 48 hours to get up to speed. 

If this works out, I would love to eventually rewrite the compiler in Scheme, so it can become self-hosting. This is not as crazy as it sounds, because not a lot of code can be written in 48 hours, so it should be similarly easy to rewrite. Plus, there would be a very convenient performance benchmark.

> Note: If I had decided to use Scheme (which I seriously considered), I would have used [Chicken Scheme](https://www.call-cc.org/). Why? Because its minimalistic approach appeals to me, not to mention [Cheney on the M.T.A.](http://home.pipeline.com/~hbaker1/CheneyMTA.html) which is possibly one of the coolest implementation techniques ever.

## Implementation

While I think I have planned the implementation in sufficient detail in my head, most of this section will likely be completed after the end of the 48 hours.

### Call/cc

`call/cc` presents one of the most crucial design decisions in a Scheme implementation because it affects most aspects of code generation and the runtime in pretty fundamental ways. An implementation could not really be said to be "true" Scheme without `call/cc` (which is not to say that it couldn't still be great or very useful). As such, `call/cc` is included in this project's commitment to a "fully functional Scheme in 48 working hours".

There are at least several well known ways of implementing `call/cc` in order of increasing complexity (this list is not meant to be exclusive):
1. Converting to continuation-passing-style (CPS).
2. Heap-allocated activation records.
3. Boxed mutable variables and stack copying (described in R. Kent Dybvig's dissertation).

All of the above are all well understood and relatively simple. While very elegant and interesting in the academic sense, these approaches result in code which is not optimal for modern optimization frameworks (like LLVM) or modern CPUs. For example, 1) and 2) defeat CPU return address prediction. These approaches also result in counter-intuitive performance, because of unexpected heap allocations and local variables living in the heap. Debugging is also unlike "regular" imperative languages.

All of this makes a canonical implementation of `call/cc` somewhat un-attractive. However, the fact remains that a serious Scheme implementation must support it. So, what is the plan?

Given the deliberately short time frame, the initial version of Scheme2020 has no choice but utilize approach 2). However, afterwards, when there is more time to experiment, I would like to transition to a different model:
- By default continuations will be single shot and *escaping*. This would be sufficient to implement co-routines, for example.
- Multi-shot escaping continuations will be supported via compiler option by CPS conversion.

> NOTE: in comparison, `setjmp()/longjmp()` or C++ exceptions can be said to be single-shot non-escaping continuations.

With this strategy, I believe Scheme2020 can eventually feel and perform very natural in an ecosystem of C/C++/Rust, etc, while preserving all of its expressivity and power.

### Numeric Tower

Traditionally Scheme implementations support the following number types:
- exact bigint
- exact rational (a ratio of bigints)
- inexact real (usually IEEE-754) 
- complex with any of the above components

From an implementation viewpoint this is not terribly interesting, as it is all done in library calls. Good implementations usually support some kind of efficient "small int" which automatically overflows into a heap-allocated *bigint*. The techniques for that are well known.

It is important to note that the R7RS-Small does not require implementations to support all of the above number types. The spec is pretty liberal in this respect and pretty much the only requirement is that an implementation must support separate *exact* and *inexact* number types. The behavior on overlow is not specified. This allows compliant Scheme implementations to exist with de-facto C number semantics.

Scheme2020 will initially support two number types: 
- IEEE 64-bit double
- 64-bit twos complement signed integer, with Java-like semantics (overflow wraps around).

As far as I can tell, this satisfies the requirements of the spec. More number types can be added incrementally if needed (rationals, complex). I do not plan on adding bigint support because philosophically it takes the implementation in a different direction (not a bad one, just one that I am not personally interested in). 
