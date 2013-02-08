//--------------------------------------
// recordStreamSource.cc
// implementation of streaming audio source
//
// Bill Hilke
//--------------------------------------

#include "audio/recordStreamSource.h"

#define BUFFERSIZE 32768


RecordStreamSource::RecordStreamSource()  {
	mInputDevice = NULL;
	mBufferIdx = 0;
	bIsValid = false;
	mMuted = true;
	bBuffersAllocated = false;
	mBufferList[0] = 0;
	clear();

	mPosition = Point3F(0.f,0.f,0.f);
}

RecordStreamSource::~RecordStreamSource() {
	if(bReady && bIsValid)
		freeStream();
}

void RecordStreamSource::clear()
{
	mMuted			  = true;
	mBufferIdx		  = 0;
	mInputDevice	  = NULL;
	mHandle           = NULL_AUDIOHANDLE;
	mSource			  = NULL;
	mMicLevel		  = 0.0f;
	mPreviousThreshold =0.0f;
	mRecordGain		  = 1.0f;
	mThreshold		  = 5.0f / 100.0f;
	mThresholdHoldTime =0;

	dMemset(&mDescription, 0, sizeof(Audio::Description));
	mEnvironment = 0;
	mPosition.set(0.f,0.f,0.f);
	mDirection.set(0.f,1.f,0.f);
	mPitch = 1.f;
	mScore = 0.f;
	mCullTime = 0;
	mEchoed = false;
	bReady = false;
	bFinishedPlaying = false;
	bIsValid = false;
	bBuffersAllocated = false;
}

bool RecordStreamSource::initStream() {
	
	ALint			error;

	alGenBuffers(NUMBUFFERS, mBufferList);

	for(int i =0; i < NUMBUFFERS; ++i)
	{
		mBufferQueue.push_back(mBufferList[i]);
	}
	bBuffersAllocated = true;

	auto capDevice = Con::getVariable("pref::Audio::DefaultCaptureDevice");


	mInputDevice = alcCaptureOpenDevice(capDevice,FREQ,AL_FORMAT_MONO16,CAP_SIZE*2);
	if ((error = alGetError()) != AL_NO_ERROR) {
		return false;
	}

	alcCaptureStart(mInputDevice); // Begin capturing
	if ((error = alGetError()) != AL_NO_ERROR) {
		return false;
	}

	bIsValid = true;
	
	auto buf = Con::getReturnBuffer(15);
	dSprintf(buf, 15, "%d", mHandle);

	Con::setVariable("AudioMicHandle", buf);

	return true;
}

bool RecordStreamSource::updateBuffers() {

	ALint			processed;
	ALuint			BufferID;
	ALuint			buffHolder[NUMBUFFERS];

	// don't do anything if buffer isn't initialized
	if(!bIsValid)
		return false;

	ALint capturedSamples = 0;
	alcGetIntegerv(mInputDevice, ALC_CAPTURE_SAMPLES, 1, &capturedSamples);
	if (capturedSamples > CAP_SIZE) 
	{
		ALchar* Buffer = new ALchar[CAP_SIZE*2];
		alcCaptureSamples(mInputDevice, Buffer, CAP_SIZE);

		//make sure the avg level meets our minium.
		float avg = 0.0f;
		short * shtBuffer = (short*)Buffer;
		for(short x =0; x < CAP_SIZE; x++)
		{
			int s = ((ALshort*)shtBuffer)[x] * (1.2f * mRecordGain);
			if(s < -32767) s = -32767;
			else if(s > 32767) s = 32767;
			((ALshort*)shtBuffer)[x] = s;

			short sample = shtBuffer[x];
			avg += sample * sample;
		}
		avg /= CAP_SIZE;
		avg = sqrt(avg) / 2000.0f;

		mMicLevel = avg;

		std::list<unsigned char*>* bufferToUse = &mHoldingBuffer;

		if (!mMuted && (mMicLevel > mThreshold || (Sim::getCurrentTime() - mThresholdHoldTime) < 400 )  )
		{
			if (mMicLevel > mThreshold)
			{
				mThresholdHoldTime = Sim::getCurrentTime();
			}

			while(mThresholdBuffer.size() > 0)
			{
				auto buffer = mThresholdBuffer.front();
				bufferToUse->push_back(buffer);
				mThresholdBuffer.pop_front();
			}

			bufferToUse->push_back((unsigned char*)Buffer);

			if (mEchoed)
			{
				ALchar* EchoBuffer = new ALchar[CAP_SIZE*2];
				memcpy(EchoBuffer, Buffer, CAP_SIZE*2);
				mSoundBuffer.push_back((unsigned char*)EchoBuffer);
			}

			if (mHoldingBuffer.size() > 10)
			{
				auto buf = mHoldingBuffer.front();
				delete buf;
				mHoldingBuffer.pop_front();
			}
		}
		else if (!mMuted && mMicLevel > mPreviousThreshold)
		{
			//we store a bit of time into a temp buffer so that if we do hit our threshold, we can slam the old data in first.
			//this provides a better mic on experience.

			mThresholdBuffer.push_back( (unsigned char*)Buffer);

			if (mThresholdBuffer.size() > 5)
			{
				auto buf = mThresholdBuffer.front();
				delete buf;
				mThresholdBuffer.pop_front();
			}
		}
		else
		{
			mThresholdHoldTime = 0;

			while(bufferToUse->size() > 0)
			{
				auto buffer = bufferToUse->front();
				bufferToUse->pop_front();
			}


			while(mThresholdBuffer.size() > 0)
			{
				auto buffer = mThresholdBuffer.front();
				mThresholdBuffer.pop_front();
			}

			delete[] Buffer;
		}

		mPreviousThreshold = mMicLevel;

	}

	alGetSourcei(mSource,AL_BUFFERS_PROCESSED,&processed);
	if (processed>0) {
		alSourceUnqueueBuffers(mSource,processed,buffHolder);
		for (int ii=0;ii<processed;++ii) {
			mBufferQueue.push_back(buffHolder[ii]);
		}
	}

	std::list<unsigned char*>::size_type waitSize = 2;
	char* soundBuffer = NULL;
	if (mSoundBuffer.size() > waitSize && !mBufferQueue.empty())
	{
		soundBuffer = new char[ (FRAMES_PER_BUFFER*2) * waitSize];
		for(std::list<unsigned char*>::size_type x=0; x < waitSize; x++)
		{
			auto data = mSoundBuffer.front(); mSoundBuffer.pop_front(); 
			memcpy(soundBuffer + (x*(FRAMES_PER_BUFFER*2) ), data, (FRAMES_PER_BUFFER*2) );
			delete[] data;
		}
	}

	if (soundBuffer)
	{
		BufferID = mBufferQueue.front(); mBufferQueue.pop_front();
		alBufferData(BufferID,AL_FORMAT_MONO16,soundBuffer,(FRAMES_PER_BUFFER*2) * waitSize,FREQ);
		alSourceQueueBuffers(mSource,1,&BufferID);
		delete[] soundBuffer;

		ALint sState=0;
		alGetSourcei(mSource,AL_SOURCE_STATE,&sState);

		if (sState!=AL_PLAYING) 
		{
			alSourcePlay(mSource);
//				ALint iQueuedBuffers;
// 				alGetSourcei(mSource, AL_BUFFERS_QUEUED, &iQueuedBuffers);
// 				if (iQueuedBuffers)
// 					alSourcePlay(mSource);
		}


	}



	// reset AL error code
	alGetError();

	return AL_TRUE;
}

