grammar basic;

options 
{
    output = AST;
    language = C;
    ASTLabelType	= pANTLR3_BASE_TREE;
}

start : WHITE? command EOF -> command
	;
	
command 
	: LINEN statement	-> ^(LINEN statement)
	| statement		-> statement
	;
	
statement
	:	PRINT expr -> ^(PRINT expr)
	|	GOTO WHITE? LINEN -> ^(GOTO LINEN)
	|	onearg TRASHY* -> ^(onearg)
	|	noarg WHITE?
	;	
	
// REM is formatted like a one-arg command, just the command does nothing.
onearg  : REM | LOAD | SAVE | RUN;
noarg	: CATALOG | LIST;

expr
	: WHITE? STRING
	;
	
STRING : '"' ~('"')+ '"'
	;
	
LINEN 	:	('0'..'9') ('0'..'9'|' ')+
	;

WHITE	:	' '+ { $channel = HIDDEN; }
	;
	
// WTF!

PRINT	:	P R I N T;
GOTO	:	G O T O;
REM	:	R E M;
LOAD	:	L O A D;
SAVE	:	S A V E;
RUN	:	R U N;
CATALOG	:	C A T A L O G;
LIST	:	L I S T;
	
TRASHY  :	 . ;
	
fragment A:('a'|'A');
fragment B:('b'|'B');
fragment C:('c'|'C');
fragment D:('d'|'D');
fragment E:('e'|'E');
fragment F:('f'|'F');
fragment G:('g'|'G');
fragment H:('h'|'H');
fragment I:('i'|'I');
fragment J:('j'|'J');
fragment K:('k'|'K');
fragment L:('l'|'L');
fragment M:('m'|'M');
fragment N:('n'|'N');
fragment O:('o'|'O');
fragment P:('p'|'P');
fragment Q:('q'|'Q');
fragment R:('r'|'R');
fragment S:('s'|'S');
fragment T:('t'|'T');
fragment U:('u'|'U');
fragment V:('v'|'V');
fragment W:('w'|'W');
fragment X:('x'|'X');
fragment Y:('y'|'Y');
fragment Z:('z'|'Z');

/*

KEYWORD :	
	;

STRING 	: '"' ~('"')+ '"'
	;

PRINTEXP: WHITE STRING
	;
	
PRINT   : 'PRINT' ( PRINTEXP | WHITE ';')+
	;

OP	: PRINT
	| 'GOTO' LINEN
	| KEYWORD WHITE
	| 'REM' TRASH
	| TRASH
	;
*/

