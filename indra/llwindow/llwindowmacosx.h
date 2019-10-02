/** 
 * @file llwindowmacosx.h
 * @brief Mac implementation of LLWindow class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLWINDOWMACOSX_H
#define LL_LLWINDOWMACOSX_H

#include "llwindow.h"
#include "llwindowcallbacks.h"
#include "llwindowmacosx-objc.h"

#include "lltimer.h"

#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>

// AssertMacros.h does bad things.
#include "fix_macros.h"
#undef verify
#undef require


class LLWindowMacOSX : public LLWindow
{
public:
	/*virtual*/ void show(bool focus = true);
	/*virtual*/ void hide();
	/*virtual*/ void close();
	/*virtual*/ BOOL getVisible();
	/*virtual*/ BOOL getMinimized();
	/*virtual*/ BOOL getMaximized();
	/*virtual*/ BOOL maximize();
	/*virtual*/ void minimize();
	/*virtual*/ void restore();
	/*virtual*/ BOOL getFullscreen();
	/*virtual*/ BOOL getPosition(LLCoordScreen *position);
	/*virtual*/ BOOL getSize(LLCoordScreen *size);
	/*virtual*/ BOOL getSize(LLCoordWindow *size);
	/*virtual*/ BOOL setPosition(LLCoordScreen position);
	/*virtual*/ BOOL setSizeImpl(LLCoordScreen size);
	/*virtual*/ BOOL setSizeImpl(LLCoordWindow size);
	/*virtual*/ BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, const S32 vsync_mode, std::function<void()> stopFn, std::function<void(bool)> restoreFn, const LLCoordScreen * const posp = nullptr);
	/*virtual*/ BOOL setCursorPosition(LLCoordWindow position);
	/*virtual*/ BOOL getCursorPosition(LLCoordWindow *position);
	/*virtual*/ void showCursor();
	/*virtual*/ void hideCursor();
	/*virtual*/ void showCursorFromMouseMove();
	/*virtual*/ void hideCursorUntilMouseMove();
	/*virtual*/ BOOL isCursorHidden();
	/*virtual*/ void updateCursor();
	/*virtual*/ ECursorType getCursor() const;
	/*virtual*/ void captureMouse();
	/*virtual*/ void releaseMouse();
	/*virtual*/ void setMouseClipping( BOOL b );
	/*virtual*/ BOOL isClipboardTextAvailable();
	/*virtual*/ BOOL pasteTextFromClipboard(LLWString &dst);
	/*virtual*/ BOOL copyTextToClipboard(const LLWString & src);
	/*virtual*/ void flashIcon(F32 seconds);
	/*virtual*/ F32 getGamma();
	/*virtual*/ BOOL setGamma(const F32 gamma); // Set the gamma
	/*virtual*/ U32 getFSAASamples();
	/*virtual*/ void setFSAASamples(const U32 fsaa_samples);
	/*virtual*/ BOOL restoreGamma();			// Restore original gamma table (before updating gamma)
	/*virtual*/ ESwapMethod getSwapMethod() { return mSwapMethod; }
	/*virtual*/ void gatherInput();
	/*virtual*/ void delayInputProcessing() {};
	/*virtual*/ void swapBuffers();
	
	// handy coordinate space conversion routines
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to);
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to);
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordGL *to);
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordWindow *to);
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordGL *to);
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordScreen *to);

	/*virtual*/ LLWindowResolution* getSupportedResolutions(S32 &num_resolutions);
	/*virtual*/ F32	getNativeAspectRatio();
	/*virtual*/ F32 getPixelAspectRatio();
	/*virtual*/ void setNativeAspectRatio(F32 ratio) { mOverrideAspectRatio = ratio; }

	/*virtual*/ void beforeDialog();
	/*virtual*/ void afterDialog();

	/*virtual*/ BOOL dialogColorPicker(F32 *r, F32 *g, F32 *b);

	/*virtual*/ void *getPlatformWindow();
	/*virtual*/ void bringToFront() {};
	
	/*virtual*/ void allowLanguageTextInput(LLPreeditor *preeditor, BOOL b);
	/*virtual*/ void interruptLanguageTextInput();
	/*virtual*/ void spawnWebBrowser(const std::string& escaped_url, bool async);
	/*virtual*/ F32 getScaleFactor();
	/*virtual*/ void updateUnreadCount(S32 num_conversations);

	static std::vector<std::string> getDynamicFallbackFontList();

	// Provide native key event data
	/*virtual*/ LLSD getNativeKeyData();
	
	void* getWindow() { return mWindow; }
	LLWindowCallbacks* getCallbacks() { return mCallbacks; }
	LLPreeditor* getPreeditor() { return mPreeditor; }
	
	void updateMouseDeltas(double* deltas);
	void getMouseDeltas(S32* delta);
	
	void handleDragNDrop(std::string url, LLWindowCallbacks::DragNDropAction action);
    
    bool allowsLanguageInput() { return mLanguageTextInputAllowed; }

