#ifndef DOT_H
#define DOT_H

typedef enum dotType {
	DOT_NONE = 0, // A dot of that type shouldn't exist at runtime
	DOT_BLOCK,
	DOT_WATER,
	DOT_SAND,
} dotType;

typedef struct dotTypeStruct {
	unsigned char r, g, b, a;
	dotType type; // Questionably useful
	int gravity;
	int maxFallSpd;
	int xDecceleration;
	int sideChance;
	int slideMultiplier;
	int weight;
	float opacity;
} dotTypeStruct;

typedef struct Dot {
	dotTypeStruct* type;
	unsigned char r, g, b, a; // Only used for DOT_BLOCK
	int mx, my;
	int dx, dy;
	unsigned char iteration; // Internal implementation
} Dot;

void initDot(void);
void cleanupDot(void);

Dot* createDot(void);
dotType getDotType(Dot* dot);
void addDotStruct(Dot* dot, dotType type);
dotType getDotTypeFromString(const char* str);

unsigned char canMoveThroughDot(Dot* dot);

#endif // DOT_H
