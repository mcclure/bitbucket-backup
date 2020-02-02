# // operator failure case: Checking from invalid object
# Expect failure

x = [ y = [ z = 3 ] ]

println ( x.q.z // 4 )
