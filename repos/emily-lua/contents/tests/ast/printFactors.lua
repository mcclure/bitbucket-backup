-- Take one number from input, print its factors as space-separated list (inlined AST)
-- Combined test of arithmetic, array indices, branches, functions, C library prototypes

-- Arg: 1386
-- Expect: 2 3 3 7 11

require("lib-lua/simple")
require("lib-lua/practice/ast")

module = AstModule{name="main"}
int32 = AstType{literal=int32Type}
int8 = AstType{literal=int8Type}
void = AstType{}
int8ptr = AstType{ptr=int8}
int8ptrptr = AstType{ptr=int8ptr}
module.namedFns.printf = AstFn{ret=int32, args={int8ptr}, vararg=true}
module.namedFns.atoi = AstFn{ret=int32, args={int8ptr}}
module.namedFns.printFactors = AstFn{ret=void, args={int32}, body=AstBody{
	vars = { value=int32, divisor=int32 },
	namedBlocks={
		entry=AstBlock{
			exprs={
				AstExpr{op='store', var='value', value=AstExpr{op='getParam',idx=0}},
				AstExpr{op='store', var='divisor', value=AstExpr{op='literal',value=2}}
			},
			exit='loop'
		},
		loop=AstBlock{
			exprs={
				AstExpr{op='branch', cond=AstExpr{op='bin', op2='>=', value={
					AstExpr{op='load', var='value'}, AstExpr{op='load', var='divisor'}
				}}, y='modulo', n='done'}
			}
		},
		modulo=AstBlock{
			exprs={
				AstExpr{op='branch', cond=AstExpr{op='bin', op2='==', value={
					AstExpr{op='bin', op2='%', value={
						AstExpr{op='load', var='value'}, AstExpr{op='load', var='divisor'}
					}},
					AstExpr{op='literal', value=0}, 
				}}, y='divide', n='increment'}
			}
		},
		divide=AstBlock{
			exprs={
				AstExpr{op='call', fn='printf', args={
					AstExpr{op='literal',value="%d "}, AstExpr{op='load',var='divisor'}
				}},
				AstExpr{op='store', var='value', value=AstExpr{op='bin', op2='/', value={
					AstExpr{op='load', var='value'}, AstExpr{op='load', var='divisor'}
				}}},
			},
			exit='loop'
		},
		increment=AstBlock{
			exprs={
				AstExpr{op='store', var='divisor', value=AstExpr{op='bin', op2='+', value={
					AstExpr{op='load', var='divisor'}, AstExpr{op='literal', value=1}
				}}},
			},
			exit='loop'
		},
		done=AstBlock{
			exprs={
				AstExpr{op='return'}
			}
		}
	},
	entry='entry'
}}
module.namedFns.main = AstFn{ret=int32, args={int32, int8ptrptr}, body=AstBody{
	namedBlocks={
		entry=AstBlock{exprs={
			AstExpr{op='branch',
				cond=AstExpr{op='bin', op2='==', value={
					AstExpr{op='getParam',idx=0}, AstExpr{op='literal',value=2}
				}},
				y=AstBlock{exprs={
					AstExpr{op='call', fn='printFactors', args={
						AstExpr{op='call', fn='atoi', args={
							AstExpr{op='deref',
								value=AstExpr{op='getParam',idx=1},
								idx=AstExpr{op='literal', value=1}
							}
						}},
					}},
					AstExpr{op='call', fn='printf', args={
						AstExpr{op='literal',value="\n"}
					}},
					AstExpr{op='return', value=AstExpr{op='literal', value=0}}
				}},
				n=AstBlock{exprs={
					AstExpr{op='call', fn='printf', args={
						AstExpr{op='literal',value="Please enter exactly one number as argument.\n"}
					}},
					AstExpr{op='return', value=AstExpr{op='literal', value=1}}
				}}
			}
		}}
	},
	entry='entry'
}}

module:emit()
