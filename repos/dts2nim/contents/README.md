# `dts2nim`: a TypeScript/Nim bridge

This is a command-line utility that analyzes a [TypeScript](https://www.typescriptlang.org/) program or type definition file and emits a source module for the [Nim](http://nim-lang.org/) programming language, containing import bindings for all symbols visible in the TypeScript file. In other words, this allows you to use any JavaScript library which TypeScript can use, from within Nim.

## Usage

An example of a Nim/TypeScript project set up with this tool is available [here](https://bitbucket.org/runhello/nim-webgl-example) (or [here](https://github.com/mcclure/nim-webgl-example) if you prefer git).

Using the tool looks like:

    dts2nim example.ts -o example.nim -q

The tool takes in exactly one `.ts` or `.d.ts` file. If you need to use multiple `.ts` files worth of material, make a joiner file and put some `// <reference />`s in it. The output file is specified with `-o`. While running, the tool will emit [many] warnings describing the symbols it was not able to translate; you can direct these to a file with `-l`, or silence them with `-q`.

The tool maintains a "blacklist" of symbols it should not try to translate. This is needed because there are at the moment some types which the tool will attempt to translate even though translating them will result in a Nim compile error. About fourteen items from the standard library are on the blacklist by default. You can add more items using the `--blacklist` flag, which takes a comma-separated list.

Blacklist items are of the format `kind:class.name`. You can shorten this to `kind:name`, `class.name` or just `kind` if you want. The allowed "kinds" are `variable`, `static`, `field` and `class` (interfaces count as classes). So for example, if the blacklist contains the item `static:Performance.timing`, then this means the static field or method `timing` on the class `Performance` will be ignored.

More flags are described in the `--help`.

## Translation

Names from TypeScript are slightly modified to work in Nim:

* The constructor for a class `Example` becomes `newExample()`. (Interfaces are not given constructors.)
* Static members of classes become variables with the class name followed by the member name (so `SomeClass.someField` becomes `SomeClassSomeField`).
* `$` symbols in names are replaced with the letters `zz`.
* Groups of two or more underscores in names are replaces with a single underscore.
* An underscore at the start of the name is replaced with a `z`.
* Any symbol which is a Nim reserved word gets an `x` prepended (so for example a TypeScript variable named `type` would be presented to Nim as `xtype`).
* A combination interface+variable whose variable type is an interface that overloads `new` will be collapsed all into one single class with the variable-type interface members treated as statics. This is an idiom which is used heavily in the TypeScript standard library.

## Future work

This is an early attempt at this tool, and `dts2nim` has a series of feature improvements I would like to make to it:

* It does not support generics. This is a huge problem because `Array` and `ArrayLike` both require generics.
* It does not support `any`, which is a big part of TypeScript. Although has nothing like an `any` type, in principle `any` could be in some situations by using generics.
* Interfaces are currently imported as object types. It would make more sense for them to be concepts. (This would also help with `ArrayLike`.)
* `dts2nim` spits all symbols into one file. It would be nice if, when given a tree of several `TypeScript` files, it could create a separate module for each one.
* `dts2nim` currently assumes that the symbols from the TypeScript side are exported into the global namespace, as if all your source files were being declared with `<script />` tags. It does not interoperate with commonjs or any sort of module system.
* The TypeScript `module` and `namespace` keywords are not supported.
* Union types are handled only under narrow circumstances (when used in method parameters).
* The name conversion `dts2nim` does can currently sometimes result in namespace collisions on the Nim side, which causes Nim errors.

In addition, there are two large "conceptual" fixes that should be done:

* At the moment, `dts2nim` works by loading type and symbol information out of the TypeScript compiler API. The compiler API features were not designed for the purpose of analyzing a whole program, and were intended to be used for syntax highlighting in text editors. Because of this, in several places `dts2nim` uses unsupported API features or relies on internal values of data structures. This is likely to break with future TypeScript upgrades. A better way for `dts2nim` to work would be either to request additional API points from the TypeScript team to get what it needs in a supported way, or to ignore the type/symbol database and instead parse ASTs exported from the compiler API.
* I would like the tool to eventually evolve to create bindings for other languages (such as Haxe, or maybe even C++ or C#). Although the tool only supports Nim currently, it is designed for additional output languages to be added easily.

The tool is being developed in a Mercurial repository at [BitBucket](https://bitbucket.org/runhello/dts2nim). Since most people prefer git, [a GitHub mirror](https://github.com/mcclure/dts2nim) is also maintained. If you wish to report issues or submit a pull request, please do so at the GitHub page.

## Building

You can install the tool by running `npm install dts2nim`. The tool will be installed into `./node_modules/.bin`.

To build your own copy from source, you will need to have `npm`, GNU `make`, TypeScript, and `typings` installed and available from your command line. You can get those last two by running `npm install -g typescript typings`.

Once you have these things, run `npm install` followed by `make all`.

To run the tests, run `npm test`. This will run the tests in node, and after you have done this you can open [tests/index.html](tests/index.html) to run the same tests in a browser.

## License

> Copyright (c) 2016 Andi McClure

> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

The software makes use of NPM packages including TypeScript itself. These will have their own licenses.

If you have any questions, you can contact me by email: <<andi.m.mcclure@gmail.com>>
