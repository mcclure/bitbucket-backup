This last year I've been working on a programming language. About a year ago, I sat down and I wrote out a [list of all the things I want out of a language](http://msm.runhello.com/p/928). Nothing that exists right now felt satisfying, so I decided to make my own. I've named this language Emily, and I'm now up to a [working "version 0.2" release that you can download and run](https://bitbucket.org/runhello/emily/wiki/Home). This version is primitive in certain ways, but you can write a program in it and it points enough toward what the language will eventually be that I'm excited.

In this file:

[TOC]

## An example

Here's a small program in Emily:

    width = 80

    foreach ^upto ^perform = {
        counter = 0
        while ^(counter < upto) ^( perform counter; counter = counter + 1; )
    }

    inherit ^class = [ parent = class ]

    line = [                               # Object for one line of printout
        createFrom ^old = {
            foreach width ^at { # Rule 135
                final = width - 1
                here   = old at
                before = old ( at == 0 ? final : at - 1 )
                after  = old ( at == final ? 0 : at + 1 )
                this.append: ( here && before && after ) \
                         || !( here || before || after )
            }
        }
        print ^ = {
            this.each ^cell { print: cell ? "*" : " " }
            println ""                                          # Next line
        }
    ]

    repeatWith ^old = {  # Repeatedly print a line, then generate a new one
        do: old.print
        new = inherit line
        new.createFrom old
        repeatWith new
    }

    starting = inherit line        # Create a starting line full of garbage
    next = 1
    foreach width ^at (
        starting.append: at != next
        if (at == next) ^( next = next * 2 )
    )
    repeatWith starting                                             # Begin

This executes the 1d rule 135 cellular automata, or in other words, it prints strange pyramids:

    *  * *** ******* *************** ******************************* ***************
          *   *****   *************   *****************************   **************
     ****   *  ***  *  ***********  *  ***************************  *  ************
      **  *     *       *********       *************************       **********
    *       ***   *****  *******  *****  ***********************  *****  ********  *
      *****  *  *  ***    *****    ***    *********************    ***    ******
    *  ***          *  **  ***  **  *  **  *******************  **  *  **  ****  ***
        *  ********         *               *****************               **    **
     **     ******  *******   *************  ***************  *************    **
        ***  ****    *****  *  ***********    *************    ***********  **    **
     **  *    **  **  ***       *********  **  ***********  **  *********      **
           **          *  *****  *******        *********        *******  ****    **
     *****    ********     ***    *****  ******  *******  ******  *****    **  **
      ***  **  ******  ***  *  **  ***    ****    *****    ****    ***  **        **
       *        ****    *           *  **  **  **  ***  **  **  **  *      ******
    **   ******  **  **   *********                 *                 ****  ****  **
    *  *  ****          *  *******  ***************   ***************  **    **    *
           **  ********     *****    *************  *  *************      **    **
    ******      ******  ***  ***  **  ***********       ***********  ****    **    *
    *****  ****  ****    *    *        *********  *****  *********    **  **    **
     ***    **    **  **   **   ******  *******    ***    *******  **        **
      *  **    **        *    *  ****    *****  **  *  **  *****      ******    ****
            **    ******   **     **  **  ***               ***  ****  ****  **  **
    *******    **  ****  *    ***          *  *************  *    **    **
     *****  **      **     **  *  ********     ***********     **    **    ********
      ***      ****    ***         ******  ***  *********  ***    **    **  ******
    *  *  ****  **  **  *  *******  ****    *    *******    *  **    **      ****  *
           **               *****    **  **   **  *****  **       **    ****  **
    ******    *************  ***  **        *      ***      *****    **  **      ***
    *****  **  ***********    *      ******   ****  *  ****  ***  **        ****  **
    ****        *********  **   ****  ****  *  **       **    *      ******  **    *
    ***  ******  *******      *  **    **         *****    **   ****  ****      **
     *    ****    *****  ****       **    *******  ***  **    *  **    **  ****
       **  **  **  ***    **  *****    **  *****    *      **       **      **  ****

Anyway, even without knowing the syntax, you might notice a few things looking at this program:

- This language is **extremely** extensible. Near the beginning, I realized there were two things that I didn't get a chance to put in the standard library: a foreach function, and a way to instantiate an object of a class. So I just implemented them in the program. Very simple.
- Support for both ["unpure"] functional and object-oriented styles: Functions are being passed as values all over the place, functions are being created offhandedly (that's the `^`), objects with prototype inheritance are created very casually (that's `[]`), tail recursion works.
- Clean, familiar syntax: At least, assuming you've been writing a lot of Python, JS, or Lua, this looks a lot like code you've written before, other than the `^`s.

## Why's Emily special?: For language geeks

The one central New Idea in Emily is that **everything is a function**, or maybe put another way, objects and functions are interchangeable. You can think of an "object", or a structure, as being a function that maps key names to values; Emily actually treats objects this way. If an object can be a function, then anything can be; `3.add` can be the function that adds 3 to another number, `3` can be the function that maps `.add` to `3.add`.

The important thing about this isn't that functions specifically are important; what turns out to be important is simply that everything in Emily is the **same thing**. Everything acts like a unary (one-argument) function, so where other languages might have several "verbs" (field lookup, variable definition, function definition, function application, arithmetic) Emily has exactly one verb (function application). All you're doing is applying functions in a particular order, and writing code just means plugging those functions together like pipes. Even the "conventional" syntax elements, like `+` or the `.` lookups, are just unary function applications in disguise. Even declaring a variable is done with a function call.

So why does this matter?

## Why's Emily special?: The practical side

I like to build abstractions in my code. If I'm writing a C program, and I find I'm writing `for(int c = 0; c < upto; c++) {something}` over and over, I get annoyed; I wish I could just define a "foreach upto {something}", like I did in the pyramid program up top.

I like to mix different kinds of tools when I write software. I really like Lua, but Lua isn't very good for certain kinds of binary manipulation and threading, so I write programs that are part C++ and part Lua. But this is tricky; the languages each have their own complexities, and the complexities clash. Languages don't work well with each other.

I assert both these things get a *lot* easier if you have a language whose underlying model is very simple.

In Emily, where everything is function applications, building complex abstractions just means plugging the applications together in a particular order. The language *itself* doesn't need to be this simple-- again, if you look above, a lot of stuff is happening in the code that doesn't look like a function application. But that complexity is something built on top of the language, rather than being a fundamental part of the model. This means it can be extended further, and it also means it can be replaced.

Syntax like `+` or `=`, for example, is actually performing macro transformations-- `3 + 5 * 4` gets rewritten into `3.plus(5 .times 4)`. This makes these operators easy to extend-- if you want to design an object that "acts like" a number, you just define an object that implements the `.plus` and `.times` methods. It also makes them possible to replace-- the transformations are just little programs, and in a later version of Emily you'll be able to define your own operator transformations, if there's a different syntax you'd like better.

That's not all that impressive, though-- operator overloading is a pretty standard feature in languages, and macros are not the way you'd prefer to implement abstractions. Rather the more basic element in Emily of everything-is-function-application is what opens up the really powerful possibilities. Let's try something a little more unusual. As mentioned above, Emily has prototype inheritance. But it's single inheritance-- only one parent per object. What if you'd prefer multiple inheritance? Well, you can implement it yourself, by writing a single function:

    # A function that returns a function-- it generates the fake "parent"
    dualInherit ^parent1 ^parent2 = ^key {
        thisUpdate this: \
            (parent1.has key ? parent1 : parent2) key
    }

    hasAddOne = [
        addOne ^ = this.value = this.value + 1
    ]
    hasAddTwo = [
        addTwo ^ = this.value = this.value + 2
    ]

    child = [
        parent = dualInherit hasAddOne hasAddTwo
        value  = 0
    ]

    do: child.addOne
    do: child.addTwo
    println: child.value

So what's happening here? The object that `child` inherits from is chosen by setting the `parent` key. But objects and functions in Emily are interchangeable. So `child` **inherits from a function**. `dualInherit` takes two desired parents and manufactures a function which takes a key and executes it on whichever of the two parents knows how to respond. `child` then uses `dualInherit` to inherit from both the classes `hasAddOne` (from which it inherits the method `addOne`) and `hasAddTwo` (from which it inherits the method `addTwo).

## What else?

There's a couple more interesting things that made it into the language, even as early as this version is:

