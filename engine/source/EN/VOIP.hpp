#pragma once

#include <RakPeerInterface.h>

#include "speex/speex.h"
#include "speex/speex_preprocess.h"

class VOIP
{
public:

	VOIP(int sampleRate);
	~VOIP();

	int Encode(byte* inputBuffer, byte* outputBuffer, int frame_size);
	void Decode(byte* inputBuffer, byte* outputBuffer, int frame_size);

	int GetFrameSize(){return speexOutgoingFrameSampleCount;}


private:

	void Init();
	void Deinit();

	int _mSampleRate;
	void* _speexEncoder;
	void* _speexDecoder;
	SpeexPreprocessState * _speexProc;

	int speexOutgoingFrameSampleCount;
	int speexIncomingFrameSampleCount;

	SpeexBits _kbBits;
};