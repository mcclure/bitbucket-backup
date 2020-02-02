/*
 *  svgload.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 12/5/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

#include "program.h"
#include "svgloader.h"
#include "tinyxml.h"

// This is an interpreter which constructs Polycode screens based on the contents of an SVG file.
// It interprets only a small, idiomatic subset of SVG based on what is observed to be emitted by Inkscape.
// It can't handle a number of moderately advanced features, like curves.
// It does recognize additional XML attributes specifically useful for making a Polycode physics screen.

inline string S(const char *s) { return s ? string(s) : string(); }

// Command stacks-- several svg properties (paths, transforms) involve a token followed by a number of floats

struct svgcommand {
	string cmd;
	vector<double> numbers;
};
typedef vector<string>::const_iterator vsi;
typedef vector<svgcommand>::const_iterator vsci;
typedef vector<cpVect>::iterator vcvi;

inline bool _isnumber(int c) { return c >= '0' && c <= '9'; }
inline bool _isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

vector <string> tokenize(const string &value) {
	vector<string> tokens;
	
	string building;
	for(int c = 0; c < value.length(); c++) {
		const char &l = value[c];
		if (_isalpha(l) || _isnumber(l) || l == '.' || l == '+' || l == '-') {
			building += l;
		} else {
			if (!building.empty()) {
				tokens.push_back(building);
				building.clear();
			}
		}
	}
	if (!building.empty()) tokens.push_back(building);

	return tokens;
}
vector <svgcommand> cmdparse(const string &value) {
	const vector<string> &tokens = tokenize(value);
	vector<svgcommand> result;
	for(vsi b = tokens.begin(); b != tokens.end(); b++) {
		const string &token = *b;
		const char *start = token.c_str();
		char *end;
		double number = strtod(start, &end);
		bool alpha = start == end;
		
		if (alpha) {
			result.push_back(svgcommand());
			result.back().cmd = token;
		} else {
			if (result.size())
				result.back().numbers.push_back(number);
		}
	}	
	return result;
}

string extract_text(TiXmlNode *xml)
{
	if (xml) {
		if (xml->Type() == TiXmlNode::TEXT) {
			return xml->Value();
		} else if (xml->Type() == TiXmlNode::ELEMENT) {
			string result;
			for(TiXmlElement *cxml = (TiXmlElement *)xml->FirstChild(); cxml; cxml = (TiXmlElement *)xml->IterateChildren(cxml)) {
				result += extract_text(cxml);
			}
			return result;
		}
	}
	return string();
}

// This really ought to be a matrix transform. But it isn't.
class svgtransform {
protected:
	cpVect translate;
	cpFloat scale;
public:
	svgtransform(cpVect _translate = cpvzero, cpFloat _scale = 1) : translate(_translate), scale(_scale) {}
	svgtransform(const svgtransform &a) {
		translate = a.translate;
		scale = a.scale;
	}
	svgtransform(const svgtransform &a, const svgtransform &b) {
		translate = cpvadd(a.translate, b.translate);
		scale = a.scale*b.scale;
		ERR("COMPOSE %lf,%lf + %lf,%lf = (%lf,%lf)\n", a.translate.x, a.translate.y, b.translate.x, b.translate.y, translate.x,translate.y);
	}
	svgtransform operator+(const svgtransform &b) const {
		return svgtransform(*this, b);
	}
	cpVect apply(const cpVect &in) const {
		return cpvmult(cpvadd(in, translate), scale);
	}
	cpFloat raw_scale() const {
		return scale;
	}
	static svgtransform parse(const string &value) {
		svgtransform result;
		const vector<svgcommand> &commands = cmdparse(value);
		
		for(vsci b = commands.begin(); b != commands.end(); b++) {
			const svgcommand& command = *b;
			if (command.cmd == "translate") {
				if (command.numbers.size() >= 2) {
					svgtransform compose;
					compose.translate = cpv(command.numbers[0],command.numbers[1]);
					result = result + compose;
				}
			} else if (command.cmd == "scale") { // Never tested. FIXME: Won't work right if ever used, bc no matrix = no ordering
				if (command.numbers.size() >= 1) {
					svgtransform compose;
					compose.scale = command.numbers[0];
					result = result + compose;
				}
			}
		}
		
		return result;
	}
};

// FUNCTION

svgloader::svgloader(Screen *__screen) : _screen(__screen) {}
	
Screen *svgloader::screen() {
	if (!_screen) _screen = createScreen();
	return _screen;
}

Screen *svgloader::createScreen() {
	return new Screen();
}

bool svgloader::loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform) {
	return loadGroup(xml, parent_transform);
}

// Everything important happens here
bool svgloader::load(const string &filename) {
	TiXmlDocument xml(filename.c_str());
	if (!xml.LoadFile()) return false;
	
	TiXmlElement *rootxml = (TiXmlElement *)xml.IterateChildren("svg",NULL);
	if (!rootxml || rootxml->Type() != TiXmlNode::ELEMENT) return false;
	
	double w = 0,h = 0;
	rootxml->QueryDoubleAttribute("width", &w);
	rootxml->QueryDoubleAttribute("height", &h);
	ERR("--------------------------------------\n");
	ERR("Parsing SVG document with size %lf,%lf\n", w, h);
	
	svgtransform base_transform(cpvzero, 1.0/h*surface_height);
		
	// Notice: I treat the toplevel XML like a group. This assumes toplevel can't have a transform=.
	loadRootXml(rootxml, base_transform);
	
	ERR("--------------------------------------\n");
	return true;
}

bool svgloader::loadGroup(TiXmlElement *group, const svgtransform &parent_transform) {
	ERR("\tDescending\n");
	for(TiXmlElement *xml = (TiXmlElement *)group->FirstChild(); xml; xml = (TiXmlElement *)group->IterateChildren(xml)) {
		loadXml(xml, parent_transform);
	}
	ERR("\tAscending\n");
	return true;
}

Color colorFrom(const string &hexcode, bool *_success = NULL) {
	const char *start = hexcode.c_str();
	char *end;
	unsigned long color = strtoul(start, &end, 16);
	bool success = start != end;
	if (_success) *_success = success;
	ERR("COLOR? %s = %x\n", start, (unsigned int)color);
	if (success) {
		return Color((color>>16)&0xFF,(color>>8)&0xFF,(color>>0)&0xFF,0xFF);
	}
	return Color();
}

bool svgloader::loadXml(TiXmlElement *xml, const svgtransform &parent_transform) {
	if (xml->Type() != TiXmlNode::ELEMENT) return false;
	const svgtransform &current_transform = svgtransform::parse(S(xml->Attribute("transform")));
	svgtransform transform(parent_transform, current_transform);
	ScreenEntity *created = NULL;
	
	const string& name = xml->Value();
	const string& id = S(xml->Attribute("id"));
	ERR("Element: %s Id: %s\n", name.c_str(), id.c_str());
	
	if (name == "g") {
		loadGroup(xml, transform);
	} else if (name == "sodipodi:namedview") { // Inkscape metadata -- KLUDGE: Notice, this is accepted at any level. Is that safe?
		vector <string> tokens = tokenize(S(xml->Attribute("pagecolor"))); 
		if (tokens.size()) {
			bool success;
			Color color = colorFrom(tokens[0], &success);
			
			if (success)
				CoreServices::getInstance()->getRenderer()->setClearColor(color);
		}
	} else if (name == "rect") {
		cpVect origin, size;
		bool failure = // I assume TIXML_SUCCESS == false
		xml->QueryDoubleAttribute("x", &origin.x) ||
		xml->QueryDoubleAttribute("y", &origin.y) ||
		xml->QueryDoubleAttribute("width", &size.x) ||
		xml->QueryDoubleAttribute("height", &size.y);
		if (!failure) {
			origin = transform.apply(origin);
			size = cpvmult(size, transform.raw_scale());
			ERR("\t(RECTANGLE %lf, %lf -> %lf, %lf)\n", origin.x, origin.y, size.x, size.y);
			ScreenShape *s = new ScreenShape(ScreenShape::SHAPE_RECT, size.x, size.y);
			origin = cpvadd(origin, cpvmult(size, 0.5));
			s->setPosition(origin.x, origin.y, 0);
			addChild(s, xml, svg_rect, transform);
			created = s;
		}
	} else if (name == "path") {
		const string& type = S(xml->Attribute("sodipodi:type"));
		bool failure = true;
		
		if (type == "arc") {
			cpVect c, r;
			failure = // I assume TIXML_SUCCESS == false
			xml->QueryDoubleAttribute("sodipodi:cx", &c.x) ||
			xml->QueryDoubleAttribute("sodipodi:cy", &c.y) ||
			xml->QueryDoubleAttribute("sodipodi:rx", &r.x) ||
			xml->QueryDoubleAttribute("sodipodi:ry", &r.y);
			if (!failure) {
				c = transform.apply(c);
				r = cpvmult(r, transform.raw_scale());
				ERR("\t(CIRCLE %lf, %lf -> %lf, %lf)\n", c.x, c.y, r.x, r.y);
				ScreenShape *s = new ScreenShape(ScreenShape::SHAPE_CIRCLE, r.x*2, r.y*2);
				s->setPosition(c.x, c.y, 0);
				addChild(s, xml, svg_circle, transform);
				created = s;
			}
		}
		
		if (failure) { // If didn't interpret as anything OTHER than a polygon
			ERR("(POLYGON)\n");
			const vector<svgcommand> &commands = cmdparse(S(xml->Attribute("d")));
			vector<cpVect> points;
			
			for(vsci b = commands.begin(); b != commands.end(); b++) {
				const svgcommand& command = *b;
				
				if (command.cmd == "M" || command.cmd == "L") { // Move/Line -- absolute
					for(int c = 0; c+1 < command.numbers.size(); c+=2) {
						points.push_back( cpv(command.numbers[c],command.numbers[c+1]) );
					}
				} else if (command.cmd == "m" || command.cmd == "l") { // Move/line -- relative
					for(int c = 0; c+1 < command.numbers.size(); c+=2) {
						const cpVect &last = points.size() ? points.back() : cpvzero;
						points.push_back( cpvadd(last, cpv(command.numbers[c],command.numbers[c+1])) );
					}
				} else if (command.cmd == "z") { // Close loop
					// Actually don't close loop, atm we assume all loops are closed.
				}
			}
						
			if (points.size()>2) {
				bool triangle = (points.size() == 3);
				ScreenMesh *s = (triangle ? new ScreenMesh(Mesh::TRI_MESH) : new ScreenMesh(Mesh::TRIFAN_MESH));
				Mesh *m = s->getMesh();
				Polycode::Polygon *p = new Polycode::Polygon();
				cpVect com = cpvzero;
				for(vcvi b = points.begin(); b != points.end(); b++) {
					*b = transform.apply(*b);
					com = cpvadd(com, *b);
				}
				com = cpvmult(com, 1.0/(points.size()));
				ERR("CENTER %lf, %lf%s\n", com.x, com.y, triangle?" (TRIANGLE)":"");
				for(vcvi b = points.begin(); b != points.end(); b++) {
					*b = cpvsub(*b, com);
					ERR("POINT %lf, %lf\n", b->x, b->y);
					p->addVertex(b->x,b->y,0);
				}
				m->addPolygon(p);
				s->setPosition(com.x,com.y,0);
				addChild(s, xml, svg_mesh, transform);
				created = s;
			}
		}
	} else if (name == "text") {
		cpVect origin;
		bool failure = // I assume TIXML_SUCCESS == false
			xml->QueryDoubleAttribute("x", &origin.x) ||
			xml->QueryDoubleAttribute("y", &origin.y);
		
		if (!failure) {
			const vector<string> &style = tokenize(S(xml->Attribute("style")));
			double size = 24;
			string font = "mono";
			const string &text = extract_text(xml);
			origin = transform.apply(origin);
			
			for(int c = 0; c < style.size()-1; c++) {
				if (style[c] == "font-size") { // Assumes color in #RRGGBB format; not a valid assumption for general SVGs.
					const char *start = style[c+1].c_str();
					char *end;
					double newsize = strtod(start, &end);
					if (start != end) {
						size = newsize; // TODO: Check pt/px?
					}
				}
			}
			
			size *= transform.raw_scale();
			
			// SVG specifies by the bottom left, but Polycode currently only lets us place by top left.
			// Reuse the terminal.cpp code for getting a font's height and move upward by the line_height to compensate.
			FT_Face desiredFont = CoreServices::getInstance()->getFontManager()->getFontByName(font)->getFace();	
			FT_Set_Char_Size(desiredFont, size*64, size*64, 72, 72);
			double line_height = desiredFont->size->metrics.height/64.0;
			origin.y -= line_height;
			
			ScreenLabel *s = new ScreenLabel(text, size, font);
			s->setPositionMode(ScreenEntity::POSITION_TOPLEFT); // Note: Will break physics unless we become a floater
			s->setPosition(origin.x, origin.y, 0);
			addChild(s, xml, svg_floater, transform);
			created = s;			
		}
		
	} else {
		ERR("Unknown element %s\n", name.c_str());
	}
	
	if (created) {
		const vector<string> &style = tokenize(S(xml->Attribute("style")));
		for(int c = 0; c < style.size()-1; c++) {
			if (style[c] == "fill") { // Assumes color in #RRGGBB format; not a valid assumption for general SVGs.
				bool success;
				Color color = colorFrom(style[c+1], &success);
				if (success)
					created->setColor(color);
			}
		}
	}
	return true;
}

bool svgloader::addChild(ScreenEntity *shape, TiXmlElement *, svgtype, const svgtransform &)
{
	screen()->addChild(shape);
	return true;
}

#if 0
// TODO: This should return a Scene not a Screen
physics_svgloader::physics_svgloader(PhysicsScreen *__screen) : svgloader(__screen) {}

PhysicsScreen *physics_svgloader::createScreen() {
	return new PhysicsScreen();
}

bool physics_svgloader::addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform)
{
	int physicsType = 0;
	switch (kind) {
		case svg_rect:		physicsType = PhysicsScreenEntity::ENTITY_RECT; break;
		case svg_circle:	physicsType = PhysicsScreenEntity::ENTITY_CIRCLE; break;
		case svg_mesh:		physicsType = PhysicsScreenEntity::ENTITY_MESH; break;
		case svg_floater:	svgloader::addChild(shape,xml,kind,transform); return true;
		default: return false;
	}
	int isStatic = true;
	double friction = 0.25;
	double density = 1;
	double restitution = 0.25;
	int fixedRotation = false;
	int isSensor = false;

	xml->QueryIntAttribute("polycode:isStatic", &isStatic);
	xml->QueryDoubleAttribute("polycode:friction", &friction);
	xml->QueryDoubleAttribute("polycode:density", &density);
	xml->QueryDoubleAttribute("polycode:restitution", &restitution);
	xml->QueryIntAttribute("polycode:fixedRotation", &fixedRotation);
	xml->QueryIntAttribute("polycode:isSensor", &isSensor);

	physics()->addPhysicsChild(shape, physicsType, isStatic, friction, density, restitution, isSensor, fixedRotation);
	return true;
}
#endif
