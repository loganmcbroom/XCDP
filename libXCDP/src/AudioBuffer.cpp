#include "xcdp/AudioBuffer.h"

#include <iostream>
#include <algorithm>

#include <sndfile.h>

namespace xcdp{

AudioBuffer::AudioBuffer()
	: format()
	, buffer()
	{}
AudioBuffer::AudioBuffer( const Format & other )
	: format( other )
	, buffer( std::make_shared<std::vector<float>>( getNumChannels() * getNumFrames() ) )
	{}
AudioBuffer::AudioBuffer( const std::string & filename )
	: format()
	, buffer()
	{
	load( filename );
	}

//======================================================
//	I/O
//======================================================
void AudioBuffer::load( const std::string & filePath ) 
	{
	//Open file and check validity, save the sample rate
	SF_INFO info;
	SNDFILE * file = sf_open( filePath.data(), SFM_READ, &info ); 
	if( file == nullptr )
		{
		std::cout << filePath << " could not be opened.\n";
		return;
		}

	//Copy file info into format
	format.sampleRate = info.samplerate;
	format.numChannels = info.channels;
	format.numFrames = info.frames;
	*this = AudioBuffer( format );

	//Create temporary buffer for interleaved data in file, read data in, close the file
	std::vector<float> interleavedBuffer( info.frames * info.channels );
	sf_readf_float( file, interleavedBuffer.data(), info.frames );

	if( sf_close( file ) != 0 )
		{
		std::cout << "Error closing " << filePath << ".\n";
		return;
		}

	//Convert interleaved data in
	for( size_t channel = 0; channel < info.channels; ++channel )
		for( size_t sample = 0; sample < size_t(info.frames); ++sample )
			setSample( channel, sample, interleavedBuffer[ sample * info.channels + channel ] );
	}

bool AudioBuffer::save( const std::string & filePath ) const 
	{
	//Check that nothing silly is going on with the file formatting
	SF_INFO info = {};
	info.channels	= int( getNumChannels() );
	info.frames		= int( getNumFrames()	);
	info.samplerate = int( getSampleRate() );
	info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
	if( !sf_format_check( &info ) )
		{
		std::cout << "Sound file formatting invalid while attempting to save to " << filePath << ",\n";
		printSummary();
		return false;
		}

	//Create a temporary buffer for interleaved data and copy the buffer in
	std::vector<float> interleavedBuffer( getNumFrames() * getNumChannels() );
	for( size_t channel = 0; channel < getNumChannels(); ++channel )
		for( size_t frame = 0; frame < getNumFrames(); ++frame )
			interleavedBuffer[ frame * info.channels + channel ] = getSample( channel, frame );

	//Clip all samples in the interleaved buffer
	std::for_each( interleavedBuffer.begin(), interleavedBuffer.end(), []( float & s )
		{
		s = std::clamp( s, -1.0f, 1.0f );
		});

	//Open the file and write in the interleaved buffer
	SF_INFO outInfo = info;
	SNDFILE * file = sf_open( filePath.data(), SFM_WRITE, &outInfo );
	if( file == nullptr )
		{
		std::cout << filePath << " could not be opened for saving.\n";
		return false;
		}
	if( sf_writef_float( file, interleavedBuffer.data(), info.frames ) != info.frames )	
		{
		std::cout << "Error writing data into " << filePath << ".\n";
		return false;
		}
	sf_close( file );
	
	return true;
	}

void AudioBuffer::printSummary() const
	{
	std::cout << *this;
	}

//======================================================
//	Getters
//======================================================
float AudioBuffer::getSample( size_t channel, size_t frame ) const 
	{
	return (*buffer)[getBufferPos( channel, frame )];
	}

AudioBuffer::Format xcdp::AudioBuffer::getFormat() const
	{
	return format;
	}

size_t AudioBuffer::getNumChannels() const
	{
	return format.numChannels;
	}

size_t AudioBuffer::getNumFrames() const 
	{
	return format.numFrames;
	}

size_t AudioBuffer::getSampleRate() const 
	{
	return format.sampleRate;
	}

float AudioBuffer::frameToTime() const
	{
	return 1.0f / timeToFrame();
	}

float AudioBuffer::timeToFrame() const
	{
	return float( getSampleRate() );
	}

float AudioBuffer::getLength() const
	{
	return getNumFrames() * frameToTime();
	}

float AudioBuffer::getMaxSampleMagnitude() const
	{
	if( getNumFrames() == 0 || getNumChannels() == 0 ) return 0;
	return std::abs(*std::max_element( buffer->begin(), buffer->end(), []( float a, float b )
		{
		return std::abs( a ) < std::abs( b );
		}));
	}

//======================================================
//	Setters
//======================================================
void AudioBuffer::setSample( size_t channel, size_t sample, float newValue ) 
	{
	(*buffer)[getBufferPos( channel, sample )] = newValue;
	}

float & AudioBuffer::getSample( size_t channel, size_t sample )
	{
	return (*buffer)[getBufferPos( channel, sample )];
	}

void xcdp::AudioBuffer::clearBuffer()
	{
	std::fill( buffer->begin(), buffer->end(), 0 );
	}

float * AudioBuffer::getSamplePointer( size_t channel, size_t frame )
	{ 
	return buffer->data() + getBufferPos( channel, frame );
	}

const float * AudioBuffer::getSamplePointer( size_t channel, size_t frame ) const
	{ 
	return buffer->data() + getBufferPos( channel, frame );
	}

//======================================================
//	Private
//======================================================

size_t AudioBuffer::getBufferPos( size_t channel, size_t sample ) const
	{
	return channel * getNumFrames() + sample;
	}

//======================================================
//	Global
//======================================================
std::ostream & operator<<( std::ostream & os, const AudioBuffer & audio )
	{
	os << "\n=========================== Audio Info ==========================="
	   << "\nChannels:\t"		<< audio.getNumChannels() 
	   << "\nSamples:\t"		<< audio.getNumFrames() 
	   << "\nSample Rate:\t"	<< audio.getSampleRate()
	   << "\n==================================================================" 
	   << "\n\n";
	return os;
	}

} // End namespace xcdp
