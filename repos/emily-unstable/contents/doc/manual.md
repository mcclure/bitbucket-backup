**Emily programming language, version 0.3b**  
**Reference manual**

This is a reference for the Emily programming language. It lists all features but does not attempt to explain usage or concepts. If you want something that explains concepts, read [intro.md](intro.md).

Non-language-lawyers will likely want to skip to the "Syntax: Operator Precedence" section and read from there.

Table of contents:

[TOC]

# Execution model

## Premise

Emily is based on what I'm calling "c-expressions", for "curried expression"; this is a minimal way of writing code, modeled after the way prefix functions are executed in an ML language.

All values in Emily are modeled as unary functions-- in other words, a function which takes one argument and returns one argument. Because some of these values don't "behave" like functions once they've received that argument, this might be a little misleading; a better way to look at it may be that everything in Emily can be "applied", which means taking one value in and passing one value out. The way you apply two values together in code is to simply write them one after the other. If you write:

    a b

You are "applying" a to b. This expression evaluates to a value, which is the result of giving b as an argument to a.

All expressions evaluate to values, which means expressions can be chained. If you write:

    3 .plus 4

You will apply "3" to its argument ".plus". This will return a function that takes a number as argument and returns that integer plus 3. This function then is applied to its argument 4, returning the number 7.

Some functions cannot accept some categories of arguments. For example, a number like 3 can only take atoms (field names) such as `.plus` or `.minus` as arguments. If you write the expression `3 4` in code, at evaluation time 3 will attempt to take 4 as an argument, the evaluation will fail and the program will halt.

The special paired characters `( )`, `{ }` and `[ ]` in an expression create a "group" containing all tokens in between. In

    1 .plus (2 .plus 3)

The value `1 .plus` is being applied to the value `(2 .plus 3)`.

In code samples or examples below, you may see things that look like "operators" as you know them from other languages, things like `=` or `+`. These are technically fake. Before a c-expression is interpreted, a set of "macros" are applied which remove all symbols. Each macro rewrites a valid c-expression containing a symbol to a valid c-expression without that symbol. Macros exist so that things like order of operations, or syntax "binding" to nearby tokens, can happen while still preserving the idea that at core everything is application of unary functions to arguments. The user generally should not have to think about this fine distinction, but should be aware that (1) any expression in Emily **could** be written as nothing but values and grouping parentheses, and (2) any operator (such as `+`) that works with a builtin **could** also work with a user-defined object that responds to the corresponding atom (for `+` this is `.plus`).

The c-expressions are homoiconic for the AST in the current Emily interpreter, if that means anything to you.

## Values

Right now, the following kinds of runtime values exist in Emily:

- `null`
- `true`
- Numbers
- Strings
- Atoms
- User closures
- User objects

In addition, there are builtin closures, "scope objects", and continuations (`return`); however, these are within the interpreter and a user cannot create these (except implicitly).

"Atoms" require explanation. These are strings, but are unequal to their string counterparts: `"add" != .add`. They are used for field names in objects; if an object responds to the string value `"add"`, it would be undesirable for this to possibly get confused with `.add` as invoked by the `+` operator.

"Numbers" are double-precision floats.

There is a `true` but is no "false"; `null` is used as the "false" value. Library functions which must evaluate the truth values of expressions, like `if`, treat any non-`null` value as true.

Remember: these different kinds of values only differ in terms of which arguments they will accept (without failing) when applied as functions.

## Token types

An Emily program is a series of statements ("lines of code" separated by ; or newline). A statement in an Emily program is a series of "tokens". In source code on disk, the following kinds of tokens exist:

- Words: An identifier-- any ASCII letter a-z A-Z, followed by any sequence of ASCII letters a-z A-Z and numbers 0-9. Underscore is not allowed.)
- Numbers: Any numbers 0-9, optionally followed by one period and more numbers 0-9.
- Strings: Open and close quotes " " and everything between them. Within the quotes, the escape characters `\n`, `\\`, and `\"` will be recognized.
- Unscoped groups: `(` ... `)`
- Scoped groups: `{` ... `}`
- Object-literal groups: `[` ... `]`
- Symbols: Any unicode character other than `#` `(` `)` `[` `]` `{` `}` `\` `;` `"` or whitespace.

Strings and groups may contain newlines within them.

After macro processing, all Symbols will be eliminated and two new types of tokens (created by macros) will appear in the processed statement which is executed. These are Atom tokens and Closure tokens. If there are any Symbols left by the time the macro processor is done, this is a failure and the program will terminate without running.

## Scopes

Each statement is executed in the context of a "scope". This is an invisible, inaccessible object that the interpreter knows about. Any word token, at evaluation time, will be converted to an atom token and the scope object will be applied to it. In other words, the expression

    a

