# Test unicode correctness in parser
# Expect:
# <true>

# Every non-newline whitespace char
# Tab, space, "ogham space mark", "en quad", "em quad", "en space", "em space", "3 per em space", "4 per em space", "6 per em space", "figure space", "punctuation space", "thin space", "hair space", "narrow no break space", "medium mathematical space", "ideographic space"
print	               　true

# TODO: Also need test cases for newline whitespace chars, normalization of identifiers