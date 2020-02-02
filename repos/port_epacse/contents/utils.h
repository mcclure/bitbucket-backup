#ifndef UTILS_H
#define UTILS_H

unsigned randint(unsigned N); // Random int [0;N-1]
void removeMatchingFiles(const char* directory, const char* substr);
unsigned char fileExists(const char* path);

#endif // UTILS_H
