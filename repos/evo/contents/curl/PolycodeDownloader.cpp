
#include "PolycodeDownloader.h"
#include "tinyxml.h"

size_t DownloaderCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {

	PolycodeDownloader *downloader = (PolycodeDownloader*) userdata;
	
	size_t newSize = downloader->size + (size*nmemb);
	downloader->data = (char*)realloc((void*)downloader->data, newSize);
	
	memcpy(downloader->data+downloader->size, ptr, (size*nmemb));
	
	downloader->size = newSize;	
	return size * nmemb;
}

String PolycodeDownloader::getDataAsString() {	
	char *ndata = (char*)malloc(size+1);
	memcpy(ndata, data, size);
	ndata[size] = '\0';
	String ret = String(ndata);
	free(ndata);
	return ret;
}
		
PolycodeDownloader::PolycodeDownloader(String url) : Threaded() {
	this->url = url;
	data = (char*)malloc(0);
	size = 0;
	returned = false;
	put_progress = 0;
}

bool PolycodeDownloader::writeToFile(String fileName) {
	FILE *f = fopen(fileName.c_str(), "wb");
	if(!f)
		return false;	
	fwrite(data, 1, size, f);
	fclose(f);	
	return true;
}

void PolycodeDownloader::putString(String _put) {
	put = _put;
}

void pop_objectEntry(lua_State *L, ObjectEntry *o);

// Arguments: root name, lua table to save
// Returns: nothing
int PolycodeDownloader::putTable(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
	luaL_checktype(L, -1, LUA_TTABLE);
	Object o;
	o.root.name = lua_tostring(L, 2);
	
	pop_objectEntry(L, &o.root);
	
	// Nothing for this in Object, so I'll build it myself.
	TiXmlDocument doc;  	
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );  
	doc.LinkEndChild( decl ); 
	
	TiXmlElement * rootElement = o.createElementFromObjectEntry(&o.root);
	doc.LinkEndChild(rootElement);
	
	TiXmlPrinter printer;
	printer.SetStreamPrinting();
	doc.Accept(&printer);
	put = printer.CStr();
	
	return 0;
}

static size_t read_callback_trampoline(void *ptr, size_t size, size_t nmemb, void *userp)
{
	PolycodeDownloader *d = (PolycodeDownloader *)userp;
	return d->read_callback(ptr,size,nmemb);
}

size_t PolycodeDownloader::read_callback(void *ptr, size_t size, size_t nmemb) {
	if(size*nmemb < 1)
		return 0;
	
	int i = 0;
	while(put.size()>put_progress && i < size*nmemb) {
		((char *)ptr)[i++] = put[put_progress++];
	}
	
	return i;                          /* no more data left to deliver */ 
}

void PolycodeDownloader::runThread() {
	curl = curl_easy_init();
		
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());	
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloaderCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	if (put.size() > 0) {
		put_progress = 0;
		
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
		
		/* we want to use our own read function */ 
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, put.size());
		
		/* we want to use our own read function */ 
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback_trampoline);
		
		/* pointer to pass to our read function */ 
		curl_easy_setopt(curl, CURLOPT_READDATA, this);
	}
			
	CURLcode curl_res = curl_easy_perform(curl);
	
	curl_easy_cleanup(curl);	

	returned = true;	
//	dispatchEvent(new Event(Event::COMPLETE_EVENT), Event::COMPLETE_EVENT);

}

PolycodeDownloader::~PolycodeDownloader() {
	free(data);
}
