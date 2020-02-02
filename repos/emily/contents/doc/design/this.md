## BEHAVIOR

`this` is probably one of the more mechanically complicated things in Emily, and veers dangerously close to "magic". `this` is the way it is because the semantics are chasing an intuitive notion of "how it should work".

Here's how I expect `this` should work:

1. If you define a function as a member of an object, then you expect on invoking it that `this` in the body refers to the object.
2. If you use `this` in the body of a function defined somewhere else, you expect it to refer to whatever `this` is in the enclosing scope.

Let's call 1 "methods" and 2 "functions".

3. If an object inherits a method from another object, then you expect on invoking it that `this` refers to the inheriting object.
4. If a method was invoked on a `super` object, then you expect that inside the method body `this` refers to the super-calling object.

5. If you store a either a "function" or a "method" in a variable or a dictionary-- something like, you say `x = object.method`, thus storing object.method to invoke later--, you expect it to have no effect on the `this` binding. You expect passing `.method` to `object` to act like currying.

7. It should be possible for `parent` to be set to a function rather than an object without fundamentally breaking anything above.

## IMPLEMENTATION

I implement this by each function secretly being in one of four states:

    | ThisBlank
    | ThisNever
    | CurrentThis of value*value
    | FrozenThis of value*value

Functions move between states at particular times:

1. Everything starts ThisBlank.
2. If a ThisBlank closure is assigned to a user object x inside a declaration, it becomes CurrentThis(x,x).
3. If a ThisBlank closure is assigned to a user object x at any other time, it becomes ThisNever.
4. If a CurrentThis(x,y) closure is assigned to a user object ever, at any time, it becomes FrozenThis(x,y).
5. When a CurrentThis(a,b) or FrozenThis(a,b) closure is executed, a `this` and a `super` is populated in the scope equal to b. "this" is equal to b. When a field is read from `super`, it is fetched from `a.parent` and if it is a CurrentThis(x,y) closure then that closure is converted to CurrentThis(x,b).
t6. If the interpreter attempts to read a field from a user object z, does not find it, and reads it from the object's parent, if the object is CurrentThis(x,y), then the closure is converted to CurrentThis(x,z).

Because user code should be able to do anything the interpreter can do, some user methods are available:

- calling `thisTransplant method` unconditionally resets method to ThisBlank.
- calling `thisInit newThis method` performs the same transform as assigning `method` to an object at definition time (set a CurrentThis if method is ThisBlank, otherwise freeze).
- calling `thisFreeze method` does the same transform of assigning `method` to an object at non-definition time (change to ThisNever or FrozenThis, as appropriate).
- calling `thisUpdate newThis method` performs the same transform as pulling method out of a `super` for `newThis` method (If method is CurrentThis(x,y) make it CurrentThis(x,newThis), otherwise change nothing).

Any references to "transform" or "conversion" above are non-mutating, by the way.