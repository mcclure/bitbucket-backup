#ifndef DOTMATRIX_H
#define DOTMATRIX_H DOTMATRIX_H // Lol, conflicted with a variable in settings.c. Talk about a confusing error

#include "dot.h"

void initDotMatrix(void);
void cleanupDotMatrix(void);

void addDotMatrix(int x, int y);
unsigned char dotMatrixExists(int x, int y);
void activateDotMatrix(int x, int y);
void deactivateDotMatrix(int x, int y);
unsigned char isDotMatrixActive(int x, int y);
void cleanInactiveMatrices(void);

void clearDotMatrix(void);
void loadDataFile(const char* data);
void loadDotMatrixFromImages(const char* level, const char* dynamic);
void loadDotMatrixFromFile(const char* save);
void saveDotMatrixToFile(const char* save);
unsigned char dotMatrixSaveExists(const char* save);

void addDot(Dot* dot, dotType type, int x, int y);
void deleteDot(int x, int y);
Dot* getDotExt(int x, int y);
Dot* getDot(int x, int y);
dotType getDotTypePos(int x, int y);
int getDotX(int index);
int getDotY(int index);

void updateDotMatrix(void);
void renderDotMatrix(void);

#endif // DOTMATRIX_H
