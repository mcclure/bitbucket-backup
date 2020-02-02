# TODO: Make these cmdline arguments
program = "This is purely a test. I am testing right now"
disasm = true
stream = null

# Utility
a = package.emily.array
f = package.emily.functional.util

Stack = [
    look ^idx = this: this.count - idx - 1
    pop ^ = {
        result = this.look 0
        this.count = this.count - 1
        this (this.count) = null
        result
    }
]

# Setup
compile ^text = {

    # Create "AST" from string
    ast = {

        # Tiny stack of AST-in-progresses
        stack = [parent=Stack]
        newFrame ^ = ( stack.append [count=0] )
        do newFrame

        # Append to topmost AST
        append ^index = {
            top = stack.look 0

            top.append index
        }

        # Complete topmost AST
        pop ^ = {
            top = do: stack.pop

            # Closing an empty parens is equivalent to identity
            if (top.count == 0) ^(top.append 0)

            # If the stack underflows, "re-inflate" it--
            # An "extra" ) on the right should auto-insert a ( on the left.
            if (stack.count == 0) newFrame

            # Push newly popped frame onto the new topmost AST, as a token
            append top
        }

        # Interpret
        text.codepoint.each ^u{
            # Wrap into (indices, )
            code = u % (stack.count + 2)

            # Remove me
            if (stream) ^(print code sp)

            code == 0 ? do newFrame : \ # Code 0 is (
            code == 1 ? do pop : \      # Code 1 is )
            code >  0 ? {               # Other positive is indices
                index = code - 2

                append index
            } : ()                      # Silently ignore anything else
        }

        # Finish interpreting by closing any unclosed )s.
        while ^(stack.count > 1) pop

        # Return top-of-stack
        stack.look 0
    }

    # Print an ast
    printFrame ^frame = {
        needSpace = null
        frame.each ^token(
            needSpace ? print " " : (needSpace = true)
            number token ? print token : (
                print "("
                printFrame token
                print ")"
            )
        )
    }

    # Remove me
    if disasm ^(printFrame ast)
    if (disasm || stream) ^(println "")

    ConstStack ^parent value key = (
        key == .parent ? parent : \
        key == 0 ? value : \
        parent (key - 1)
    )

    value ^v = [value=v; parent=nullfn] # nullfn is for safety

    executeAst ^stack ast value = {
        stack = ConstStack stack value
        register = f.identity # I NEED A FOLD
        ast.each ^token(
            register = register (
                number token ? stack token : \
                token.value  ? token : \
                executeAst stack token
            )
        )
        register
    }

    ^arg (executeAst nullfn ast: value arg)
}

compile program