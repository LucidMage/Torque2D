#include "VOIP.hpp"

VOIP::VOIP(int sampleRate)
{
	_mSampleRate = sampleRate;
	_speexDecoder = NULL;
	_speexEncoder = NULL;

	Init();
}

VOIP::~VOIP()
{
	Deinit();
}


void VOIP::Init()
{
	speex_bits_init(&_kbBits);

	if (_mSampleRate==8000)
	{
		_speexEncoder = speex_encoder_init(&speex_nb_mode);
		_speexDecoder = speex_decoder_init(&speex_nb_mode);
	}
	else if (_mSampleRate==16000)
	{
		_speexEncoder = speex_encoder_init(&speex_wb_mode);
		_speexDecoder = speex_decoder_init(&speex_wb_mode);
	}
	else // 32000
	{
		_speexEncoder = speex_encoder_init(&speex_uwb_mode);
		_speexDecoder = speex_decoder_init(&speex_uwb_mode);
	}

	int ret;
	ret = speex_encoder_ctl(_speexEncoder, SPEEX_GET_FRAME_SIZE, &speexOutgoingFrameSampleCount);
	ret = speex_decoder_ctl(_speexDecoder, SPEEX_GET_FRAME_SIZE, &speexIncomingFrameSampleCount);

	_speexProc= speex_preprocess_state_init(speexOutgoingFrameSampleCount, _mSampleRate);


	//setup options here.
	int flagOn = 1, flagOff = 0;
	float vbrQuality = 4.0f;

	//encoder	
	speex_encoder_ctl(_speexEncoder, SPEEX_SET_VAD, &flagOn);
	speex_encoder_ctl(_speexEncoder, SPEEX_SET_VBR, &flagOn);
	speex_encoder_ctl(_speexEncoder, SPEEX_SET_VBR_QUALITY, &vbrQuality);

	//decoder
	speex_decoder_ctl(_speexDecoder, SPEEX_SET_ENH, &flagOn); 

	//proc
	float start =0.10f, cont= 0.07f;

	speex_preprocess_ctl(_speexProc, SPEEX_PREPROCESS_SET_DENOISE, &flagOn);
	speex_preprocess_ctl(_speexProc, SPEEX_PREPROCESS_SET_VAD, &flagOn);
	speex_preprocess_ctl(_speexProc, SPEEX_PREPROCESS_SET_PROB_START, &start);
	speex_preprocess_ctl(_speexProc, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &cont);

}

void VOIP::Deinit()
{
	speex_encoder_destroy(_speexEncoder);
	speex_decoder_destroy(_speexDecoder);
	speex_preprocess_state_destroy(_speexProc);
	speex_bits_destroy(&_kbBits);
}

int VOIP::Encode(byte* inputBuffer, byte* outputBuffer, int frame_size)
{
	speex_bits_reset(&_kbBits);


	int chk = speex_preprocess(_speexProc,(short*)inputBuffer, NULL );
	if (chk == 0) return 0;

	speex_encode_int(_speexEncoder, (short*)inputBuffer, &_kbBits);

	return speex_bits_write(&_kbBits, (char*)outputBuffer, frame_size);
}

void VOIP::Decode(byte* inputBuffer, byte* outputBuffer, int input_size)
{
	SpeexBits bits;
	speex_bits_init(&bits);

	speex_bits_read_from(&bits,(char*)inputBuffer, input_size );

	speex_decode_int(_speexDecoder, &bits, (short*)outputBuffer);
	speex_bits_destroy(&bits);



}