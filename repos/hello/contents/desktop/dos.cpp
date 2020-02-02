// DOS 3.3

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include "chipmunk.h"
#include "color.h"
#include "program.h"
#include "tinyxml.h"
#include "sound.h"
#include "internalfile.h"
#include "bfiles.h"
#include "cpRect.h"

#define INPUT_LIMIT 256
#define INVISIBLE(x) ((x) == SDLK_LCTRL || (x) == SDLK_RCTRL || (x) == SDLK_LSHIFT || (x) == SDLK_RSHIFT \
	|| (x) == SDLK_CAPSLOCK || (x) == SDLK_LMETA || (x) == SDLK_RMETA || (x) == SDLK_LALT || (x) == SDLK_RALT \
	|| (x) == SDLK_UP || (x) == SDLK_DOWN)

#define CYCLES_PERROUND 100
#define CYCLES_ONECHAR  10
#define CYCLES_ONEGOTO	1

int op_interface::cycles = 0;
void op_interface::reset_cycles() { cycles = CYCLES_PERROUND; }

// Mode for entering line at ] prompt
struct input_mode : public mode_interface {
	string entry;
	bool ever_run;
	int eidx;
	
	input_mode(basic_automaton *_parent) : mode_interface(_parent), ever_run(false) {}
	virtual void enter() {
		entry.clear();
		eidx = 0;
		
		if (ever_run) parent->lf();
		ever_run = true;
		
		parent->x = 1;
		parent->screen[parent->y][0] = ']';
	}
	virtual void key(SDLKey sym, Uint16 unicode, Uint8 type) {
		bool wshow = false, wset = false; int wscroll = 0; Uint16 wunicode = 0;
		if (type == SDL_KEYDOWN) {
			if (sym == SDLK_RETURN) {		// EXECUTE
				if (eidx <= entry.size())
					entry.erase(eidx);
				parent->lf();
				if (entry.size())
					parent->nextmode(parent->parse(entry));
				else
					parent->nextmode(this);
			} else if (sym == SDLK_LEFT || sym == SDLK_BACKSPACE) {  // CURSOR
				if (eidx) {
					eidx--;
					wscroll = -1;
				} else { // Go "too far"
					enter();
				}
			} else if (sym == SDLK_RIGHT) {
				if (eidx >= entry.size()) { // Extend line
					wset = true; wunicode = ' ';
				} else {
					eidx++;
				}
				wscroll = 1;
			} else if (!INVISIBLE(sym)) {	// INPUT
				wshow = true; wset = true; wscroll = 1; wunicode = unicode;
			}
		}
		
		if (wshow) // Must we display a character?
			parent->set(parent->x,parent->y,wunicode,false);
		if (wset) { // Must we save a character?
			if (eidx >= entry.size())
				entry += ' ';
			entry[eidx] = wunicode;
			eidx++;
		}	
		if (wscroll) // Must we scroll the cursor?
			parent->next(wscroll,0);
	}
};

// Operators

void op_interface::exit() {
	mode_interface *realnext;
	if (bufnext) realnext = bufnext;
	else if (next) realnext = next;
	else realnext = parent->std_input;
	if (bufout.size()) {
		string temp = bufout;
		bufout.clear();
		print(temp);
	}
	if (cycles > 0) {
		cycles--;
		bufnext = NULL;
		parent->nextmode(realnext);
	}
}

void op_interface::tick() {
	exit();
}

void op_interface::key(SDLKey sym, Uint16 unicode, Uint8 type) {
	if (sym == 'c' && (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))) {
		bufout = string();
		bufnext = NULL;
		parent->nextmode(parent->std_input);
	}
}

void op_interface::done(mode_interface *into) {
	bufnext = into; // TODO: Only if into non-null?
	exit();
}
void op_interface::done(const string &data, mode_interface *into) {
	print(data);
	done(into);
}
void op_interface::print(const string &data) {
	int p;
	bufout += data;
	for(p = 0; p < bufout.size() && cycles > 0; p++,cycles--) {
		if (bufout[p] == '\n') {
			parent->lf();
		} else if (bufout[p] == '\r') {
			parent->next(0,1);
		} else {
			parent->set(parent->x,parent->y,bufout[p],false);
			parent->next(1,0);
		}
	}
	bufout = bufout.substr(p);
}

struct line_op : public op_interface {
	int line;
	op_interface *contents;
	line_op *nextline;
	line_op(basic_automaton *_parent, int _line, op_interface *_contents) : op_interface(_parent), line(_line), contents(_contents), nextline(NULL) {}
	virtual string toString() {
		ostringstream s;
		s << " " << line << "  "
		  << contents->toString() << "\n";
		return s.str();
	}
	void insert(line_op *newnext) { // Two pairs of linked lists? Dunno about this.
		bool replace = nextline && nextline->line == newnext->line;
		if (replace) {
			line_op *oldnext = nextline;
			nextline = nextline->nextline;
			contents->next = nextline->contents->next;
			delete oldnext;
		}
		newnext->nextline = nextline;
		nextline = newnext;
		newnext->contents->next = contents->next;
		contents->next = newnext->contents;
	}
	virtual void enter() {
		parent->line_before(line)->insert(this);
		done();
	}
};

