#include "console/console.h"
#include "console/consoleTypes.h"
#include "graphics/dgl.h"


#include "guiMicLevel.h"

#include "audio/recordStreamSource.h"
#include "math/mMath.h"


IMPLEMENT_CONOBJECT(GuiMicLevel);


GuiMicLevel::GuiMicLevel()
{
	mCurrentLevel  = 0.0f;
	mMicSound	   = NULL;
	mAudioHandle   = NULL;
	mEnabled	   = false;
}

void GuiMicLevel::initPersistFields()
{
	Parent::initPersistFields();
	addField("MicAsset",       TypeAudioAssetPtr, Offset(mMicSound, GuiMicLevel));
}

void GuiMicLevel::onStaticModified(const char* slotName, const char*newValue /* = NULL */)
{
}

void GuiMicLevel::onPreRender()
{

	if (mMicSound == NULL || !mEnabled) return;

	RecordStreamSource* recordingSource = NULL;

	if (!alxIsValidHandle(mAudioHandle))
	{
		mAudioHandle = alxCreateSource(mMicSound);
		
	}

	alxPlay(mAudioHandle);
	Platform::sleep(10);

	recordingSource =  dynamic_cast<RecordStreamSource*>(alxFindAudioStreamSource(mAudioHandle));
	if (NULL != recordingSource)
	{
		recordingSource->mMuted = false;
		recordingSource->mEchoed = true;

		mCurrentLevel = recordingSource->getMicLevel();
	}
}

void GuiMicLevel::onRender(Point2I offset, const RectI &updateRect)
{
	RectI ctrlRect(offset, mBounds.extent);

	//draw the progress
	S32 width = (S32)((F32)mBounds.extent.x * mCurrentLevel);
	if (width > 0)
	{
		RectI progressRect = ctrlRect;
		progressRect.extent.x = width;
		dglDrawRectFill(progressRect, mProfile->mFillColor);
	}

	F32 displayValue = mCurrentLevel * 100;

	char mText[50];
	dSprintf(mText, 50, "     %.3f", displayValue );

	renderJustifiedText(offset, mBounds.extent, (char*)mText);


	//now draw the border
	if (mProfile->mBorder)
		dglDrawRect(ctrlRect, mProfile->mBorderColor);

	Parent::onRender( offset, updateRect );
}

void GuiMicLevel::destroyMic()
{
	if (alxIsValidHandle(mAudioHandle))
	{
		alxStop(mAudioHandle);
	}
}

void GuiMicLevel::toggleMic()
{
	mEnabled = !mEnabled;
}


ConsoleMethod( GuiMicLevel, destroyMic, void, 2, 2,  "")
{
	object->destroyMic();
}


ConsoleMethod( GuiMicLevel, toggleMic, void, 2, 2,  "")
{
	object->toggleMic();
}
