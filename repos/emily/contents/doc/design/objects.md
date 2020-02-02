*See also [set.md](set.md)*

# EMILY OBJECT PROTOCOL

Data items in Emily are "combinators"-- they act as functions from anything to anything. In order for interactions with data items to actually make sense, we have to constrain their behavior a little bit. We define a particular group of combinators as "objects". Objects act as maps from keys to values, and are guaranteed to respond to the following three keys:

* `has` ^key: returns true if `(object key)` can return some value without failing. (In other words 'a->bool)

* `let` ^key value: Unconditionally binds `(object key)` to value `value`. (In other words 'a->('b->null))

* `set` ^key value: If `(object key)` is already bound to some value-- possibly in object's parent rather than object itself-- reassigns the value (preferably where it is bound), then returns null. Otherwise, fails. (In other words 'a->('b->null))

`has`, `set` and `let` must exist, but `set` and `let` could potentially do tricky things in addition to carrying out their basic goal. From the user's perspective, `let` corresponds only to `a = b` and `set` corresponds only to `nonlocal a = b`, and the only guaranteed semantics are that both `set` and `let` assign *something* but `set` may fail.

Some additional notes:

`set` and `let` are likely to be renamed soon to reflect (1) that this is an inaccurate usage of the word "let"; (2) that soon "getter properties" (`internal.setMethodFrom`) are going to be exposed directly on objects; and (3) that soon there will be some way to set a blanket response for a class of keys, such as "all ints".

# EMILY-CREATED OBJECTS

An "object" is any data-like entity following the above protocol. This could be some strange thing crafted by the user out of pure functions, or a wrapper for a data entity that lives "outside Emily" (a guest from C, for example).

However, there are some things that the Emily language itself does that create "objects" by the above definition. These come in two categories, "scopes" and "user objects". In the code, the latter category is often referred to as just "objects" (this is bad and should be fixed). Both of these are backed, in the current interpreter, by a simple hash table (and they fail when objects are used as a key, possibly because they tend to be self-referential). The difference between scopes and user objects is how they are constructed, and a single special behavior the interpreter performs when a key lookup completes on a user object: If the value a key maps to on a user object is a user closure, then `this` is bound on the closure.

# PREPOPULATION

Here are means by which something a user does could result in the creation of an object.

## Scoped groups:

Inside of `{ }`, a scope *S* is created with:

    * `has`, `set` and `let` bound to *S*
    * `parent` is enclosing scope

*Implementation: `tableBlank`/`tableInheriting` in `valueUtil.ml`*

*Called from:* `evaluateTokenFromTokens` in `execute.ml`

## Unscoped closures:

Inside of `^( )`, a scope *S* is created with

    * `has` and `set` bound to *S*
    * `parent` is enclosing scope of `^( )`
    * If appropriate, all args set as variables, plus `this`, `current` and `super` as appropriate for the call site.

There is a bug here: Because no `let` is set at all, a `let` bypasses *S* and goes to the enclosing scope. If you assign to an argument or to `this`, `current` or `super`, this will lead to surprising behavior which is different depending on whether you called `=` or `nonlocal =`.

*Implementation: `apply` in `execute.ml`*

## Scoped closures:

Inside of `^{ }`, a scope *S* is created with

    * `has`, `set` and `let` bound to *S*
    * `parent` is enclosing scope of `^{ }`
    * If appropriate, all args set as variables, plus `this`, `current` and `super` as appropriate for the call site.

In other words, this is nearly identical to the unscoped closure case; the difference is a `let` is created.

*Implementation: `apply` in `execute.ml`*

## Boxes:

Inside of `[ ]`, three things are created.

A user object *U* is created with

    * `has` and `set` bound to *U*
    * `let` bound to *U* with a filter that "freezes" any closure passed in to **prevent** `this`/`current` binding.
    * `parent` is the current base object prototype.

*Implementation for U: `objectBlank` in `valueUtil.ml`, called by `boxBlank`*

A scope *P* is created with

    * `has` and `set` bound to *P*
    * `let` is a function that does two things:
        * Assigns to *P*
        * Assigns to *S*

A scope *S* is created with

    * `has` and `set` bound to *S*
    * `let` bound to *U* with a filter that readies any closure passed in for `this`/`current` binding.
    * `this` and `current` are *U*
    * `private` is *P*
    * `parent` is the enclosing scope of `[ ]`.

*Implementation for S and P: `boxBlank` in `valueUtil.ml`*

An observation: the difference in behavior between `let` and `set` (i.e. `let` is magic, `set` is not, `set` and `let` are bound to different objects altogether in both *S* and *P*) could lead to surprising behavior.

## Packages

Inside of a `.em` file loaded via `package` or `project`, three things are created.

A scope *U* is created with

    * `has` bound to *U*

*Implementation for U: `completeStarter` or `starterForExecute` in `loader.ml`*

A scope *P* is created with

    * `has` and `set` bound to *P*
    * `let` is a function that does two things:
        * Assigns to *P*
        * Assigns to *S*

A scope *S* is created with

    * `has` and `set` bound to *S*
    * `let` is a function that does two things:
        * Assigns to *U*
        * Assigns to *S*
    * `this` and `current` are *U*
    * `private` is *P*
    * `parent` is the base scope prototype.

*Implementation for S and P: `boxBlank` in `valueUtil.ml`*

In other words, this is the same as the box case, except the target is a minimal scope instead of a full object, and the parent of *S* can see members of *U*.

An observation, in addition to the complaints about `set` and `let` in the box case: It does seem like it is poor usability all this is created for loaded .em files, but not normally-executed .em files.