By itself, is a field lookup on the scope object.

Inside of a group or a closure, the scope may be different from that of the surrounding code. The scope will not change over the course of an executed statement (although the contents of the scope may change).

Scopes in Emily are objects, which means they can have parents via object inheritance. If a scoped group encloses another scoped group in the code, the inner group's scope will have the outer group's scope as a parent.

## Sequencing

All Emily values are functions; this means they map one value to one value, and evaluating the map may optionally have a side effect. The order in which evaluations (and thus, potentially, side effects) occur is precise.

Consider a program as a list of statements. Statements are separated by semicolons or newlines. The statements are executed one after the other.

Consider a statement as a list of tokens. Execution works like this:
1. The token at the start of the list is popped off, evaluated, and becomes Value 1.
2. If there are no more tokens, Value 1 is the value of the statement.
3. If there are more tokens, the token at the start of the list is popped off, evaluated, and becomes Value 2.
4. Value 1 is applied to Value 2 as a function; the result becomes the new Value 1.
5. Return to step 2 and repeat.

An "empty" statement (i.e., two semicolons or two newlines with only whitespace between them) is ignored by the interpreter.

"Evaluating" a token means:
1. For a word token: Lookup on current scope object (see section on scopes above).
2. For a group token: All statements in the group are immediately executed in order. The value the group token evaluates to depends on the type of group (see below).
3. For a closure token: A new closure value is created, bound to the current scope object (see section on closures below). The evaluation will not have side-effects.
4. For any other kind of token: The token becomes a value in "the obvious way" (The token 3 becomes the number 3, etc). The evaluation will not have side-effects.

# Syntax

## Order of evaluation

This is only important to know if you are writing macros (not possible in Emily 0.1). If you are writing code, you probably actually want the "operator precedence" table in the section below.

In the table below, operators higher in the table are evaluated first. If a phase is marked LTR in the Order column, symbols in that phase are evaluated leftmost-token-first. If a phase is marked RTL, they are evaluated rightmost-first.

    Phase   | Order | Symbol
    --------|-------|----------
    Reader  | LTR   | \
            |       | \version
            |       | #
            |       | ;
            |       | ( )
            |       | { }
            |       | [ ]
            |       | " "
    110     | LTR   | .
    105     | LTR   | =
    100     | LTR   | ^
            |       | ^@
    90      | LTR   | ? :
            |       | :
    77      | RTL   | %%
    75      | RTL   | ||
    70      | RTL   | &&
    65      | RTL   | !=
            |       | ==
    60      | RTL   | >=
            |       | >
            |       | <=
            |       | <
    50      | RTL   | +
            |       | -
    40      | RTL   | *
            |       | /
    30      | RTL   | ~
            |       | !
    20      | RTL   | `

(This table can be different from conventional "precedence" because all the macros do different things when they execute-- for example the `.` macro adheres directly to the item to its right, whereas the `+` macro creates groups out of its entire left and right sides and adheres directly to neither.)

## Operator precedence

If you just ignore all this "macro" stuff and consider the symbols in Emily as normal operators, here's what another language would call "precedence" and "associativity" for those operators.

In the table below, higher items are "higher precedence" (bind tighter). Left-associative operators "bind to the left" (prefer to group left in the absence of clarifying parenthesis).

    Precedence | Associativity | Operator
    -----------|---------------|-------------
    1          | Right         | \version
    2          | Right         | .
    3          | Right         | ^
               |               | ^@
    4          | Left          | ~
               |               | !
    5          | Left          | *
               |               | /
    6          | Left          | +
               |               | -
    7          | Left          | >=
               |               | >
               |               | <=
               |               | <
    8          | Left          | !=
               |               | ==
    9          | Left          | &&
    10         | Left          | ||
    11         | Left          | %%
    12         | Right         | ? :
    13         | Right         | =
    14         | Left          | `

## Operators

### \

