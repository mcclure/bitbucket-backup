# Test unicode correctness in parser
# Expect failure

# Fails because ⚧ is a symbol, like *, not a letter character.
let .⚧ true
print ⚧