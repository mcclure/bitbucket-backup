DESCRIBES ASPIRATIONS, NOT REALITY

Emily at the moment is a dynamic language; any variable or field can hold any value, and no correctness checking is performed before the program begins. I want types and type annotations. Types are nice. They enable powerful constructs like ML's `match` that leverage type information to infer intent, they help find errors, they can help enable optimizations. Those last two are unusually important in Emily becuase Emily is way more dynamic than even most dynamic languages. Even dynamic languages typically use syntax to infer certain kinds of information; Python knows, for example, that `3 + print` is nonsense without even trying to run it, Lua knows that `3 (4)` is not something you could write in a real program. But in Emily `+` is fake and anything can be applied to anything. The reason `3 (4)` is incorrect is because 3 cannot take the argument 4, not because there's something inherently wrong with trying to call 3 like a function. `3 (.plus)` is legal.

Emily should be gradually typed-- I really do like writing in the dynamic style sometimes, I just have other things I need types-- and the interpreter should make an attempt to infer types. What this means is you should be able to put type annotations on parts of your code where you want them (arguments to functions, variables, whatever), but leave off the types when you don't care. Whenever you put something no-known-typed into something known-typed the interpreter should do a runtime check, and fail. When the file is "compiled" (or in the interpreter, as a quick pass before the file runs) Emily should scan over the files, and see if it can figure out the no-type-annotation variables' actual types from how they are used; if it knows for a fact the type is right, it can remove the runtime safety checks, and if it knows for a fact a type somewhere is **wrong** (you fed `4` to `3` as an argument, you passed `"OK"` into a function which is typed as taking ints) it stops the program before it runs. There will be a lot of places it can't tell for sure either way, those become the runtime checks and maybe compile-time warnings.

So far, this is all pretty much standard for a language developed in the last ten years. Here's where things get a little more unusual:

In Emily, as I've said, everything is "the same thing", functions or objects or whatever you want to call them. Because there are closures and higher-order functions, I can construct functions at runtime. Because classes are just objects that act as prototypes, I can construct classes at runtime. I can interchange objects and functions. It therefore follows to me that I should be able to make types at runtime, and I should be able to interchange them with functions. A type is a set; I should be able to make a predicate that defines that set. In other words I should have some syntax like `^x [x > 3]` that creates a type "all variables x where `x > 3` is true". Similarly, since Emily has no "syntax" for just about anything (everything is a function application), type-annotating should also be some kind of function call (i.e. a runtime check, which potentially fails and that the optimizer knows how to remove when it's provably unnecessary). `int` is somehow interchangeable with a function that returns true if its argument is an integer, etc.

Functional languages offer something really cool called "pattern matching", where you can pass a value into a meat grinder that switches on either value or certain kinds of types. Like, this is legal OCaml code:

    let rec factorial = function
        | 0 -> 1
        | x -> x * factorial (x-1);;

In other words, the function result is "1 if the value is 0, multiply by factorial(x-1) if not". I want Emily to be able to do something more expressive:

    factorial = [
        (0) = 1
        ^x where x > 0 = x * (factorial: x - 1)
        ^x where x < 0 = fail "Positive numbers only"
    ]

That's awful code (no tail recursion! `:(`) but you get the idea. Here the "where x > 0" is basically just another key. This would mean I could guard up a function guard, or something like OCaml `match`, by just adding fields to an object-- and the objects still have a prototype chain, so I could build up a `match` one piece at a time by inheritance.