This stitches lines together. If a line "ends with" a `\` (nothing may appear after the `\` except `\`, `\version`, comments or whitespace) the following line will be considered part of the same statement.

If a `\` is followed by anything else, or by an end-of-file, this is a failure and the program will terminate without running.

### \version

**Usage:** `\version` *[version number]*

This is a reader instruction (right now, the only one) which reports the interface version of Emily that the program was written against. In this version of Emily, if anything at all is written here other than the literal string `\version 0.1` or `\version 0.2`, this is a failure and the program will terminate without running.

In the long term, for **all future versions of Emily**, the plan is that when the Emily interpreter sees `\version *[number]*`, it will:

- If the host version of Emily is backward compatible with *[number]*, it will do nothing and run the program.
- If the host version of Emily has backward-incompatible changes, it will attempt to run the program in a compatibility mode.
- If this is not possible, or the version number is not recognized (for example it is a future version of the language), this is a failure and the program will terminate without running.

Ideally a program should always start with `\version`, and should select the oldest interface version which can run the code.

*Note: The contract here is not yet being totally correctly followed. Emily 0.2 treats the 0.1 interface as something it is fully backward compatible with. However, this is not really accurate-- it is possible to write a valid 0.1 program which behaves differently in 0.2. In particular imagine what happens if you create a local variable named "private" and then try to access it from inside of an object literal. I figure this is "close enough for now" but in future versions of Emily the compatibility handling will need to be more precise.*

### # 

Comment. Everything from the # until a newline is ignored.

### ;

Statement separator. So is non-escaped newline.

### ( )

**Usage:** `(` *[any number of statements]* `)`

Unscoped group. Executes all statements between the parenthesis, in the context of the enclosing statement's scope. The group evaluates to the value of the final nonempty statement.

`()` with nothing in between evaluates to `null`.

### { }

**Usage:** `{` *[any number of statements]* `}`

Scoped group. Creates a new scope object which is a child of the enclosing statement's scope (new `has`, `set` and `let`). Executes all statements between the braces in the context of this new scope.

As with unscoped groups, the group evaluates to the value of the final nonempty statement and `{}` with nothing in between evaluates to `null`.

### [ ]

**Usage:** `[` *[any number of statements]* `]`

Object-literal group. Creates a new user object, and executes the statements in a specially prepared scope designed for convenient object construction. For more detail see the section on objects below.

### " "

**Usage:** `"` *[string contents]* `"`

See "values" above.

### .

**Usage:** `.` *[word]*

Atom constructor. Captures the token directly to the right of the `.`; if it is a valid word, this evaluates to the atom for that word. Otherwise this is a failure and the program will terminate without running.

### ^

**Usage:** `^` *[optional: any number of words]* *[group]*

Closure literal. Captures to the right zero or more word tokens, and one group token. If non-word tokens are found before the group, or the group is not found, this is a failure and the program will terminate without running. Evaluates to a user closure which has: the given words as bound arguments; the enclosing statement's scope as context scope; and the given group as its code.

Redundant ^s are allowed-- `^a b (a)` does the same thing as `^a ^b (a)`. Imagine the currying is actually multiple wrapped closures, if you like.

For more information on closure values, see "About functions -> user closures" below.

### ^@

**Usage:** `^@` *[optional: any number of words]* *[group]*

Exactly the same as `^`, but a "return" will be created in the execution scope as if a new function were being defined with `=`.

### :

Apply right: Captures the entire rest of the statement to the right of the `:`, and replaces the tokens with one unscoped group containing the captured tokens.

*Treated as a macro:*

    a b c: d e f

*...becomes*

    a b c (d e f)

### ? :

**Usage:** *[statement condition]* `?` *[statement 1]* `:` *[statement 2]*

Ternary operator. Captures the entire statement and splits it in three. At run time, *[statement condition]* is evaluated. If it is true (non-`null`), *[statement 1]* will be evaluated and its value returned; otherwise, *[statement 2]* will be evaluated and its value returned. *[statement 1]* cannot contain a question mark as a token, *[statement 2]* can. (To be explicit: `a?(b?c:d):e` is allowed, `a?b:c?d:e` is allowed, `a?b?c:d:e` is not.) Any of the three statements may be blank, in which case the value for that statement will be null.

*Treated as a macro:*

    a b c ? d e f : g h i

*...becomes*

    tern (a b c) ^(d e f) ^(g h i)

### ||

**Usage:** *[statement 1]* `||` *[statement 2]*

Boolean OR. *[statement 1]* will be evaluated. If it is true (not `null`), its value will be returned. If it is false (`null`), *[statement 2]* will be evaluated and its value returned.

### &&

**Usage:** *[statement 1]* `&&` *[statement 2]*

Boolean AND. *[statement 1]* will be evaluated. If it is false (`null`), `null` will be returned. If it is true (not `null`), then *[statement 2]* will be evaluated and its value returned.

### %%

**Usage:** *[statement 1]* `%%` *[statement 2]*

Boolean XOR. *[statement 1]* and *[statement 2]* will be evaluated. If both are true (not `null`) or both are false (`null`), `null` will be returned. If only *[statement 1]* is true, its value will be returned. If only *[statement 2]* is true, its value will be returned.

### ~

**Usage:** `~` *[token]*

Negation. Captures one token to the right and evaluates to that token with `.negate` as argument.

*Treated as a macro:*

    a b ~ c d e

*...becomes*

    a b (c.negate) d e

### !

**Usage:** `!` *[token]*

"Not". Captures one token to the right and evaluates to the application of the `not` function (see below) to that token.

*Treated as a macro:*

    a b ! c d e

*...becomes*

    a b (not c) d e

### `

