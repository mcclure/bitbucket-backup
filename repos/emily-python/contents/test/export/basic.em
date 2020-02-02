# Test export 
# Arg: --exported
# Expect:
# Printed
# w: Cheats
# x: 4
# y: yes
# z: [Method]

let w = 2

export x = 4
export y = "no"
y = "yes"
export method z = 5
export w = "Cheats"

println "Printed"
