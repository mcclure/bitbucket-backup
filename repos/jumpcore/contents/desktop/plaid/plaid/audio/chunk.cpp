#include "stream.h"
#include "audio.h"

using namespace plaid;


AudioChunk::AudioChunk(AudioScratch &scratch, const AudioFormat &format,
	Sint32 **data, Uint32 length, const AudioSync &_sync) :
	sync(_sync), _scratch(scratch), _alloc(!data),
	_format(format), _cutoff(0), _length(length)
{
	//TODO move this check to AudioFormat
	if (!_format.channels || _format.channels > PG_MAX_CHANNELS)
		reportError(L"Invalid channel count in AudioChunk!");

	for (Uint32 i = 0; i < PG_MAX_CHANNELS; ++i) _data[i] = NULL;
	if (_alloc)
	{
		_scratch.alloc(_data, _length, _format.channels);
		if (!_data[0])
		{
			_alloc = false;
			_length = 0;
		}
	}
	else
	{
		for (Uint32 i = 0; i < format.channels; ++i)
		{
			_data[i] = data[i];
		}
	}
}

AudioChunk::AudioChunk(const AudioChunk &basis, Uint32 length) :
	sync(basis.sync), _scratch(basis._scratch), _alloc(true),
	_format(basis._format), _cutoff(0), _length(length)
{
	for (Uint32 i = 0; i < PG_MAX_CHANNELS; ++i) _data[i] = NULL;
	_scratch.alloc(_data, _length, _format.channels);
	if (!_data[0])
	{
		_alloc = false;
		_length = 0;
	}
}

AudioChunk::AudioChunk(const AudioChunk &basis, Uint32 length,
	const AudioFormat &format) :
	sync(basis.sync), _scratch(basis._scratch), _alloc(true),
	_format(format), _cutoff(0), _length(length)
{
	for (Uint32 i = 0; i < PG_MAX_CHANNELS; ++i) _data[i] = NULL;
	_scratch.alloc(_data, _length, _format.channels);
	if (!_data[0])
	{
		_alloc = false;
		_length = 0;
	}
}

AudioChunk::~AudioChunk()
{
	if (_alloc) _scratch.release(_data);
}