**Usage:** `` ` `` *[token]* *[token]*

Backtick is the "apply pair" operator. It captures two tokens to the right, and replaces the tokens with an unscoped group containing the tokens. Useful for performing a quick application in the middle of a long expression (remember, `cos a.b` in this language is evaluated like `((cos a) b)`, not `(cos (a (b)))`).

*Treated as a macro:*

    a b `c d e f

*...becomes*

    a b (c d) e f

### =

**Usage examples:**

    a = 3
    a.b = 3
    a ^ x = x + 1
    a.b.c (3) ^ x y = x + y
    nonlocal a = 3

`=` is the most complicated operator, and its behavior is based on trying to do "what you expect". It defines a new key on some object somewhere and sets its value. If a single token appears to the left of the `=`, this is a key on the scope object. If a multi-token expression is to the left of the `=`, the rightmost token is used as a key on the expression defined by the other tokens.

If a `^` appears to the left of the `=`, all tokens to the right of `^` and the left of `=` are argument bindings for a closure, and the tokens to the right of `=` are the code group for the closure. A closure created this way will have a `return`. In other words, `x ^ = 3` is equivalent to `x = ^@ (3)` and `x ^b c = 4` is equivalent to `x = ^@ b c (4)`. Like with normal `^`, multiple `^`s are allowed.

If the first token on the left is `nonlocal`, this is a special flag to `=` that rather than defining a new key, it should set an existing key, setting the key on a parent if necessary, and failing if the existing key is not found. This for example makes it possible to set a variable in a parent scope, similar to the `nonlocal` directive in Python.

`=` can currently set fields on objects, but can't implicitly make new objects; `a.b = 4` is only legal if `a` is already an existing table. `a = []; a.b = 4` is legal.

`=` cannot set a non-word key on a scope object. In other words, `4 = 5` is an error.

*Treated as a macro*, `=` is actually identifying a key (the rightmost token to the left of the `=`, or, if present, the `^`) and a target (everything to the left of the key), and invoking *[target]* `.let` *[key]*. If the `nonlocal` modifier is present, it uses `.set` instead of `.let`. See "Assignments" below.

## Splitter operators

The remaining operators are all of a particular type; for brevity, a single explanation will be given here.

Each of these operators is a "splitter" for some designated atom: it captures the entire line, splits it into two statements, and at run time evaluates to the entire left-side statement, applied to the designated atom, applied to the entire right-side statement.

Take `+` as an example; its designated atom is `.plus`. Treated as a macro,

    a b c + d e f

*...becomes*

    (a b c) .plus (d e f)

The familiar "order of operations" emerges from the grouping caused by all this splitting.

What the application of `.plus`, or any other atom, means in practice will depend on the type returned by the left-side statement.

### ==

**Usage:** *[statement 1]* `==` *[statement 2]*

Splitter for `.eq`. Intended for equality test.

### !=

**Usage:** *[statement 1]* `!=` *[statement 2]*

"Not Equal". Not actually a splitter; splits for `.eq`, then NOTs the entire thing.

In other words, *treated as a macro:*

    a b c != d e f

*...becomes*

    not ( (a b c) .eq (d e f) )

### >=

**Usage:** *[statement 1]* `>=` *[statement 2]*

Splitter for `.gte`. Intended for "greater than or equal to" test.

### >

**Usage:** *[statement 1]* `>` *[statement 2]*

Splitter for `.gt`. Intended for "greater than" test.

### <=

**Usage:** *[statement 1]* `<=` *[statement 2]*

Splitter for `.lte`. Intended for "less than or equal to" test.

### <

**Usage:** *[statement 1]* `<` *[statement 2]*

Splitter for `.lt`. Intended for "less than" test.

### +

**Usage:** *[statement 1]* `+` *[statement 2]*

Splitter for `.plus`. Intended for mathematical addition or something like it.

### -

**Usage:** *[statement 1]* `-` *[statement 2]*

Splitter for `.minus`. Intended for mathematical subtraction or something like it.

### *

**Usage:** *[statement 1]* `*` *[statement 2]*

Splitter for `.times`. Intended for mathematical multiplication or something like it.

### /

**Usage:** *[statement 1]* `/` *[statement 2]*

Splitter for `.divide`. Intended for mathematical division or something like it.

### %

**Usage:** *[statement 1]* `/` *[statement 2]*

Splitter for `.mod`. Intended for mathematical modulus or something like it. See `mod` under Number for behavior details.

# Builtins

A set of builtin functions and other values are available on the various built-in types.

In each of the following sections, the documented fields are in the base prototype for the section's type. The value `3` has "integer" as its base prototype, so any methods in the integer base prototype will be usable on `3`. For an explanation of prototypes, see "About objects" below.

