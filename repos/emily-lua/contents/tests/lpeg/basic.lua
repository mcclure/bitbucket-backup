require("lib-lua/parser")
require("lib-lua/practice/ast")

lpeg.locale(lpeg) -- NO NO NO THIS IS BAD

-- Lowercase if returns string, uppercase if returns AST value

local letter = lpeg.R("az") + lpeg.R("AZ")
local digit = lpeg.R("09")
local number = digit^1
local word = letter * (letter + digit)^0
local space = lpeg.space^0

local BaseExp = number / function(v) return AstExpr{op='literal',value=tonumber(v)} end
              + word / function(v) return AstExpr{op='load',var=v} end

local bin = lpeg.P("<=") + lpeg.P(">=") + lpeg.S("+-*/%<>")

local Exp = (BaseExp * space * lpeg.C(bin) * space * BaseExp)
	       		/ function(left, op, right)
	       			return AstExpr{op='bin', op2=op, value={left,right}}
	       		end
          + BaseExp

local EqLeft = lpeg.C(word)

local AssignStatement = (EqLeft * space * lpeg.P("=") * space * Exp)
		/ function(left, e)
			return AstExpr{op='store', var=left, value=e}
		end

local ReturnStatement = (lpeg.P("return") * space * Exp)
		/ function(e)
			return AstExpr{op='return', value=e}
		end

local ExpressionStatement = Exp

local newline = lpeg.P("\n")
local commentStatement = lpeg.P("#") * (1-newline)^0 * newline

local Statement = AssignStatement + ReturnStatement + ExpressionStatement + commentStatement

local Program = ( (space * Statement)^0 * space * -1 ) -- -1 for EOF
		/ function(...)
			return AstModule{namedFns = {
				main=AstFn{ret=AstType{literal=int32Type},
					body=AstBody{ entry = AstBlock{
						exprs = {...}
					}}
				}
			}}
		end

local args = lapp [[
Attempts to compile a pseudo-Emily
-o (default 'main') Name of output object file
<file> (string) File to parse
]]

local file = io.open( args.file, "rb" )

if not file then lapp.error(args.file .. " not found") end

local contents = filedump( file )

local module = lpeg.match(Program, contents)

if not module then lapp.error("Program failed to compile") end

module.name = args.o

--pretty.dump(module)

module:emit()
