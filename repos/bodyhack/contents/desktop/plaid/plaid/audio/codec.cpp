#include "implementation.h"


using namespace plaid;


//Codec registry
typedef std::map<String, AudioCodec*> Codecs;
typedef Codecs::value_type Codec;
static Codecs &codecRegistry() {static Codecs c; return c;}
static bool &lockCodecs() {static bool b = false; return b;}

AudioCodec::AudioCodec(String ext)
{
	if (lockCodecs()) return;
	Codecs &codecs = codecRegistry();

	Uint32 i = 0; String tmp; Char c;
	const Char *arr = ext.c_str();
	while (true)
	{
		c = arr[i]; ++i;
		if (!c || c == ',')
		{
			codecs.insert(Codec(tmp, this));
			tmp = L"";
			if (!c) break;
			continue;
		}
		if (c >= 'A' && c <= 'Z') c += ('a'-'A');
		tmp.push_back(c);
	}
}

void AudioCodec::LockRegistry()
{
	lockCodecs() = true;
}


AudioCodec *AudioCodec::Find(const String &file)
{
	size_t pos = file.find_last_of(L'.');
	if (pos != String::npos) ++pos;
	String ext = file.substr(pos);

	//Case insensitivity -- convert to lowercase
	for (unsigned i = 0; i < ext.size(); ++i)
		if (ext[i] <= 'Z' && ext[i] >= 'A') ext[i] += ('a'-'A');

	Codecs &codecs = codecRegistry();
	Codecs::const_iterator i = codecs.find(ext);
	if (i == codecs.end()) return NULL;
	return i->second;
}
