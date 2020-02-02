-- Simple AST variable test, return value
-- Expect return 11

require("lib-lua/simple")
require("lib-lua/practice/ast")

module = AstModule{name="main"}
int32 = AstType{literal=int32Type}
int8 = AstType{literal=int8Type}
void = AstType{}
int8ptr = AstType{ptr=int8}
int8ptrptr = AstType{ptr=int8ptr}
module.namedFns.main = AstFn{ret=int32, args={int32, int8ptrptr}, body=AstBody{
	entry=AstBlock{
		exprs={
			AstExpr{op='store', var='x', value=AstExpr{op='bin',op2='+',
				value={ AstExpr{op='literal', value=4}, AstExpr{op='literal', value=7} }}},
			AstExpr{op='return', value=AstExpr{op='load', var='x'}},
		}
	}
}}
module:emit()
