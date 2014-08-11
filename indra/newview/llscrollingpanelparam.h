/** 
 * @file llscrollingpanelparam.h
 * @brief the scrolling panel containing a list of visual param 
 *  	  panels
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_SCROLLINGPANELPARAM_H
#define LL_SCROLLINGPANELPARAM_H

#include "llscrollingpanelparambase.h"

class LLViewerJointMesh;
class LLViewerVisualParam;
class LLWearable;
class LLVisualParamHint;
class LLViewerVisualParam;
class LLJoint;

class LLScrollingPanelParam : public LLScrollingPanelParamBase
{
public:
	LLScrollingPanelParam( const std::string& name, LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLWearable* wearable, bool bVisualHint );
	virtual ~LLScrollingPanelParam();

	virtual void		draw();
	virtual void		setVisible( BOOL visible );
	virtual void		updatePanel(BOOL allow_modify);

	void				onHintMouseUp( bool max );
	void				onHintMouseDown( bool max );
	void				onHintHeldDown( bool max );

public:
	// Constants for LLPanelVisualParam
	const static F32 PARAM_STEP_TIME_THRESHOLD;
	
	const static S32 PARAM_HINT_WIDTH;
	const static S32 PARAM_HINT_HEIGHT;
	LLPointer<LLVisualParamHint>	mHintMin;
	LLPointer<LLVisualParamHint>	mHintMax;
	LLButton*						mLess;
	LLButton*						mMore;
	static S32 			sUpdateDelayFrames;
	
protected:
	LLTimer				mMouseDownTimer;	// timer for how long mouse has been held down on a hint.
	F32					mLastHeldTime;
private:
	LLView *mMinText, *mMaxText;
};


#endif
