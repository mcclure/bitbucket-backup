// Binary hibernate/defrost helpers
#include <fstream>

#define BWRITE(x) f.write((char *) &(x), sizeof(x))
#define BREAD(x) f.read((char *) &(x), sizeof(x))

inline void s_bwrites(ofstream &f, string s) {
	int tempCount = s.length();
	BWRITE(tempCount);
	f.write(s.c_str(), tempCount);
}

inline string s_breads(ifstream &f) {
	int tempCount;
	BREAD(tempCount);
	if (tempCount > FILENAMESIZE) return string();
	char filename[FILENAMESIZE];
	f.read(filename, tempCount);
	return string(filename, tempCount);
}

#define BWRITES(x) s_bwrites(f, (x))
#define BREADS(x) (x) = s_breads(f)

template< typename T> inline void s_bwritev(ofstream &f, vector<T> &v) {
	int tempCount = v.size();
	BWRITE(tempCount);
	if (!tempCount) return;
	f.write((char *)&v[0], tempCount*sizeof(T));
}

template < typename T > inline void s_breadv(ifstream &f, vector<T> &v) {
	int tempCount;
	BREAD(tempCount);
	v.resize(tempCount);
	if (!tempCount) return;
	f.read((char *)&v[0], tempCount*sizeof(T));
}

#define BWRITEV(x) s_bwritev(f, (x))
#define BREADV(x) s_breadv(f, (x))