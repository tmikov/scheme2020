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

While I think I have planned the implementation in sufficient detail in my head, this section will likely be completed after the end of the 48 hours.



 