Remember that for scopes, a field lookup is done by just writing a word, like `not`; for other kind of objects, the field lookup requires a `.`, like `3.negate`.

Note that there is nothing to stop the user from shadowing builtins in the current scope with local variables, and that if the user does this then some macros may break. For example `!` cannot work if `not` is shadowed, `? :` cannot work if `tern` is shadowed, etc. Future updates will contain features to prevent such accidental breakage.

## Common

### All objects

Note: Even `null` responds to these.

#### has

**Usage:** `has` *[arg]*

**Or:** *[object]* `.has` *[arg]*

Takes a value and returns `true` if the target is able to properly handle the value and `null` if it is not (i.e., the result is `null` if applying the target against *[arg]* will result in an error).

#### eq

**Usage:** `eq` *[arg]*

**Or:** *[object]* `.eq` *[arg]*

If *[arg]* is equal to the target, returns `true`. Otherwise returns `null`.

### Scopes and objects

These methods are present in common for scopes and user objects (things that act as containers).

#### set, let

See "About objects: Assignments" below.

## Scope

Remember the idea is that these are the fields inherited by all scope objects. In other words, these are "globals".

### null

Null value.

### true

True value.

### print

**Usage:** `print` *[arg]*

Takes any argument, prints it to stdout as a string, then returns `print` again (so it can be chained).

*Example:* `print 1 2 3` (prints "123")

### println

**Usage:** `println` *[arg]*

Takes any argument, prints it to stdout as a string followed by a newline, then returns `println` again (so it can be chained).

*Example:* `println 1 2 3` (prints 1, 2 and 3 separated by newlines)

Note: The behavior of `print` and `println` with regard to buffer flushing is currently undefined, but incidentally, the current 0.1 interpreter flushes every time it prints something with `println` and never flushes for `print`.

### `ln`

Value equal to "\n".

### `sp`

Value equal to " " (one space).

### do

**Usage:** `do` *[arg]*

Performs `arg null` and returns the result. Useful for executing a zero-argument closure.

*Example:* `do ^( println 3 )`

### loop

**Usage:** `loop` *[arg]*

Takes a value assumed to be a zero-argument closure, executes it by passing null as an argument, if the return value is true (non-null) executes it again, if that return value is true executes it again, if that...

### if

**Usage:** `if` *[arg 1]* *[arg 2]*

*[arg 2]* is assumed to be a zero-argument closure, *[arg 1]* is anything.

If *[arg 1]* is true (not `null`), executes *[arg 2]* by passing `null` as an argument.

### while

**Usage:** `while` *[arg 1]* *[arg 2]*

Takes two arguments, both assumed to be zero-argument closures. Repeatedly executes *[arg 1]* by passing null as an argument, if the result is false (null) stops looping, if the result is true (not null) executes *[arg 2]* and repeats.

### not

**Usage:** `not` *[arg]*

Takes one argument. If the argument is `null`, returns `true`. If the argument is anything else, returns `null`.

### and

**Usage:** `and` *[arg 1]* *[arg 2]*

Slightly arcane. You probably actually want to use `&&`, which this is the underlying implementation for.

Takes two arguments, both closures. If the evaluated value of *[arg 1]* is true (not `null`) returns the evaluated value of *[arg 2]*. If *[arg 2]* is false (`null`) returns `null` without evaluating *[arg 2]*.

### or

**Usage:** `or` *[arg 1]* *[arg 2]*

Slightly arcane. You probably actually want to use `&&`, which this is the underlying implementation for.

Takes two arguments, both closures. If the evaluated value of *[arg 1]* is true (not `null`), returns it without evaluating *[arg 2]*. Otherwise evaluates and returns the valeu of *[arg 2]*.

### xor

**Usage:** `xor` *[arg 1]* *[arg 2]*

Slightly arcane. You probably actually want to use `%%`, which this is the underlying implementation for.

Takes two arguments, both closures, and evaluates both. If only the evaluated value of *[arg 1]* is true (not `null`), returns it. If only the evaluated value of *[arg 2]* is true (not `null`), returns it. If neither or both evaluated values are true, returns `null`.

### nullfn

**Usage:** `nullfn` *[arg]*

Slightly arcane. Takes an argument, ignores it, and returns `null`.

### tern

**Usage:** `tern` *[arg 1]* *[arg 2]* *[arg 3]*

Slightly arcane. This is the underlying implementation for `?:`. If *[arg 1]* is true (not `null`) this executes *[arg 2]* by passing `null` as an argument, otherwise executes *[arg 3]* by passing `null` as an argument.

### thisTransplant, thisInit, thisFreeze, thisUpdate

See "Manual control of 'this' and 'current'" below.

### package, project, directory

See "About packages" below.

## Object

In other words, user objects, created with `[` ... `]`.