struct no_op : public op_interface { // Should keep?
	no_op(basic_automaton *_parent) : op_interface(_parent) {}
	virtual string toString() { return string(); }
	virtual void enter() {
		done();
	}
};

struct cmd_op : public op_interface {
	int cmd;
	string cmd_literal;
	cmd_op(basic_automaton *_parent, int _cmd, const string &_cmd_literal) : op_interface(_parent), cmd(_cmd), cmd_literal(_cmd_literal) {
		for(int c = 0; c < cmd_literal.length(); c++)
			cmd_literal[c] = toupper(cmd_literal[c]);
	}
	virtual string toString() {
		return cmd_literal;
	}
	virtual void enter();
};

struct onearg_op : public cmd_op {
	string arg;
	onearg_op(basic_automaton *_parent, int _cmd, const string &_cmd_literal, const string &_arg) : cmd_op(_parent, _cmd, _cmd_literal), arg(_arg) {}
	virtual string toString() {
		return cmd_literal + " " + arg;
	}
	virtual void enter();
};

struct syntax_error_op : public op_interface {
	string entry;
	syntax_error_op(basic_automaton *_parent, const string &_entry = string()) : op_interface(_parent), entry(_entry) {}
	virtual void enter() {
		done("\n?SYNTAX ERROR");
	}
	virtual string toString() { return entry; }
};

struct goto_op : public op_interface {
	int goto_line;
	goto_op(basic_automaton *_parent, int _goto_line) : op_interface(_parent), goto_line(_goto_line) {}
	virtual void exit() {
		if (!bufnext) {
			line_op *gotonext = parent->line_at(goto_line);
			if (gotonext) {
				bufnext = gotonext->contents;
			} else { 
				// TODO: PRINT "\n?UNDEF'D STATEMENT ERROR IN " thisline
				bufnext = parent->std_input;
			}
		}
		op_interface::exit();
	}
	virtual string toString() { stringstream s; s << "GOTO  " << goto_line; return s.str(); }
};

struct print_op : public op_interface {
	exp_interface *exp;
	print_op(basic_automaton *_parent, exp_interface *_exp) : op_interface(_parent), exp(_exp) {}
	virtual void enter() {
		done(exp->strval() + "\n"); // TODO: SEMICOLONS AND STUFF
	}
	virtual string toString() { return "PRINT  " + exp->toString(); }
};

struct string_exp : public exp_interface {
	string val;
	string_exp(const string &_val) {
		val = _val.substr(1,_val.length()-2); // Assume ""s present ; clip them.
	}
	virtual string toString() { return "\"" + val + "\""; }
	virtual string strval() { return val; }
};

// Automaton implementation

basic_automaton::basic_automaton() : freetype_automaton(), mode(NULL), std_input(NULL) {
	std_input = new input_mode(this);
	root = new line_op(this, -1, new no_op(this)); // This line may be called but should be hidden from the user.
	nextmode(std_input);
}

void basic_automaton::nextmode(mode_interface *next) {
	if (mode && mode->zombie) // Normally don't test if mode exists, but nextmode could be called at boot
		delete mode;
	mode = next;
	next->enter();
}

void basic_automaton::tick() {
	automaton::tick();
	if (mode)
		mode->tick();
}

void basic_automaton::input(SDL_Event &event) {
	if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {
		mode->key(event.key.keysym.sym, event.key.keysym.unicode, event.type);
	}
}

void basic_automaton::next(int dx, int dy) {
	x+=dx; y += dy;
	if (x >= 40) {
		x = 0;
		y++;
	}
	if (x < 0) {
		x = 39;
		y--;
	}
	if (y >= 24) {
		scroll();
		y = 23;
	}
	if (y < 0) {
		y = 0;
	}	
}

void basic_automaton::lf() {
	x = 0;
	next(0,1);
}

line_op *basic_automaton::line_before(int line) {
	line_op *n;
	for(n = root; n->nextline && n->nextline->line < line; n = n->nextline);
	return n;
}
				   
line_op *basic_automaton::line_at(int line) {
	for(line_op *n = root; n->nextline; n = n->nextline)
		if (n->line == line)
			return n;
	return NULL;
}
				   
// ENTERING ANTLR NAMESPACE, ABANDON ALL HOPE YE WHO ENTER HERE

#include "basicLexer.h"
#include "BasicParser.h"
#include "antlr3.h"
				   
void cmd_op::enter() {
	switch (cmd) {
		case LIST: {
			for(line_op *n = parent->root->nextline; n; n = n->nextline)
				print(n->toString());
		} break;
	}
	done();
}				   


void onearg_op::enter() {
	bool do_done = true;
	op_interface *going = NULL;
	
	switch (cmd) {
		case RUN: {
			going = parent->root->contents;
		} break;
	}
	if (do_done)
		done(going);
}				   

string astring(pANTLR3_STRING s) {
	return string((char *)s->chars);
}

void printtabs(int indent) {
	for(int c = 0; c < indent; c++)
		ERR("\t");
}

