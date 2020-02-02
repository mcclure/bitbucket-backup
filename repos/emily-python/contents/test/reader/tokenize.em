# Make sure spaces are handled sensibly

# Arg: --ast
# Expect: (#3 a3 #0.3, a = #3, a = #3, a = #3, a = #3, a == #3, a = = #3, a = = #3, a == #3, a ((a)), #3 "3" #3, #3.3 #0.3, #3 + #3 + a +@+ a3 + a #0.3 + .++ a .a ++)

3a3.3
a = 3, a=3, a= 3, a =3
a==3, a= =3, a = = 3, a == 3
a( ( a))
3"3"3
3.3.3
3+3+a+@+a3+a.3+.++a.a++
