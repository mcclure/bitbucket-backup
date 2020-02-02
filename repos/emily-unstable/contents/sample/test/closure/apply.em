# Demonstrate a closure.
# Expect:
# 4.
# 5.
# OK
# YEAH

# Arguments

^x( println x ) 4

(^x: println x) 5

# Discard arguments

^(println "OK") 4

(^ : println "YEAH") 5
