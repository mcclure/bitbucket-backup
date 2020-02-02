**Emily programming language, version 0.3b**  
**Tutorial**

This is a quick overview of how to write programs in the Emily programming language. I assume you have written in some programming language before, I assume you know how to access your operating system's command line, and I assume you have [already installed the interpreter](build.md).

If you just want an explanation of what Emily is, see [intro.md](intro.md). If you want exhaustive documentation of the language, see [manual.md](manual.md).

Table of Contents:

[TOC]

# For starters

## A really simple program

Let's start with a really simple emily program:

    println 3

If you run this program, it will print "3.". I suggest you try running this program by saving it to a file `play.em` and then running `emily play.em` at the command line. Alternately, you can run the program from the command line directly by typing `emily -e 'println 3'`.

What is this program doing? Well, Emily is all about applying functions. `println` is a function which prints its argument followed by a newline. In most languages, if you want to apply a function, there's a special syntax for it, usually `()`. In Emily the way you apply a function to an argument is you just write the argument after the function. `println(3)` would have worked too, but the `()` are just parenthesis. They don't mean anything special.

Emily has variables, and we can assign to them with `=`. So we can say:

    a = 3
    b = println
    b a

And this will print "3", again. Notice: Functions are just values. You can store a function in a variable. You can pass a function as an argument to a function. You can return a function from another function. In fact, `println`, when you call it, returns a value, and that value is a function. What `println x` returns is another copy of `println`. So you can say:

    a = println 3
    a 4

And this will print

    3.
    4.

If you write several things on a line one after another, it will apply them one by one-- it will apply the first value to the second value, and then apply the return value of that to the third value, and the return value of **that** to the fourth value, and so on. So since `println` returns itself, you can say:

    (println 3) 4

Or:

    println 3 4

And this again will just print

    3.
    4.

## Values and math

Besides variable names, there are a couple different types of values you can write into your program. There are numbers and strings:

    3
    3.3
    "Okay"

All numbers are floating-point. You can do math on the numbers:

    println (3 + 4)
    println (4 * 5)
    println (5 * (6 + 7))
    println (3 == 3)
    println (4 + ~8 > 10 - 3)

