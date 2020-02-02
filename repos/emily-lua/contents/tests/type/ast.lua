-- Test ast "type" operator

-- Expect return 112

require("lib-lua/simple")
require("lib-lua/practice/ast")

module = AstModule{name="main"}
int32 = AstType{literal=int32Type}
int8 = AstType{literal=int8Type}
void = AstType{}
int8ptr = AstType{ptr=int8}
int8ptrptr = AstType{ptr=int8ptr}
module.namedFns.exit = AstFn{ret=void, args={int32}}
module.namedFns.main = AstFn{ret=int32, args={int32, int8ptrptr}, body=AstBody{
	entry=AstBlock{exprs={
			AstExpr{op='store', var='x', value=AstExpr{op='literal', value=4} },

			-- Type unify with var
			AstExpr{op='type', var='x', type=int32},

			-- Type unify with expression
			AstExpr{op='type', value=AstExpr{op='literal', value=3}, type=int32},
			AstExpr{op='type', value=AstExpr{op='literal', value="string"}, type=int8ptr},

			-- This line verifies that values in 'type' exprs currently have no side-effects
			AstExpr{op='type', value=AstExpr{op='call', fn='exit', args={AstExpr{op='literal', value=1}}}, type=void},

			AstExpr{op='return', value=AstExpr{op='literal', value=112}}
		}}
}}

module:emit()
