#ifndef _GUIMICLEVEL_H_
#define _GUIMICLEVEL_H_

#ifndef _GUICONTROL_H_
#include "gui/guiControl.h"
#endif

#include "audio/audio.h"

class GuiMicLevel : public GuiControl
{
private:
	typedef GuiControl Parent;

public:

	//creation methods
	DECLARE_CONOBJECT(GuiMicLevel);
	GuiMicLevel();
	static void initPersistFields();

	void onPreRender();
	void onRender(Point2I offset, const RectI &updateRect);
	void onStaticModified(const char* slotName, const char*newValue = NULL);

	void destroyMic();

	void toggleMic();

private:

	bool mEnabled;
	F32 mCurrentLevel;
	AssetPtr<AudioAsset>  mMicSound;
	AUDIOHANDLE			  mAudioHandle;
};

#endif