### append

**Usage:** *[object]* `.append` *[arg]*

Allows an object to be used as a numeric-index array. For object `o`, `o.append x` will check `o.count` (assuming `0` if there is no such field), set the value *[arg]* for the key `o.count`, then set the value `o.count + 1` for the key `.count`.

### each

**Usage:** *[object]* `.each` *[arg]*

Iterates over an object which has been used as a numeric-index array. Starting with *n*=0, applies *[arg]* to the value of each consecutive field *n* that the object possesses (testing with `.has`) until one is not present, then stops. Returns `null`.

## Number

For all number methods taking arguments, the argument must be another number. If it is not, this is a failure and the program halts on evaluation.

### negate

**Usage:** *[number]* `.negate`

Returns *[number]* times -1.

### add

**Usage:** *[number]* `.add` *[arg]*

Adds *[number]* to *[arg]* and returns the result.

### minus

**Usage:** *[number]* `.minus` *[arg]*

Subtracts *[arg]* from *[number]* and returns the result.

### times

**Usage:** *[number]* `.times` *[arg]*

Multiples *[number]* by *[arg]* and returns the result.

### divide

**Usage:** *[number]* `.divide` *[arg]*

Divides *[number]* by *[arg]* and returns the result.

### mod

**Usage:** *[number]* `.mod` *[arg]*

Performs the modulus of *[number]* by *[arg]* and returns the result.

When *[number]* and *[arg]* have differing signs, modulus has "floored division" or "sign-of-divisor" semantics; in other words, it works like `%` in Python, not like C99 fmod.

To be explicit:

     11 %  10 ==  1
    ~11 % ~10 == ~1
    ~11 %  10 ==  9
     11 % ~10 == ~9

### lt

**Usage:** *[number]* `.lt` *[arg]*

If *[number]* is less than *[arg]*, returns `true`. Otherwise returns `null`.

### lte

**Usage:** *[number]* `.lte` *[arg]*

If *[number]* is less than or equal to *[arg]*, returns `true`. Otherwise returns `null`.

### gt

**Usage:** *[number]* `.gt` *[arg]*

If *[number]* is greater than *[arg]*, returns `true`. Otherwise returns `null`.

### gte

**Usage:** *[number]* `.gte` *[arg]*

If *[number]* is greater than or equal to *[arg]*, returns `true`. Otherwise returns `null`.

# About user closures

Everything in Emily "is a function", so what we usually think of as "a function"-- a block of reusable code you define-- in Emily is just one particular kind of function, called a "closure". Closures are created with the ^ operator; if you see a ^, you know you're making a closure.

Closures primarily consist of a group (`()`, `{}` or `[]`) full of code, but they also carry around several other pieces of information:

- A list of argument names.
- The scope in which the closure was defined.
- Whether or not it will create a `return`.
- Whether and how to set the object context variables (`current`, `this` and `super`). See "About objects" below.

These four things are determined by how and where the closure was created.

In order to execute a closure, it must be given as many arguments as it was originally defined with. The arguments are "curried", so if a function has two arguments, calling it with an argument returns a function which if called with an argument returns the closure's result. You can save the partially-applied function and reuse it.

If a closure has zero arguments, you still have to give it an argument to execute it, but the argument will be discarded. A handy value for this purpose is `null`-- since `()` is shorthand for `null`, the familiar-looking syntax `object.method()` will work. You can also use `do` as defined above.

When a closure executes, its code executes in a special scope with the enclosing scope as parent; this new scope is pre-populated with whichever of `return`, `current`, `this` and `super` are appropriate, then with all arguments. The behavior of assignment-- in other words, `set`, `let` and `=`-- is determined by the kind of group. In the case of an unscoped group (`^()`) they will "fall through" and occur directly in the enclosing scope. In the case of a scoped group (`^{}`) they will exist in a new scope, created just for the function application, which is a child of the enclosing scope. (`^[]` is not allowed.)

TODO: Explain the behavior of set and let on an argument variable.

## return

`return` is populated into the scope each time a function executes. It behaves the same way as `return` in other languages; putting a value after "return" causes the function to immediately terminate and its call site to evaluate to the returned value. There is one way `return` differs from other languages: `return` is not a keyword, but rather is just a name given to a function, a special function called a "continuation" which when called causes program execution to jump somewhere else. Because `return` is thus a value like any other, the value of `return` can be stored in a variable and can even outlive the function call itself. This can lead to a sort of time travel:

    savedReturn = null
    value = do ^(
        savedReturn = return

        return 5
    )
    println value
    value > 0 ? savedReturn (value - 1) : println "done"

Continuations are basically the functional-programming equivalent of GOTO.

# About objects

