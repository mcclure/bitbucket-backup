#pragma once

#include <Polycode.h>

extern "C" {	
#include <curl/curl.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

using namespace Polycode;

class PolycodeDownloader : public Threaded {
	public:
		PolycodeDownloader(String url);
		~PolycodeDownloader();		
		
		void runThread();
		
		String getDataAsString();
		
		bool writeToFile(String fileName);
		
		char *data;
		size_t size;
		
		bool returned;
		
		void putString(String _put);
		int putTable(lua_State *L);
		size_t read_callback(void *ptr, size_t size, size_t nmemb);
	
		void *userData;
		
	protected:
		String url;
		String put;
		int put_progress;
		CURL *curl;
};