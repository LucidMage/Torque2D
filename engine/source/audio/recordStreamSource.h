//--------------------------------------------
// recordStreamSource.h
// header for recording audio via default recording device
//--------------------------------------------


#ifndef _RECORDSTREAMSOURCE_H_
#define _RECORDSTREAMSOURCE_H_

#ifndef _AUDIOSTREAMSOURCE_H_
#include "audio/audioStreamSource.h"
#endif

#include <list>

#define FREQ 16000   // Sample rate
#define CAP_SIZE 320*2 // How much to capture at a time (affects latency)


// Reads and writes per second of the sound data
// Speex only supports these 3 values
//#define SAMPLE_RATE  (8000)
#define SAMPLE_RATE  (16000)
//#define SAMPLE_RATE  (32000)

#define FRAMES_PER_BUFFER CAP_SIZE
//#define FRAMES_PER_BUFFER  (2048 / (32000 / SAMPLE_RATE))

class RecordStreamSource: public AudioStreamSource
{
public:
	RecordStreamSource();
	virtual ~RecordStreamSource();

	virtual bool initStream();
	virtual bool updateBuffers();
	virtual void freeStream();
	virtual F32 getElapsedTime();
	virtual F32 getTotalTime();

	F32 getMicLevel(){return mMicLevel;}

private:
	ALuint				    mBufferList[NUMBUFFERS];
	std::list<ALuint>		mBufferQueue;		
	S32						mNumBuffers;
	S32						mBufferSize;
	ALCdevice*				mInputDevice;

	S32						mBufferIdx;

	bool					bReady;
	bool					bFinished;

	ALenum  format;
	ALsizei size;
	ALsizei freq;

	ALuint			DataSize;
	ALuint			DataLeft;
	ALuint			dataStart;
	ALuint			buffersinqueue;

	bool			bBuffersAllocated;

	F32				mMicLevel;
	F32				mPreviousThreshold;
	SimTime			mThresholdHoldTime;

	void clear();
	void resetStream();

public:

	std::list<unsigned char*> mHoldingBuffer;
	std::list<unsigned char*> mThresholdBuffer;
	std::list<unsigned char*> mSoundBuffer;

	int mSoundBufferSize;
	bool mMuted;
	bool mEchoed;

	F32  mRecordGain;
	F32  mThreshold;

};

#endif // _RECORDSTREAMSOURCE_H_