Functions are maps from value to value. A closure, in Emily, is a function which is expressed as a series of lines of code, and the mapping is defined by that value. A special kind of function is the object, which another language might call a dictionary, which is a map from a finite set of keys to a finite set of values. A lookup on an object is generally non-mutating.

Object creation is done with a group wrapped by the `[` ... `]` symbols. Like `{ }`, `[ ]` creates a new scope. The difference is that `{ }` returns the value from the final line and discards its scope, whereas `[ ]` discards its final line and returns the scope. In other words, by making assignments inside a `[ ]` you are building up an object, and then at the end of the `[ ]` this object is returned. The scope inside of an object literal also has a `private`; see "Private" under "About packages" below.

Objects have a "parent". This is what some languages call a "prototype". If a key is requested on an object, and the object does not have a value for that key, it will check the parent (which is literally the field `parent`). If the parent doesn't have it, it will then check *its* parent, and so on. If an object is reached in this chain with no parent and the key still has not been matched, this is a failure and the program will halt.

Because objects are interchangeable with other functions, the parent can be a a closure. When invoked, the `.parent` closure will be treated like any other closure (e.g. `this` and `current` will be correct).

## Assignments

There are two ways to set values on an object: `let` and `set`. **You should not directly use `.set` and `.let`.** You should use `=` (which is a shorthand for `let`) and `nonlocal` `=` (which is a shorthand for `set`; see `=` above). However, if you are in a situation in which you need to call the functions directly, here is how they work:

`.let` will directly set a key, creating a new key->value pair if necessary.

`.set` will set a key which already exists; if it does not exist, it will attempt to set it on the parent. (If it gets all the way to the end of the prototype chain and has not found a object which has a match for that key, this is a failure and the program will halt.)

`let` and `set` take two curried arguments, a key and a value. To use `let` to set the variable `a` to 3, for example, this would be `let .a 3` for a scope variable or `object.let .a 3` for an object field.

## Object-oriented programming

There is one more wrinkle.

In languages where objects can act as "containers" for functions, we often want to have those functions be "methods" on the object-- aware of and able to operate on the object they were fetched from. In Emily, if a function is defined "inside" of an object (if it is assigned inside of the object literal group) it becomes a "method" and when it is executed it will have three special variables defined in its scope: `this`, `current` and `super`. `this` is the object that the method was invoked on. `current` is the object that the method was originally defined as part of; it can be different if the method was inherited from a parent object. (You will usually want to use `this` and ignore `current`.) A method invoked on `super` will be fetched from a parent object, then executed with the `this` of the invoked object. So for example:

    object1 = [
        field = 1;
        method ^ = ( print (current.field) ", " (this.field) ln )
    ]
    object2 = [
        field = 2
        method ^ = ( print "Hello: "; do: super.method )
        parent = object1
    ]
    do: object2.method

This will print "Hello: 1 2". When it is run, `method` will be invoked on object2, `this` and `current` will be pointing to object2, and "super" will be pointing to object1. `object2.method` then invokes `super.method`, which causes `object1` to be invoked with `current`=`object1` and `this`=`object2`.

`this` and `current` are also visible inside object literals (Example: `[ this 3 = 4 ]` will work). `super` is not.

### Edge cases

Think about the method invocation `array3.append("x")`. Consider what happens if you say just `array3.append`. Persisting with the model that everything in this language is a function, partial `array3.append` application should be like a "curried" version of the operation `array3 .append("x")`. We should be able to save `array3.append`, and use it later, and saving it should not change its behavior-- if we store `array3.append` in a variable, or in an object, and later invoke it, it should still have its effect on `array3` and not some other object.

This intuitive idea is achieved by each closure being in one of four states (language lawyer alert on the following):

- The closure is created in "blank" state; it has no `this` or `current`, and if the closure is called those variables will not be set.
- If a "blank" closure is stored as part of an **object literal** definition-- that is, if it is assigned to a field between a `[` and `]`-- it will upgrade to "method" state. In "method" state, if it is pulled out of an object (either directly, or through the `.parent` chain), "this" and "current" will be set appropriately.
- If a "method" closure is ever stored in a **user object** at a time that is **not** defining an object literal, it will be moved to "frozen" state. The `this` and `current` from the moment the closure was frozen will be remembered and afterward not ever be updated.
- Similarly, if a "blank" closure is ever stored in a **user object**, it enters "never" state, which means that it is frozen in a state of never accepting a `this` or `current`. (Any `this` or `current` captured from the enclosing scope will of course be visible.)

Storing a blank/method closure in a scope object (in other words: in a variable) does not freeze it.

This is a bit complicated!, and it involves something nonobvious and slightly "magic" happening behind the scenes. I am interested in finding a simpler way to achieve these same goals in a later version of Emily.

### Manual control of "this" and "current"