void dump_tree(pANTLR3_BASE_TREE tree, int indent = 0) {
	pANTLR3_COMMON_TOKEN top = tree->getToken(tree);
	ERR("\n");
	printtabs(indent);
	if (top) {
//		ERR("(type %d start %d stop %d) ", (int)top->type, (int)top->start, (int)top->stop);
		ERR("NODE: %s", top->toString(top)->chars);
	}
	if (tree->children) for(int c = 0; c < tree->children->count; c++) {
		void *child = tree->getChild(tree, c);
		dump_tree((pANTLR3_BASE_TREE)child, indent+1);
	}
}

// #define SS(s, c) ((c) < (s).length ? (s).substr(c) : string())
#define CH(t, c) ((t)->children->count > (c) ? (pANTLR3_BASE_TREE)(t)->getChild((t),(c)) : NULL)
#define TOK(entry, top) (entry).substr((top)->start-start, (top)->stop-(top)->start+1)

// Assumes string valid; assumes numbers over 64000 don't matter
int linefrom(string s) {
	int accumulator = 0;
	for(int c = 0; c < s.length() && accumulator < 100000000; c++) {
		if (isdigit(s[c])) {
			accumulator *= 10;
			accumulator += (s[c] - '0');
		}
	}
	return accumulator;
}

pANTLR3_COMMON_TOKEN tokfrom(pANTLR3_BASE_TREE tree) {
	if (!tree) return NULL;
	return tree->getToken(tree);
}

static exp_interface *get_exp(basic_automaton *parent, const string &entry, const int &start, const int tokstart, pANTLR3_BASE_TREE tree) {
	exp_interface *result = NULL;
	
	if (tree) {
		pANTLR3_COMMON_TOKEN top = tree->getToken(tree);
		switch (top->type) {
			case STRING: {
				result = new string_exp(TOK(entry, top));
			} break;
		}
	}
	
	return result;
}

static op_interface *get_toplevel(basic_automaton *parent, const string &entry, const int &start, const int tokstart, pANTLR3_BASE_TREE tree) {
	op_interface *result = NULL;
	
	if (tree) {
		pANTLR3_COMMON_TOKEN top = tree->getToken(tree);
		ERR("(type %d start %d stop %d len %d)\n", (int)top->type, (int)top->start-start, (int)top->stop-start, (int)((top)->stop-(top)->start+1));
		switch (top->type) {
			case LINEN: {
				int line = linefrom(TOK(entry, top));
				if (line >= 0 && line < 64000) {
					op_interface *contents = get_toplevel(parent, entry, start, top->stop-start+1, CH(tree, 0));
					result = new line_op(parent, line, contents);
				}
			} break;
			case CATALOG: case LIST: {
				result = new cmd_op(parent, top->type, TOK(entry, top));
			} break;
			case REM: case SAVE: case LOAD: case RUN: {
				result = new onearg_op(parent, top->type, TOK(entry, top), entry.substr(top->stop-start+1));				
			} break; 
			case PRINT: {
				exp_interface *exp = get_exp(parent, entry, start, top->stop-start+1, CH(tree, 0));
				if (exp)
					result = new print_op(parent, exp);
			} break;
			case GOTO: {
				int line = linefrom(TOK(entry, tokfrom(CH(tree, 0))));
				if (line >= 0 && line < 64000) {
					result = new goto_op(parent, line);
				}
			} break;
		}
	}
	
	if (!result)
		result = new syntax_error_op(parent, entry.substr(tokstart));
	ERR("%s\n", result->toString().c_str()); // DELETE ME
	return result;
}

// See http://www.antlr.org/api/C/buildrec.html
op_interface *basic_automaton::parse(const string &entry) {
	pANTLR3_INPUT_STREAM	   input;
	input = antlr3StringStreamNew ((const uint8_t *)entry.c_str(), ANTLR3_ENC_UTF8, entry.size(), (const uint8_t *)"BasicLine");
	
	basicLexer_Ctx_struct*				lxr;
	pANTLR3_COMMON_TOKEN_STREAM		tstream;
	basicParser *				psr;
//	basicParser_decl_return		langAST;
//	pANTLR3_COMMON_TREE_NODE_STREAM	nodes;
//	basicDumpDecl			treePsr;

	lxr		= basicLexerNew(input);		// CLexerNew is generated by ANTLR
	tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lxr));
	psr		= basicParserNew(tstream);  // CParserNew is generated by ANTLR3
	//int langAST = 
	basicParser_start_return tree = psr->start(psr);

#if SELF_EDIT
	dump_tree(tree.tree); // DELETE ME
#endif
	
	pANTLR3_COMMON_TOKEN top = tree.tree->getToken(tree.tree);
	int start = top->start;
	
	op_interface *result = get_toplevel(this, entry, start, 0, tree.tree);
	
//	tree	->free	(tree);		tree	= NULL;
	psr		->free  (psr);		psr		= NULL;
	tstream ->free  (tstream);	tstream	= NULL;
	lxr		->free  (lxr);		lxr		= NULL;
	input   ->close (input);	input	= NULL;
		
	return result;
}