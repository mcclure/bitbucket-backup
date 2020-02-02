## BEHAVIOR

Because all things in Emily are done with functions and keys on objects, there can be nothing (at the lowest layer) like an "=" directive. Therefore setting object fields, and for that matter local variables (remember: just fields on a scope object) is done by calling a method on the object or scope.

We need two of these methods, because there are two kinds of operations we might want to do:

"let": this method creates, and sets the value of, a new field binding.

"set": this method sets the value of an existing field binding.

A "binding" may not be exactly what you're thinking. Let us imagine you have an object `child` and an object `mother`; `mother` is `child`'s prototype parent. `mother` posesses a field x; `child` does not. When you say `child.set .x`, the `child.set` method will do nothing, because `child` has no x field, and instead pass on the call to `mother.set .x`. Thus `child` will be unmodified, and `mother` will have its x value changed.

This behavior for "set", "let" and bindings is designed for-- and makes the most sense in the case of-- a scope, where you will often specifically want to use an inner scope object to manipulate variables living in an outer scope. In the case of objects, it makes less sense: the "set"/"let" distinction still serves some purpose, since it protects you from setting a nonexistent field. However the set "fallthrough" behavior is not something you will generally want in an object-- this behavior corresponds to objects in no languages I know (although it does correspond to the inheritance behavior class statics in C++). And for some kinds of objects, say dictionaries, the set/let distinction may not be desirable at all. This behavior will need to be revisited.

Calling `obj .set .value 3` should be treated like calling a curried function of three arguments; it has no effect until all three arguments have been provided. The "binding" which set refers to should be determined when all arguments are present, no sooner.