A design goal of Emily is that anything the language interpreter can do, a third-party library can also do. To this end, some standard functions are available that can invoke functions with `this` and `current` set specifically. These are:

- `thisTransplant` *[closure]* - Resets ANY closure to "blank" state. If something that isn't a closure is passed in, it's returned unaltered.
- `thisFreeze` *[closure]* - If closure is "blank", converts to "never". If closure is "method", converts it to "frozen".
- `thisInit` *[object]* *[closure]* - If given a "blank" closure, puts it in "method" state with `current` and `this` equal to *[object]*. If given a "method" closure, converts it to "frozen".
- `thisUpdate` *[object]* *[closure]* - If given a "blank" closure, puts it in "method" state with `current` and `this` equal to *[object]*. If given a "method" closure, leaves `current` unaltered and updates `this` to *[object]*.

In other words, `thisInit` does the transform performed when assigning to an object definition; `thisFreeze` does the transform performed when assigning to an object at other times; and `thisUpdate` does the transform performed when invoking a method on `super` or implicitly fetching a method out of an object's `.parent`. It is not clear to me that these are useful.

# About packages

Packages are Emily code loaded from disk. They are accessed through the package "loaders" `package`, `project` and `directory` (see below). A package loader is an object which invisibly encapsulates an on-disk directory. Atom lookups on this object will query the directory; if a directory with the given atom name is found a loader for that directory will be returned, or if a source file with the atom name plus the `.em` extension is found that file will be loaded and executed "as a package" (see below) and its result returned. Notice an consequence of this is that you cannot use a loader to load any file whose filename contains characters diallowed in an Emily atom.

Loading code through a loader is lazy; if a single `.em` file is loaded multiple times from a loader, the interpreter will attempt to avoid executing it more than once. (This has not been well tested and might not always work properly if you have set custom loader paths-- in particular, setting the package and project dirs to the same directory will probably behave weird.)

## Executing code "as a package"

When code is executed through a loader, it runs in a scope very similar to that of an object literal. Code run inside the package file is "building an object" as it goes, and all assignments to top-level scope within the file are added to this new object. Basically, imagine that the toplevel scope is "returned" at the end of the file.

Similar to object literals, a `current` object is visible inside the package code; however, `this` is not. Also unlike object literals, variables assigned to within the package file are immediately visible without having to use `current`. To be totally explicit, this is legal in the toplevel scope of a package:

    x = 3     # Add to package scope
    println x # Read back from package scope

However, this is not legal code:

    obj = [
        x = 3     # Add to object
        println x # This reads from enclosing scope, not object.
    ]

## Private

When writing a package file, you might find that you need to use scratch variables which you don't want to be exported outside the package. For this purpose, you can use `private`. `private` is an object (it can be both written to and read from) the members of which will be visible to code in the package. If both private and non-private variables exist with the same name, the private variable will take precedence. For example, if these two files exist in the same directory:

File `includeMe.em`:

    private.x = "don't export me!"
    x = "export me!"
    print "Private variable says: " x ln

File `executeMe.em`:

    exported = directory.includeMe.x
    print "exported variable says: " exported ln

Then executing `executeMe.em` will print:

    Private variable says: don't export me!
    exported variable says: export me!

But this is confusing, so please don't do it.

`private` will not exist in source files that are being executed directly using the interpreter. It only exists in packages...

#### ...and object literals

`private` is also available inside of object literals, where it behaves the same way as it does in package code-- **including** the un-object-literal-like detail of any variables assigned to `private` being immediately visible. For example, this is valid:

    a = [
        private.x = 3
        y = x
    ]
    println: a.y

This will print "3".

## Loaders

* `package` loads the "standard library" packages from the Emily standard library. (Right now this is empty.)
* `project` loads packages from "your project"-- that is, the directory the interpreter believes to contain your program.
* `directory` loads from "the directory which contains the currently executing file".

Unlike the other two loaders, the search path for `directory` can vary from file to file; in the entry point for your program (the `.em` file, your `-e` string), `directory` and `project` will by default be the same, but for code executed using a loader they can differ.

If you run a `.em` file as a program using the Emily interpreter, the project directory will be by default set to the directory which contains that file. If you instruct the Emily interperter to run with `-e`, stdin input, or in interactive mode, the project directory will be the current working directory at the time the interpreter is invoked.

You can also explicitly set both the package and project directories when you invoke the Emily interpreter, either by using command line arguments (`--package-path` and `--project-path`) or using environment variables (`EMILY_PACKAGE_PATH` and `EMILY_PROJECT_PATH`). See the [manpage](manpage.1.md).

# Miscellania

Emily is garbage-collected.

The standard file extension for Emily source files is `.em`.

## Compiling Emily

Please see [build.md](build.md)

## Running Emily

Please see the [man page](manpage.md).