protected:
	LLWindowMacOSX(LLWindowCallbacks* callbacks,
		const std::string& title, const std::string& name, int x, int y, int width, int height, U32 flags,
		BOOL fullscreen, BOOL clearBg, S32 vsync_setting,
		BOOL ignore_pixel_depth,
		U32 fsaa_samples);
	~LLWindowMacOSX();

	void	initCursors();
	BOOL	isValid();
	void	moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);


	// Changes display resolution. Returns true if successful
	BOOL	setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

	// Go back to last fullscreen display resolution.
	BOOL	setFullscreenResolution();

	// Restore the display resolution to its value before we ran the app.
	BOOL	resetDisplayResolution();

	BOOL	shouldPostQuit() { return mPostQuit; }

private:
    void restoreGLContext();

protected:
	//
	// Platform specific methods
	//

	// create or re-create the GL context/window.  Called from the constructor and switchContext().
	BOOL createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, S32 vsync_setting);
	void destroyContext();
	void setupFailure(const std::string& text, const std::string& caption, U32 type);
	void adjustCursorDecouple(bool warpingMouse = false);
	static MASK modifiersToMask(S16 modifiers);
	
#if LL_OS_DRAGDROP_ENABLED
	
	//static OSErr dragTrackingHandler(DragTrackingMessage message, WindowRef theWindow, void * handlerRefCon, DragRef theDrag);
	//static OSErr dragReceiveHandler(WindowRef theWindow, void * handlerRefCon,	DragRef theDrag);
	
	
#endif // LL_OS_DRAGDROP_ENABLED
	
	//
	// Platform specific variables
	//
	
	// Use generic pointers here.  This lets us do some funky Obj-C interop using Obj-C objects without having to worry about any compilation problems that may arise.
	NSWindowRef			mWindow;
	GLViewRef			mGLView;
	CGLContextObj		mContext;
	CGLPixelFormatObj	mPixelFormat;
	CGDirectDisplayID	mDisplay;
	
	LLRect		mOldMouseClip;  // Screen rect to which the mouse cursor was globally constrained before we changed it in clipMouse()
	std::string mWindowTitle;
	double		mOriginalAspectRatio;
	BOOL		mSimulatedRightClick;
	U32			mLastModifiers;
	BOOL		mHandsOffEvents;	// When true, temporarially disable CarbonEvent processing.
	// Used to allow event processing when putting up dialogs in fullscreen mode.
	BOOL		mCursorDecoupled;
	S32			mCursorLastEventDeltaX;
	S32			mCursorLastEventDeltaY;
	BOOL		mCursorIgnoreNextDelta;
	BOOL		mNeedsResize;		// Constructor figured out the window is too big, it needs a resize.
	LLCoordScreen   mNeedsResizeSize;
	F32			mOverrideAspectRatio;
	BOOL		mMaximized;
	BOOL		mMinimized;
	U32			mFSAASamples;
	BOOL		mForceRebuild;
	
	S32	mDragOverrideCursor;

	// Input method management through Text Service Manager.
	BOOL		mLanguageTextInputAllowed;
	LLPreeditor*	mPreeditor;
	
	static BOOL	sUseMultGL;

	friend class LLWindowManager;
	
};


class LLSplashScreenMacOSX : public LLSplashScreen
{
public:
	LLSplashScreenMacOSX();
	virtual ~LLSplashScreenMacOSX();

	/*virtual*/ void showImpl();
	/*virtual*/ void updateImpl(const std::string& mesg);
	/*virtual*/ void hideImpl();

private:
	WindowRef   mWindow;
};

S32 OSMessageBoxMacOSX(const std::string& text, const std::string& caption, U32 type);

void load_url_external(const char* url);

#endif //LL_LLWINDOWMACOSX_H