- `\version 0.2`
    This feature is small, but I believes it solves a somewhat fundamental problem with programming languages. Each Emily program is encouraged to start with a line identifying the language version it was developed against. When an Emily interpreter-- current or future-- encounters the `\version` line, it makes a decision about whether that code can be run or not. If the hosting interpreter is backward-compatible with the code's version, it just runs. But if backward-incompatible changes have been made to the language since then, it will either enter a compatibility mode or politely refuse to run. At some point, it will be possible to install these compatibility modes as pluggable modules.

    I write a lot of Python, and a huge running problem is compatibility between versions. In Python, as in most programming languages, the implementation version is the same as the language version. Python 2.4 runs Python 2.4, Python 2.7 runs Python 2.7, Python 3.1 runs Python 3.1, etc. Meanwhile Python 2.7 can run Python 2.4 code, but Python 3.1 **can't** run Python 2.7 code, which means Python is competing with itself and nobody uses Python 3 because all the code's written for 2.7. (And even before the big 3.0 switch, **forward** compatibility created a huge problem all by itself: If you had a program that used a feature from 2.5, but what you had installed was 2.4, you wouldn't know it until you tried to run it and something would break strangely, possibly at runtime.)

    This is all silly! Language versions define interfaces, and interpreters are engines. We shouldn't be holding back on upgrading our engines because the interface changed (and if there's some reason a new engine **can't** handle the old interface, it should at least fail very early). It's generally possible at least in principle to convert between these interfaces, so it should be possible to install something that does conversion for an incompatible past interface (probably even a future one!). It should be possible to mix code written against different interfaces in the same program-- maybe even the same file. There's surely a point at which this becomes untenable (library cross compatibility probably gets awkward quick), but language implementors not being able to get updates adopted because nobody wants to lose back compatibility with 15-year-old versions doesn't sound very tenable either.

    Anyway, for now: Just tag each file with its version, and all this becomes a lot easier to sort out later.

- Proper Unicode support

    This really ought to be something we just expect of a language these days, but Emily is being developed for full Unicode support and the interpreter treats source files as UTF-8. Right now this only extends as far as handling Unicode whitespace-- well, and the macro system supports unicode symbols, but since you can't make macros yet that's not so useful. As soon as possible though my goal is to implement [UAX #31](http://unicode.org/reports/tr31/) so you can use Unicode in identifiers, and I'm hoping to work with coders fluent in non-latin-script languages to make sure Emily is usable for those purposes.

    Oh, also: At some point I'm just gonna make smart quotes work as quotes. If we're gonna keep pasting them in to our code by accident, they might as well work.

- Return continuations

    I'm... not sure I should be calling too much attention to this, but it *is* a bit unique. As I've said, everything in Emily is a function, at least in form. There are no "keywords" as syntax constructs-- any "syntax" is just shorthand for function calls. So if I'm going to implement, say, `return`, it has to be something that can be treated like a function. `return` in Emily winds up being what's called a "continuation"-- a function-like object that when called just jumps to a particular place, in this case the end of the method call.

    This has an interesting side effect that I had not entirely intended:

        timeMachine ^ = (           # Function takes no arguments
            goBackward = return     # Store return in a variable. Yes, really.
            return 1                # Return. This line only runs once.
        )

        counter = do timeMachine    # The return value of "do TimeMachine" is 1, right?
        println: counter            # Only the first time-- every time we call goBackward,
        goBackward: counter + 1     # we return "counter + 1" from timeMachine,
                                    # even though timeMachine already ended.

    ...it turns out "return" for a particular function call can be treated like any other value, and even outlive the function call it was born in. As with any other kind of continuation, this creates the opportunity for some very powerful constructs (I'm trying to work out how I can implement Java-style named breaks with it) and also the opportunity for some really bad ideas. I'm going to see if I can find a way to encourage the former while maybe putting some limits on the latter.

## What next?

What you see here is part of a [more ambitious set of ideas](http://msm.runhello.com/p/934) (link goes to my original design writeup before I started writing any code). Here's some things I want in Emily eventually; some of this may not be completely feasible, but I think even making it part of the way there would be great.

- Types

    There's a lot of things I want to try with typing in Emily, and I'm nervous about elaborating too far on what they are before I know for sure what I can pull off. But in general: Emily should have type annotations. Typing should be "gradual"; you should be able to have a part of your program with type annotations and a part without. There should be a prover for types, and an inference engine. If you assign something to a variable or argument which has a type annotation, that should mean a runtime check for correctness. There should be a "compile time" check also; the compile time check should be able to tell some calls are definitely type-correct and optimize out the correctness check, and it should be able to tell some calls are definitely type-**incorrect** and refuse to run the script until they're fixed. (Right now, nothing stops you from typing `3(4)` except that this will fail at runtime).

    This is all pretty standard for a language written in the last ten years. But I want to try some odder things, based around Emily's core ideas that everything in the language is "the same thing", interchangeable with functions, everything is extensible and there is no syntax "magic". So: Types should be constructable at runtime. It should be possible to use a function as a type; there should be a syntax for turning `x such x > 4` into a type. Some types should just be language-level assertions, like there should be a type for "this variable's value is known at compile time" or "executing this function has no side-effects". There should be an ability along the lines of assigning a type as a key for an object-- something like `[ .name="minusone"; ^x of int = x - 1 ]` for a function that returns `"minusone"` when applied on `.name` or subtracts 1 when called with an int. In other words, I want average objects to be able to act like `match` or pattern-matching guards from a functional programming language. (This would mean that you could build up guards that inherit from other guards, which I kind of like.)

- C++

    I need to be able to interoperate with other languages. I write video games, so there's a whole bunch of C++ libraries I really need access to. Interoperability with C++ is hard. Interoperability with **anything** is hard. As alluded above I used to write a lot of Lua, and the interaction layer with C++ was just incredibly complicated and involved generating code with a Python script.

    Complicated or no, I'm going to have to write an interop layer for Emily. But I think that there are some design choices in Emily that are going to make this easier. Because Emily is so dynamic, I can generate a bunch of stuff at runtime that otherwise would have required a code generator, and Emily's everything-is-functions rule means that wrappers can be much less awkward than they might in another language. Objects and classes and methods from different languages all have different semantics; if you have an object in one language which actually wraps a guest object from another language, you're going to wind up with weird mismatches where the objects and methods you're interacting with in the host language don't quite behave like their analogues in the guest language (the "complexity clash" I mentioned earlier). But an object in Emily is a smooth ball; its semantics are just a matter of what arguments you feed it. If the type system is smart, it could even verify that the arguments you feed the wrappers are always consistent with the semantics of the guest object.

- Reader macros

    Emily has macros that transform the AST into a different AST before it is executed. A lot of functional languages have the concept of a "reader macro", which generates AST from whole cloth. There's a piece of implied code called the "reader" which is consuming the text of your program and parsing it into an AST; `\` will eventually be a way of sending messages to this reader, possibly enabling transformations that a normal macro couldn't perform because it would need access to the program source as a string. Right now the only reader instruction is `\version`, and the only thing it can do is halt the program if the version is wrong. There will be more reader instructions in future (for example, `\op` will probably be the way to define the normal, non-reader macros).

    What really interests me though is the possibility of a `\reader`, which would cause the reader to just plain get out of the way and hand off parsing the rest of the file to a piece of Emily code. My daydream here is that if you could write a `\reader` that parses some completely other programming language and emits Emily ASTs (hopefully easy since Emily really only barely has an AST, the AST is just a tree of applications)-- and if I'm right that it will be unusually easy to write Emily code that accepts other languages' objects as guests-- then Emily could become a useful intermediate layer that knows how to translate between several different languages and mediate their differences.

These are all Big Ideas though, and the ideas the language **already** has could stand some cleanup, so for 0.x I'm going to be focused on basic functionality improvements (less confusing scoping, operators on strings, short-circuiting booleans, user-defined operators, unicode, package loading, IO).

## Downloading and running Emily

As mentioned, Emily is available [from a BitBucket page](https://bitbucket.org/runhello/emily) (or, if you can't use Mercurial, its [GitHub mirror](https://github.com/mcclure/emily)), but not yet in any other form. You will need to compile it yourself; it's written in Objective Caml, so you'll need to install that first. For instructions, see [build.md](build.md). By the time you read this, there may also be install packages; see [emilylang.org](http://emilylang.org).

### What's "BitBucket"?

If you're not familiar with BitBucket or Github, go to the [Downloads page on BitBucket](https://bitbucket.org/runhello/emily/downloads), click the "Branches" tab, and to the right of the word "stable" click "zip". This will download a zip file of the most recent stable release.

### Getting started

If you're interested in Emily and want to give it a shot, a good thing to read next would be the [tutorial](tutorial.md). If you just want to know more, there's a bunch of documentation including some design documents and information about modding the language in the [docs folder](.).