(In this version of the language, `~8` is how you write "negative 8". You can't just say `(-8)`.)

If you run this code you get:

    7.
    20.
    65.
    <true>
    <null>

You'll notice two additional kinds of values here, when we used the equality operators: `true` and `null`. You can get these values directly in your program by saying `true` or `null`. `null` is just what Emily calls false.

You can't do anything to a string right now except `print` or `println` it. (`==` on strings also works.)

There's three more kinds of values: Closures, objects, and atoms.

## Making functions

The `=` operator can do a lot more than assign variables. It can also make functions:

    twice ^number = number * 2

This makes a function "twice" which multiplies a number by two. Functions, again, are just values, so you're actually just assigning a function to the variable "twice"; this would have done the same thing:

    twice = ^number (number * 2)

And we could use this function without creating a variable at all:

    println ( ( ^number (number * 2) ) 4 )

That might be a confusing way to write it! But this applies an "anonymous" function which doubles its arguments to the number 4, then prints the result. This code prints `8`.

`^`, wherever you see it in Emily, just means "make a function here". I call the functions that are made with ^ "closures". I'll explain why later.

## Making objects

One more thing you can make in Emily is an "object". Objects store things. You can make an empty object by saying `[ ]`. Or you can store values in it, like this:

    numbers = [
        one = 1
        two = 2
        three = 3
    ]
    println (numbers.two)

This will print `2.`

Any `=` statement inside of a `[ ]`, rather than assigning to a variable, will assign to a field inside of the new object. At the end of the `[ ]`, the new object is returned.

An object is a mapping of keys to values. You can say:

    obj = []
    obj 3 = "three"
    println (obj 3)

And this will print `three`. This is actually not any different from what we were doing a moment ago with `numbers.two`; `.` is not some magic field-access operator in this language, rather `.two` is just a kind of value, and the `numbers` object maps the value `.two` to the value `2`. `.anyIdentifierHere` is what's called an "atom", and it is a special kind of string. You can store it in a variable:

    key = .two
    println (numbers key)

This will print `2`.

Above we see `=` being used both to set the values of variables, and to set the values of keys on objects. There is a trick: `=` is doing the same thing in both cases. Every line of code has a "scope object", which holds all its variables. When you write an identifier by itself, like `numbers`, what you are doing is looking up the key `.numbers` on the scope object; `numbers.a` is the same as saying `(SCOPE).numbers.a`, if you could somehow get a reference to the scope. When you use `=` inside an object literal, assigning to the scope object reroutes the assignment to the object you are building.

# Fancier stuff

## Statements and groups

You can put comments on lines with `#`.

    println (3 + 4)   # Prints 7

As you saw above, you can put a series of statements on lines one after the other, and they're executed in order. You can put multiple statements on one line with `;`:

    a = 3; println a  # Prints 3

If you want to do the opposite and break a single statement across multiple lines, you can do this with `\`:

    println 3 \    # This prints a 3, then a newline, then a 4
            4

You can also put multiple statements, (i.e. multiple lines) inside of a `()` or a `[]`. I call these parenthetical-type things "groups". If you put multiple lines in a `( )`, the group evaluates to the value of the final nonempty statement.

    println (
        x = 3
        x + 5
    )

This prints `8`. This code is ugly though: it assigns `x` in the middle of evaluating an expression, and `x` endures after the parenthesis finishes. For these situations-- where you want to do some calculation inside of a expression-- `{ }` is a group which acts exactly like `( )` except that a new scope is created inside of it. So you can say:

    x = 3
    println {
        x = 4
        x + 1
    }
    println x

And, well, honestly this is not great code either, but the `x` assigned inside the `{ }` will be a different `x` than the one outside the `{ }`. This code prints a `5` and then a `3`.

## "Currying"

You'll notice I am putting parenthesis after "println" a lot in these code samples, even though I said parenthesis are not needed. This is because function applications greedily always apply to whatever token is on the left, and this can mix you up:

    numbers = [ one = 1 ]
    println numbers.one

This code **looks** like it prints `1`, the values of `numbers.one`. In fact no, what it does is apply `println` to `numbers`, which returns `println`, which is then applied to `.one`. So it prints:

    <object>
    one

This isn't what you wanted, so we need the `( )`. Writing `( )` gets annoying all the time, I think, so you can write `:` as a shortcut:

    println: numbers.one

`:` wraps the remainder of the statement after it in parenthesis, so this is like saying `println (numbers.one)`.

Why require the `( )` (or the `:`) in the first place, though? Well, some of this syntax might change in a later version. But the idea is that Emily expressions are meant to be "curried". Emily functions are all single-argument. Wait, wait, you're saying, but I need functions that take multiple arguments. Well, you can build multiple-argument functions on top of single-argument ones. One way is to pass the argument list as an object:

    printRecord ^record = \
        print (record.name) ": " (record.numberOfArms) " arms\n"

    printRecord [name = "Sarah"; numberOfArms = 3]

(`print` is just like `println`, but doesn't add a newline at the end.)

Another approach is to sort of fold functions inside each other. Emily has a special syntax for this:

    describe ^name ^species ^arms = \
        print name " is a " species " with " arms " arms\n"

    describe "Natalie" "centaur" 2

So, you're probably thinking, "describe" is a function with 3 arguments, and you apply the function by writing the arguments one after the other? And sure, you can think of it that way, but from the language's perspective what's happening is that each `^` is creating a new function that returns a function inside of it. Remember how `print` is basically a machine that you feed one thing into, and it spits out a copy of itself for you to feed the next thing into? This is why I'm able to write those long chained `print` statements like in the last example. Well, `describe` is like a machine that spits out another machine that spits out **another** machine that spits out a description of your friend Natalie. Once you're thinking about it this way, you can do something neat: You can take one of these intermediate machines and make a copy.

    natalieGenerator = describe "Natalie" "centaur"
    natalieGenerator 3
    natalieGenerator 4
    natalieGenerator 5

I'm not really sure what's going on with Natalie there, maybe she should see a doctor. But, this code prints three separate lines about an increasingly multi-limbed centaur named Natalie:

    Natalie is a centaur with 3. arms
    Natalie is a centaur with 4. arms
    Natalie is a centaur with 5. arms

By leaving out the final argument when we made `natalieGenerator`, we got a "partial application" function which is still just sitting there, waiting, for its last argument so it can run.

## Flow control

The "currying" trick is a little academic, but it's an example of what kinds of things we can do once we start thinking of functions as values. What this is leading up to is "higher-order functions", which are a bit more practical, since they're are how you do flow control in Emily:

    x = 3
    if (x < 4) ^(
        print x " is less than 4\n"
    )

In most languages, things like `if` or `while` are magic-- `if` in C snarfs up "something surrounded by a parenthesis" and "something surrounded by curly braces" and then decides whether to execute the curly brace code or not. There's no magic in Emily. There's just functions. `if` is a function. It takes an argument, checks if it's true (meaning: not `null`), and if it's true it executes its next argument as a function.

You might be saying now: I don't **care** about all this functional programming currying stuff! I just want to write an if statement. And that's fine, you shouldn't have to get wrapped up in the theory. What you do need to know is that things like `if` and `while` are not magic, and any statement gets executed exactly when and where you wrote it unless there's that `^` to say "not yet". So if you're using `if` or `while`, you **do** need to put in a `^`. Oh, right, there's also a `while`:

    # Print the numbers 1 2 3 4
    x = 1
    while ^(x < 5) ^( println x ; x = x + 1 )

`while` needs `^` on both of its parentheticals-- the `(x < 5)` gets run again and again at each pass of the loop, so it needs that "not yet".

By the way, you don't **need** to use `while` to get a loop, because there's recursion:

    countdown ^x = (
        println x
        x > 0 ? countdown (x - 1) : println "blastoff"
    )

    # Print: 5 4 3 2 1 blastoff
    countdown 5

Emily uses a trick from functional programming that isn't worth explaining here called "tail recursion", which keeps the stack from overflowing if you end a function with a recursive call.

## Loading files

If you write more than one program in Emily, you might find there are functions or definitions you want in every program. You can share a piece of code between programs by putting it in a "package". A package is just a file or directory on disk. You can load packages out of your current directory by using the `directory` object. If you make these two files:

* `includeMe.em`:

        name = "test"

* `executeMe.em`:

        println: directory.includeMe.name

Running `executeMe.em` will print out "`test`".

For more information on working with packages, see "About packages" in [manual.md](manual.md).

## Prototypes

One more concept I want to throw at you, and then I'll try to explain why this stuff works the way it does.

So Emily has "objects". Can it do object-oriented programming? The answer is yes-- if you assign a function to a field inside of an object declaration, it becomes a "method" and gains access to special `this` and `super` variables:

    apple = [
        color = "red"
        describe ^ = print "It is " (this.color) ln    # ln is shorthand for "\n"
    ]

    do: apple.describe

This prints "`It is red`". You'll notice I did something new here-- I defined `describe` as a function with no arguments. If you do this, it actually becomes a one-argument function that throws its argument away. You can then execute the function later by passing it any value you like, or passing it to `do` (a builtin which applies a function to the argument `null`).

Emily objects have inheritance, although they do not have classes. Instead, they use "prototypes", which is just a fancy way of saying that objects can inherit from other objects.

    fruit = [
        describe ^ = print "It is " (this.color) ln
    ]

    lime = [
        parent = fruit
        color = "green"
    ]

    do: lime.describe

This prints "`It is green`". What happens here is that `lime` does not have a `describe` field, but it does have a `parent`, and the field named `parent` is special. When you check `lime.describe`, if it does not find `.describe`, it checks the parent, and returns it from there. Because `describe` is not just a normal function but a method, when you call `lime.describe` the interpreter knows to set `this` to be equal to `lime`, not `fruit`. (The `fruit` base prototype doesn't even have a color, so if you say `do: fruit.describe`, you'll just get an error.) `fruit` here is just another object, but it's **acting like** a class.

If an object needs to send a message to its parent, it should use the special `super` variable to do that:

    banana = [
        parent = fruit
        color = yellow
        describe ^ = (
            do: super.describe
            println "It has seeds"
        )
    ]

    plantain = [
        parent = banana
        color = beige
        describe ^ = (
            do: super.describe
            println "It's crispy, like it's been fried"
        )
    ]

    do: plantain.describe

This prints

    It is beige
    It has seeds
    It's crispy, like it's been fried

Each call to `super` calls the function in the parent. Wait, couldn't you have also just said `do: this.parent.describe`, instead of using `super`? Well, that's legal, but then the special-ness that makes `this` work would break; if you'd said `this.parent.describe`, you'd have gotten a function that was a method of `banana`, not a method of `plantain`, and the describe line would have said "It is yellow". `super` makes sure the special method rewiring still works.

## Array builtins

One last thing, real quick: If you don't set a `parent` on an object, it still has a parent. There's a universal default parent for objects. At the moment, all this universal parent contains is `append` and `each`. These are methods that let you treat objects like arrays:

    array = []
    array.append "one"
    array.append "two"
    array.append "three"
    println (array.count) (array 0)
    array.each print

This prints:

    3
    one
    onetwothree

Calling `append` on an object looks up the numeric value of the `count` field (assuming `0` if this is the first time something's been appended to the object), creates a new field with the previous item count as the key and `append`'s argument as the value, and then increments the value of the `count` field. `each`, meanwhile, takes a function as argument, and applies it to all numeric keys from `0` up to `count`.

# So what is this good for?

Up until now, I've been describing to you a language which looks like a lot of languages you've seen before, with a few quirks. Why these quirks? Why use this language?

Emily is **simpler** than other programming languages, in certain ways. My goal in designing it was to reduce the number of basic concepts that a programming language is built on. Fewer concepts winds up meaning that the concepts that are left are more versatile, and you can use this versatility in some really neat ways.

## Everything is a function

Something you might have noticed: Passing an argument to a function, and looking up a key on an object, are both done by writing one after the other. What is the difference between a function and an object?

The answer is: Objects **are** functions. A function in Emily is a map from one value to another (which maybe has some sort of side-effect when it evaluates). Objects are one way of mapping things, closures (code functions made with `^`) are another, but the language doesn't know the difference between them and doesn't care. You can interchange objects and functions. For example an object can inherit from a function:

    sensor = [
        parent = println
    ]

    sensor.testValue

This code prints `testValue`, because when the language doesn't find `.testValue` in `sensor` it looks `.testValue` up in `sensor`'s parent, which happens to be the println function. This is not an example of something you would actually ever do. But...

## Everything you do is a function call

When I say everything is a function call, I do mean everything. Operators, like `=` or `+`, are actually function calls in disguise: `a + b` is code for `a .plus b`, and `a = b` is code for `(SCOPE).let .a b` (this will probably be named `set` in the next version). Objects like `[]` have that `.let` field built in when you make them, objects like `3` (numbers are objects) have a `.plus` field built in when you make them. Look at what happens if I replace `let` for a particular object:

    sensor = [
        secrets = []              # This is the real object

        # Catch attempts to read a field...
        parent ^key = (
            print "Read key " key ln               # Gossip
            this.secrets key      # Then forward to secrets
        )

        # Catch attempts to write a field...
        let ^key ^value = (
            print "Set key "  key " to " value ln
            this.secrets key = value   # Forward to secrets
        )
    ]

    sensor.testValue = 5 + 10

    println: sensor.testValue

`sensor` acts like any other object, but it gossips about everything you do to it. The above code prints:

    Set key testValue to 15.
    Read key testValue
    15.

It's not unusual to find a language which allows attribute getters and setters, but using them here is unusually straightforward because you're using the normal language machinery, not some kind of special construct.

## Make your own flow control

I mentioned earlier that `if` and `while` are "higher-order functions". This means having to write `^`s a bunch (at least until I figure out a better syntax...), but there's a distinct advantage to this: It means writing your own flow control operators is easy. Perl and Ruby have an `until` in addition to `while`; if an Emily user wanted an `until`, they could add it by writing one function:

    until ^condition ^block = \
        while ^(!( do condition )) block

    x = 5
    until ^(x < 0) ^(println x; x = x - 1)

Or if we wanted to be all fancypants functional programmer, we could do this with currying:

    compose ^a ^b ^c = a (b c)
    until = compose while (compose not)

A thing to notice: `until` is a function, but we constructed it without actually using a `^`. If functions are values, it starts to feel attractive to make functions by performing operations on those values rather than writing out the code. (Although maybe taking it **this** far is a little pointless.)

## Currying objects

If we're thinking of objects as functions, we can think of field accesses as similar to currying-- `x.functionName argument` applies a method, and `x.functionName` is like partially applying that method.

    # Take the `each` method from an object,
    # and print each value with a filter function applied.
    printFiltered ^each ^function = each ^value(println: function value)

    # Make the list 1 2 3
    x = [ this.append 1; this.append 2; this.append 3 ]

    # Prints 4 5 6
    printFiltered (x.each) (3.plus)

The interesting thing here is that `x.each` and `3.plus` "remember" their targets; `3.plus` constructs a little standalone function that adds 3 to any number.

## What is Emily?

Emily is an attempt to get the power and flexibility of functional programming languages-- things like ML, with things like higher-order functions and user-defined operators-- while retaining the abilities and ease of use of a dynamic object-oriented language. It does this by modeling all operations as chains of function applications, approximating traditional programming language operators as macros that construct those applications, and adding some builtins that support OO style.
