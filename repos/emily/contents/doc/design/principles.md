# Emily's underlying principles

*In no particular order*

## I am not important

I do not think that the fact I (me, Andi, the woman writing these words right now) wrote the interpreter and spec for this language means that I should hold a particularly privileged position in how the language works or what it should do.

Here is a problem. If you look at your average programming language, you'll find there's often kinds of data manipulation that the language makes "easy" (via built-in operators, type support, conveniently structured syntax) and kinds it doesn't. This inevitably corresponds to what the original language author found interesting. Erlang is really good at bit/byte manipulation and quite bad at string manipulation, Python is the reverse. This is because at the time those languages were written, Joe Armstrong had a job involving a lot of byte manipulation and Guido van Rossum had a job involving a lot of string manipulation. Those languages are now forever stuck being really good at one thing and less good at another, just because of who the language implementor was.

As a separate but in my view related issue, there's a pattern where language authors often find their own rules are too strict when implementing the standard libraries-- but rather than relaxing the rules, just give the standard library an exception or carved-out language quirk. These often seem to group around the "print" function. Modern Clang has this really neat feature where "%d"s and "%f"s and such in printf and NSLog statements are typechecked and result in compiler warnings, but it doesn't seem to be possible for a random third party to implement such a compile-time warning system for a custom string formatter in their own libraries. (Even in the original C, the vararg capability is pretty clearly designed just to make printf possible, and isn't very convenient for any other use.) Strongly typed functional languages often have similar implementor privilege around the magic "print" function.

Languages are simply written in such a way there's a wall, with some concepts surrounding the language the domain of the language implementor and some concepts the domain of the "user". The wall is sometimes in a different place (language implementors may choose to offer operator overloading, say), but the wall is always there.

I don't want this wall there at all. Users or at least library authors should be able to do anything, **anything**, including things we currently consider unthinkable (participate in generation of error messages, define compiler optimization passes?), including things we typically consider simply a very bad idea (redefine syntax).

When I am implementing this language, if there is something the language "does", I choose to implement that feature using some sort of mechanism that the user can also take control of. Even better would be if I could design the language *around* general mechanisms anyone can take control of, and then implement the language features I want on *top* of that.

### I trust you

One of the consequences of this philosophy is I will wind up making possible a lot of user behaviors which are "a bad idea". There's this idea in language design that if you give the user too much power, they will hurt themselves with it, and therefore the user's power must be constrained to behaviors which are "safe". This makes a lot of sense, but it puts me (the language implementor) in a position where I am allowed to decide what is "safe" and the user is not. I'm not comfortable with that.

I choose to trust the user. "Safety measures" in a language are a good thing, but if the user has a need to go around those safety measures there should be *some* way to do that.

As the language author, it is my job to make it hard for you to do something wrong by accident. It is not my job to make it hard for you to do something wrong on purpose.

## Minimize the number of concepts

## "Anything a code generator can do", in-language

The language should be dynamic enough that if you ever feel tempted to write a code generator, it's actually easier to use some language-built-in facility.

### One language

As opposed to one language for code, plus one language for templates, plus one language for macros, plus one language for type definitions, plus one language for build scripts...

Many languages, when they need a sub-language for controlling the behavior of some particular quirky thing, define something new with its own syntax. Although macros do create the possibilities of DSLs, this should be avoided to the greatest extent possible. At the least, any "little languages" should just be sugar for the main language, so you can fall back to the main language if you need.

### Define objects by their behavior, not their nature

## DRY/SWYM

Destroy all boilerplate

Destroy all idiomatic code

You should not have to be continously trying to remember "how to do it" (do I type , ; or space?)

Computers are good at repetitive, non-executive tasks and bad at decisionmaking. Humans are good at decisionmaking and bad at repetitive tasks. If there is something such that "you have to do it every time", it is something the computer could and should be doing.

The language you use should be precise. Using "inline" to describe "do not duplicate across linking units" is bad because eventually it comes to mean *only* that and you no longer have a way to say something is "inline".

## No parser

### Types determine what is possible

## Privilege ease over correctness and performance

Programming should be easy. Programming should be egaltarian.

Words are better than symbols.

Words are better than dvwlld grbg.

### Correctness and performance should be *possible*

### Don't pay for what you don't use

C++ has a design rule like this, but gets it backward; it ensures you never have to pay a performance cost for language features you aren't at any one time use, but in order to enforce this rule it usually incurs heavy costs in terms of usability and syntax complexibility. For one very small example, C++ saves a few bytes per object by only including vtables when a virtual function is present, but the costs to this are (1) writing "virtual" over and over when doing OO engineering, (2) by-default unsafe behavior on destructors, (3) excluding certain kinds of dynamism from the language entirely, and (4) the mental weight of objects following weirdly different rules depending on the nonobvious question of whether at least one "virtual" exists in the inheritance chain. You incur these usability and safety costs even if you, in your particular application, do not have a need for the performance benefit this vtable quirk enables.

"Don't pay for what you don't use" should be a more general principle, and the primary emphasis-- in terms of what we're protecting from "cost"-- should be usability. If there's a way that dynamism can be traded off for performance, the user should not have to pay the cost of reduced dynamism unless they decide they actually want that performance. If I decide that there should be some way for a user to perform unchecked memory accesses, the user should not have to pay the stability and safety cost associated with that being possible unless they actually have a need for it.

## It should be hard to write yourself in a corner

### Parity of expressions and definitions

## Deployability is all-important

## No magic

## No super cow powers