void RecordStreamSource::freeStream() {
	bReady = false;

	alcCaptureStop(mInputDevice);
	alcCaptureCloseDevice(mInputDevice);

	if(bBuffersAllocated) {
		if(mBufferList[0] != 0)
			alDeleteBuffers(NUMBUFFERS, mBufferList);
		for(int i = 0; i < NUMBUFFERS; i++)
			mBufferList[i] = 0;

		mBufferQueue.clear();
		bBuffersAllocated = false;
	}

	while(mHoldingBuffer.size() > 0)
	{
		auto b = mHoldingBuffer.front();
		delete b;
		mHoldingBuffer.pop_front();
	}

	while(mSoundBuffer.size() > 0)
	{
		auto b = mSoundBuffer.front();
		delete b;
		mSoundBuffer.pop_front();
	}

	while(mThresholdBuffer.size() > 0)
	{
		auto b = mThresholdBuffer.front();
		delete b;
		mThresholdBuffer.pop_front();
	}

}

void RecordStreamSource::resetStream() {
}

#include "console/console.h"

F32 RecordStreamSource::getElapsedTime()
{
	Con::warnf( "GetElapsedTime not implemented in RecordStream yet" );
	return -1.f;
}

F32 RecordStreamSource::getTotalTime()
{
	Con::warnf( "GetTotalTime not implemented in RecordStream yet" );
	return -1.f;
}




ConsoleFunctionGroupBegin(AudioCapture, "Functions dealing with the OpenAL recording audio layer.\n\n"
	"@see www.OpenAL.org for what these functions do. Variances from posted"
	"     behaviour is described below.");

ConsoleFunction(setMicrophoneVolume, void, 3, 3, "(captureHandle, gain)")
{
	AUDIOHANDLE handle = dAtoi(argv[1]);
	if(handle == NULL_AUDIOHANDLE)
		return;

	RecordStreamSource* source = (RecordStreamSource*)alxFindAudioStreamSource(handle);
	if ( source )
	{
		source->mRecordGain = dAtof(argv[2]);
	}
}

ConsoleFunction(SetMicrophoneThreshold, void, 3, 3, "(captureHandle, gain)")
{
	AUDIOHANDLE handle = dAtoi(argv[1]);
	if(handle == NULL_AUDIOHANDLE)
		return;

	RecordStreamSource* source = (RecordStreamSource*)alxFindAudioStreamSource(handle);
	if ( source )
	{
		source->mThreshold = dAtof(argv[2]);
	}
}

ConsoleFunction(SetMicrophoneEcho, void, 3, 3, "(captureHandle, gain)")
{
	AUDIOHANDLE handle = dAtoi(argv[1]);
	if(handle == NULL_AUDIOHANDLE)
		return;

	RecordStreamSource* source = (RecordStreamSource*)alxFindAudioStreamSource(handle);
	if ( source )
	{
		source->mEchoed = dAtof(argv[2]);
	}
}