# Manually loaded	 from minimal profile

export ! = not
export ~ = neg
export && = and
export || = or
export ^^ = xor

export macro
	unaryMacro(651, "!")
	unaryMacro(651, "~")

	splitMacro(641, "*")
	splitMacro(641, "/")
	splitMacro(641, "%")

	splitMacro(631, "+")
	splitMacro(631, "-")

	splitMacro(621, "<")
	splitMacro(621, ">")
	splitMacro(621, "<=")
	splitMacro(621, ">=")

	splitMacro(611, "==")
	splitMacro(611, "!=")

	splitMacro(601, "^^")

export macro shortCircuitBoolean # 603, 605
