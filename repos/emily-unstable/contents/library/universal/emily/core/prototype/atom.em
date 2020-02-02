exportLet .parent: directory.primitive # Prototype inherits other prototype

# Given a binary function, embed it on the current object as a method
private.setMethodFrom ^key method = \
    internal.setPropertyKey current key method

setMethodFrom .toString: internal.atom.toString
