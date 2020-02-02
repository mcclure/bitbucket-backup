
#define TETRAHEDRON_FACES (5*4)
#define TETRAHEDRON_POINTS 12
const static float tetrahedron_faces[TETRAHEDRON_FACES][3] = {
	// 5 faces around point 0
	{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},

	// 5 adjacent faces
	{1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
	 
	// 5 faces around point 3
	{3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},

	// 5 adjacent faces
	{4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1},
};


struct coloroid {
	virtual unsigned int color() = 0;
};

struct color_random : public coloroid {
	unsigned int color() {
		return randomColor();
	}
};

struct color_filter : public coloroid {
	coloroid *sub;
	color_filter(coloroid *_sub) : sub(_sub) {}
};

struct color_triplet : public color_filter {
	unsigned int last; unsigned int count;
	color_triplet(coloroid *_sub) : color_filter(_sub), last(0), count(0) {}
	unsigned int color() {
		if (count++%3 == 0)
			last = sub->color();
		return last;
	}
};

struct color_triplet_sometimes : public color_filter {
	unsigned int last; unsigned int count; bool stuck;
	color_triplet_sometimes(coloroid *_sub) : color_filter(_sub), last(0), count(0) {}
	unsigned int color() {
		if (count%3 == 0) stuck = random() % 2;
		if (count++%3 == 0 || !stuck)
			last = sub->color();
		return last;
	}
};

struct color_sometimes : public color_filter {
	color_sometimes(coloroid *_sub) : color_filter(_sub) {}
	unsigned int color() {
		return random()%2 ? sub->color() : BLACK;
	}
};

struct color_tetrahedron : public color_filter {
	vector<unsigned int> map;
	int i;
	color_tetrahedron(coloroid *_sub) : color_filter(_sub), i(0) {
		map.resize(TETRAHEDRON_POINTS);
	}
	unsigned int color() {
		int idx = i++%TETRAHEDRON_POINTS;
		if (map[idx]) {
			return map[idx];
		} else {
			unsigned int color = sub->color();
			int face;
			for(face = 0; face < TETRAHEDRON_FACES-1; face++) { // Assume final matches
				bool match = false;
				for(int c = 0; c < 3; c++)
					match = match || tetrahedron_faces[face][c] == idx;
				if (match) break;
			}
			for(int c = 0; c < 3; c++) {
				int outidx = tetrahedron_faces[face][c];
				if (!map[outidx])
					map[ outidx ] = color;
			}
			return color;
		}
	}
};

void pile_recoloroid(pile &q, coloroid *color) {
	int count = q.vertices();
	for(int c = 0; c < count; c++)
		pile_recolor(q, c, color->color());
}