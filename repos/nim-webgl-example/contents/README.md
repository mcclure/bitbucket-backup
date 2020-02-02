# A Nim/WebGL example, using dts2nim

This is a demo of how to use the [dts2nim](https://github.com/mcclure/dts2nim) tool to create a hybrid Nim/TypeScript web app.

## Running

You will need Typescript, `npm` and `nim` installed and available from your command line. You can get Typescript from npm by running `npm install -g typescript`.

Run `make all`. Then open the file install/index.html in a web browser. That's it.

## What this is / how to make your own

The important files in this repository are `src/app.nim` and `src/tsGlue.ts`. Together these form the WebGL app. The TypeScript file sets up the WebGL context and provides some helper functions. The Nim file does all the drawing.

When you run `make all`, the `index.html` and `style.css` files are copied from `static/` to `install/`; `tsc` converts `tsGlue.ts` to a js file in `install/`; `nim2dts` converts `tsGlue.ts` to a nim file in `src/`; and `nim` converts `app.nim` to a js file in `install/`.

The file `nim2dts` creates, `tsGlue.nim`, is worth explaining. It contains nothing but declarations. It has declarations for the types of every symbol in `tsGlue`. It also contains all the declarations from the TypeScript standard library. `app.nim` imports the declarations file, and this allows the code in `app.nim` not just to use the functions and variables in `tsGlue`, but to use the standard JavaScript functionality such as WebGLContext itself.

If you wanted to adapt this to write your own application, all you really need to do is make sure you retain the `tsGlue.ts` file somewhere. If you add more nim files, all they need to do is `import tsGlue`. If you add more TypeScript files, then if you want the symbols from those files to be visible to Nim you'll need to add `/// <reference path="src/yourTsFileHere.ts"/>` to the start of `tsGlue.ts`. Because `dts2nim` ropes in the entire standard library every time it creates a nim file, I don't recommend trying to call it on multiple files in a single project.

## License

The files in this package are available under the [Creative Commons Zero](https://creativecommons.org/publicdomain/zero/1.0/) license. In other words you may treat the files as public domain. If you are republishing this source code as sample code, then a credit to the original author (Andi McClure, <<andi.m.mcclure@gmail.com>>) would be appreciated but is not required.