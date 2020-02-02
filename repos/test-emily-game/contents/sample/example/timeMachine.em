# "Time machine" sample from intro.md
# NOT PART OF REGRESSION TESTS -- DOES NOT TERMINATE

timeMachine ^ = (           # Function takes no arguments
    goBackward = return     # Store return in a variable. Yes, really.
    return 1                # Return. This line only runs once.
)

counter = do timeMachine    # The return value of "do TimeMachine" is 1, right?
println: counter            # Only the first time-- every time we call goBackward,
goBackward: counter + 1     # we return "counter + 1" from timeMachine,
                            # even though timeMachine already ended.
