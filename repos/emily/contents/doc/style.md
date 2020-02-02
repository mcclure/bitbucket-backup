**ML coding standards for Emily project**

[TOC]

## Writing code

- Variable and package names are camelCased (contrary to normal OCaml standards). I could probably be talked into changing this.
- Tabs should be 4 spaces, no tab characters should appear in source. To ensure I don't accidentally mix in tabs, I install [the Checkfiles Extension](http://mercurial.selenic.com/wiki/CheckFilesExtension) for Mercurial and then I add this to my `.hg/hgrc`:

        [hooks]
        pretxncommit.checkfiles=hg checkfiles
        [checkfiles]
        checked_exts = .ml .em .py .pl .md .txt .h .c .cpp
        ignored_files = sample/test/parse/unicode/whitespace.em sample/test/backslash/basic.em sample/test/backslash/fail/eof.em doc/manual.md doc/tutorial.md

- There is a `make test`. It runs all the test cases listed in `sample/regression.txt`. `make && make test` should be run frequently (maybe eventually I'll make myself a precommit hook for that, too).

    Because `make test` is meant to catch and document **regressions**, nothing should ever be intentionally checked in if `make test` is failing. If a commit must be made while a test is failing, the test in question should be moved from `regression.txt` to `regression-known-bad.txt`. "Actually all tests" including the known bad ones can be tested with `make test-all`.

- Nothing critical to the build should ever depend on anything but OCaml, opam+opam modules or (because I guess I don't have an alternative) GNU make. (Note `make test` uses Python, and `make manpage` uses the Ruby-based `ronn` tool, but neither of these are critical to the build.) Required opam modules should be documented in [build.md](build.md).

- Please write as many comments as you sensibly can. I seriously think a good ratio of lines of comments:lines of code is 1:1.  A good use for comments is explaining the code's **intent**; we can probably understand **what** the code is doing just by reading it, but are less certain to understand **why**.

## When writing comments or documentation

- The preferred pronoun for describing a nonspecific third person is singular "they".

- Never use "foo", "bar", or "baz" as example names for variables. Just... find something else.

- Standalone documentation files should be in [Bitbucket-format Markdown](https://bitbucket.org/tutorials/markdowndemo).

## About branching and versions

Note: **Don't stress about this** if you're submitting code, just pull request and I'll clean it up myself. But:

- This repository has multiple development branches. In the Mercurial version of the repository (and anywhere in this document I say "branch"), "branches" refers to a commit marked by a bookmark and its ancestors. I do not use the `hg branch` functionality of Mercurial at all (although some `hg branch`es have been unintentionally introduced to the repository history by BitBucket pull requests). In Git the branches are just branches.

- There are two main branches: `stable` and `unstable`. `stable` is the commit of the most recent officially released version. `unstable` is the commit of the most recent development version.

- For each minor version, a "family" branch exists; for example, `family-0.1` refers to the most recent 0.1.x update, `family-0.2` refers to the most recent 0.2.x update, and so on. Family branches may exist for as-yet unreleased or incomplete versions.

- Each release has a code version number. The code version number should be reflected accurately in all docs which contain a version number at all, as well as in the `options.ml` source file, as well as the Makefile. If commits are made on a version which has not been released, the version number should end with a "b", like `0.2b`, in both the docs and the code.

- If changes are made to the documentation without any changes being made to the code, there may be a "documentation update". For example, after version 0.1 was released, there was a "version 0.1 documentation update 1". The code version number (i.e., in `options.ml`) remains the same for documentation releases.

- Any official release should have a tag entered along with it. The tag for a code release should look like `emily-0.1`. The tag for a documentation update looks like `emily-0.1-doc_2` for version 0.1, documentation update 2. If there is uncertainty as to exactly which commit was the official release (for example, due to documentation errors), trust the tag.

- To keep merging easy, if a commit is made in the main repository to an old family, that commit should be merged forward into all future families, **even if** merging forward just means reverting all the changes on merge. (In Mercurial terminology, this means that default should have only one head.) People submitting pull requests should **not** attempt to handle this themselves.