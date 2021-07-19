/**
 * @file llwindowmacosx-objc.mm
 * @brief Definition of functions shared between llwindowmacosx.cpp
 * and llwindowmacosx-objc.mm.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#import "llopenglview-objc.h"
#import "llwindowmacosx-objc.h"
#import "llappdelegate-objc.h"

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

/*
 * These functions are broken out into a separate file because the
 * objective-C typedef for 'BOOL' conflicts with the one in
 * llcommon/stdtypes.h.  This makes it impossible to use the standard
 * linden headers with any objective-C++ source.
 */

namespace LLDarwin {

int createNSApp(int argc, const char *argv[])
{
	return NSApplicationMain(argc, argv);
}

void setupCocoa()
{
	static bool inited = false;
	
	if(!inited)
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		
		// The following prevents the Cocoa command line parser from trying to open 'unknown' arguements as documents.
		// ie. running './secondlife -set Language fr' would cause a pop-up saying can't open document 'fr'
		// when init'ing the Cocoa App window.
		[[NSUserDefaults standardUserDefaults] setObject:@"NO" forKey:@"NSTreatUnknownArgumentsAsOpen"];
		
		[pool release];

		inited = true;
	}
}

bool copyToPBoard(const unsigned short *str, unsigned int len)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];
	NSPasteboard *pboard = [NSPasteboard generalPasteboard];
	[pboard clearContents];
	
	NSArray *contentsToPaste = [[[NSArray alloc] initWithObjects:[NSString stringWithCharacters:str length:len], nil] autorelease];
	BOOL ret = [pboard writeObjects:contentsToPaste];
	[pool release];
	return ret;
}

bool pasteBoardAvailable()
{
	NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
	return [[NSPasteboard generalPasteboard] canReadObjectForClasses:classArray options:[NSDictionary dictionary]];
}

const unsigned short *copyFromPBoard()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];
	NSPasteboard *pboard = [NSPasteboard generalPasteboard];
	NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
	NSString *str = NULL;
	BOOL ok = [pboard canReadObjectForClasses:classArray options:[NSDictionary dictionary]];
	if (ok)
	{
		NSArray *objToPaste = [pboard readObjectsForClasses:classArray options:[NSDictionary dictionary]];
		str = [objToPaste objectAtIndex:0];
	}
	NSUInteger len = [str length];
	unichar* temp = (unichar*)calloc(len+1, sizeof(unichar));
	[str getCharacters:temp range:NSMakeRange(0, len)];

	[pool release];
	return temp;
}

CursorRef createImageCursor(const char *fullpath, int hotspotX, int hotspotY)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	// extra retain on the NSCursor since we want it to live for the lifetime of the app.
	NSCursor *cursor =
	[[[NSCursor alloc]
	  initWithImage:
	  [[[NSImage alloc] initWithContentsOfFile:
		[NSString stringWithFormat:@"%s", fullpath]
		]autorelease]
	  hotSpot:NSMakePoint(hotspotX, hotspotY)
	  ]retain];
	
	[pool release];
	
	return (CursorRef)cursor;
}

void setArrowCursor()
{
	NSCursor *cursor = [NSCursor arrowCursor];
	[NSCursor unhide];
	[cursor set];
}

void setIBeamCursor()
{
	NSCursor *cursor = [NSCursor IBeamCursor];
	[cursor set];
}

void setPointingHandCursor()
{
	NSCursor *cursor = [NSCursor pointingHandCursor];
	[cursor set];
}

void setCopyCursor()
{
	NSCursor *cursor = [NSCursor dragCopyCursor];
	[cursor set];
}

void setCrossCursor()
{
	NSCursor *cursor = [NSCursor crosshairCursor];
	[cursor set];
}

void setNotAllowedCursor()
{
	NSCursor *cursor = [NSCursor operationNotAllowedCursor];
	[cursor set];
}

void hideNSCursor()
{
	[NSCursor hide];
}

void showNSCursor()
{
	[NSCursor unhide];
}

void hideNSCursorTillMove(bool hide)
{
	[NSCursor setHiddenUntilMouseMoves:hide];
}

// This is currently unused, since we want all our cursors to persist for the life of the app, but I've included it for completeness.
OSErr releaseImageCursor(CursorRef ref)
{
	if( ref != NULL )
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSCursor *cursor = (NSCursor*)ref;
		[cursor release];
		[pool release];
	}
	else
	{
		return paramErr;
	}
	
	return noErr;
}

OSErr setImageCursor(CursorRef ref)
{
	if( ref != NULL )
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSCursor *cursor = (NSCursor*)ref;
		[cursor set];
		[pool release];
	}
	else
	{
		return paramErr;
	}
	
	return noErr;
}

// Now for some unholy juggling between generic pointers and casting them to Obj-C objects!
// Note: things can get a bit hairy from here.  This is not for the faint of heart.

NSWindowRef createNSWindow(int x, int y, int width, int height)
{
	LLNSWindow *window = [[LLNSWindow alloc]initWithContentRect:NSMakeRect(x, y, width, height)
													  styleMask:NSTitledWindowMask | NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSTexturedBackgroundWindowMask backing:NSBackingStoreBuffered defer:NO];
	[window makeKeyAndOrderFront:nil];
	[window setAcceptsMouseMovedEvents:TRUE];
	return window;
}

GLViewRef createOpenGLView(NSWindowRef window, unsigned int samples, bool vsync)
{
	LLOpenGLView *glview = [[LLOpenGLView alloc]initWithFrame:[(LLNSWindow*)window frame] withSamples:samples andVsync:vsync];
	[(LLNSWindow*)window setContentView:glview];
	return glview;
}

void glSwapBuffers(void* context)
{
	[(NSOpenGLContext*)context flushBuffer];
}

CGLContextObj getCGLContextObj(GLViewRef view)
{
	return [(LLOpenGLView *)view getCGLContextObj];
}

CGLPixelFormatObj* getCGLPixelFormatObj(NSWindowRef window)
{
	LLOpenGLView *glview = [(LLNSWindow*)window contentView];
	return [glview getCGLPixelFormatObj];
}

unsigned long getVramSize(GLViewRef view)
{
	return [(LLOpenGLView *)view getVramSize];
}

void getContentViewBounds(NSWindowRef window, float* bounds)
{
	bounds[0] = [[(LLNSWindow*)window contentView] bounds].origin.x;
	bounds[1] = [[(LLNSWindow*)window contentView] bounds].origin.y;
	bounds[2] = [[(LLNSWindow*)window contentView] bounds].size.width;
	bounds[3] = [[(LLNSWindow*)window contentView] bounds].size.height;
}

void getScaledContentViewBounds(NSWindowRef window, GLViewRef view, float* bounds)
{
    NSRect b = [(NSOpenGLView*)view convertRectToBacking:[[(LLNSWindow*)window contentView] bounds]];
	bounds[0] = b.origin.x;
	bounds[1] = b.origin.y;
	bounds[2] = b.size.width;
	bounds[3] = b.size.height;
}

void getWindowSize(NSWindowRef window, float* size)
{
	NSRect frame = [(LLNSWindow*)window frame];
	size[0] = frame.origin.x;
	size[1] = frame.origin.y;
	size[2] = frame.size.width;
	size[3] = frame.size.height;
}

void setWindowSize(NSWindowRef window, int width, int height)
{
	NSRect frame = [(LLNSWindow*)window frame];
	frame.size.width = width;
	frame.size.height = height;
	[(LLNSWindow*)window setFrame:frame display:TRUE];
}

void setWindowPos(NSWindowRef window, float* pos)
{
	NSPoint point;
	point.x = pos[0];
	point.y = pos[1];
	[(LLNSWindow*)window setFrameOrigin:point];
}

void getCursorPos(NSWindowRef window, float* pos)
{
	NSPoint mLoc;
	mLoc = [(LLNSWindow*)window mouseLocationOutsideOfEventStream];
	pos[0] = mLoc.x;
	pos[1] = mLoc.y;
}

void makeWindowOrderFront(NSWindowRef window)
{
	[(LLNSWindow*)window makeKeyAndOrderFront:nil];
}

void convertScreenToWindow(NSWindowRef window, float *coord)
{
	NSRect point = NSMakeRect(coord[0], coord[1], 0, 0);
	point = [(LLNSWindow*)window convertRectFromScreen:point];
	coord[0] = point.origin.x;
	coord[1] = point.origin.y;
}

void convertRectToScreen(NSWindowRef window, float *coord)
{
	NSRect point = NSMakeRect(coord[0], coord[1], coord[2], coord[3]);
	point = [(LLNSWindow*)window convertRectToScreen:point];
	
	coord[0] = point.origin.x;
	coord[1] = point.origin.y;
	coord[2] = point.size.width;
	coord[3] = point.size.height;
}

void convertRectFromScreen(NSWindowRef window, float *coord)
{
	NSRect point = NSMakeRect(coord[0], coord[1], coord[2], coord[3]);
	point = [(LLNSWindow*)window convertRectFromScreen:point];
	
	coord[0] = point.origin.x;
	coord[1] = point.origin.y;
	coord[2] = point.size.width;
	coord[3] = point.size.height;
}

void convertWindowToScreen(NSWindowRef window, float *coord)
{
	LLNSWindow *nsWindow = (LLNSWindow*)window;
	NSRect rect = NSMakeRect(coord[0], coord[1], 0, 0);
	rect = [nsWindow convertRectToScreen:rect];
	NSRect screenRect = [[nsWindow screen] frame];
	NSPoint retPoint = NSMakePoint(rect.origin.x, screenRect.origin.y + screenRect.size.height - rect.origin.y);
	coord[0] = retPoint.x;
	coord[1] = retPoint.y;
}

void closeWindow(NSWindowRef window)
{
	[(LLNSWindow*)window close];
	[(LLNSWindow*)window release];
}

void removeGLView(GLViewRef view)
{
	[(LLOpenGLView*)view removeFromSuperview];
	[(LLOpenGLView*)view release];
}

void setupInputWindow(NSWindowRef window, GLViewRef glview)
{
	[[(LLAppDelegate*)[NSApp delegate] inputView] setGLView:(LLOpenGLView*)glview];
}

void commitCurrentPreedit(GLViewRef glView)
{
	[(LLOpenGLView*)glView commitCurrentPreedit];
}

void allowDirectMarkedTextInput(bool allow, GLViewRef glView)
{
    [(LLOpenGLView*)glView allowMarkedTextInput:allow];
}

NSWindowRef getMainAppWindow()
{
	LLNSWindow *winRef = [(LLAppDelegate*)[[LLNSApplication sharedApplication] delegate] window];
	
	[winRef setAcceptsMouseMovedEvents:TRUE];
	return winRef;
}

void makeFirstResponder(NSWindowRef window, GLViewRef view)
{
	[(LLNSWindow*)window makeFirstResponder:(LLOpenGLView*)view];
}

void requestUserAttention()
{
	[[LLNSApplication sharedApplication] requestUserAttention:NSInformationalRequest];
}

long showAlert(std::string text, std::string title, int type)
{
    NSAlert *alert = [[NSAlert alloc] init];
    
    [alert setMessageText:[NSString stringWithCString:title.c_str() encoding:[NSString defaultCStringEncoding]]];
    [alert setInformativeText:[NSString stringWithCString:text.c_str() encoding:[NSString defaultCStringEncoding]]];
    if (type == 0)
    {
        [alert addButtonWithTitle:@"Okay"];
    } else if (type == 1)
    {
        [alert addButtonWithTitle:@"Okay"];
        [alert addButtonWithTitle:@"Cancel"];
    } else if (type == 2)
    {
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
    }
    long ret = [alert runModal];
    [alert dealloc];
    
    if (ret == NSAlertFirstButtonReturn)
    {
        if (type == 1)
        {
            ret = 3;
        } else if (type == 2)
        {
            ret = 0;
        }
    } else if (ret == NSAlertSecondButtonReturn)
    {
        if (type == 0 || type == 1)
        {
            ret = 2;
        } else if (type == 2)
        {
            ret = 1;
        }
    }
    
    return ret;
}

/*
 GLViewRef getGLView()
 {
 return [(LLAppDelegate*)[[LLNSApplication sharedApplication] delegate] glview];
 }
 */

unsigned int getModifiers()
{
	return [NSEvent modifierFlags];
}

// [CR:Retina]
float getScaleFactor(GLViewRef view)
{
    return [[(LLOpenGLView*)view window] backingScaleFactor];
}

void updateBadge(int count)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSDockTile *tile = [[LLNSApplication sharedApplication] dockTile];
	if (count > 0) {
		NSString *value = [NSString stringWithFormat:@"%d",count];
		[tile setBadgeLabel:value];
	} else {
		[tile setBadgeLabel:nil];
	}
	[pool release];
}

void setTitle(const std::string& title)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	LLNSWindow *winRef = [(LLAppDelegate*)[[LLNSApplication sharedApplication] delegate] window];
	NSString *nsTitle = [NSString stringWithUTF8String:title.c_str()];
	[winRef setTitle:nsTitle];
	[pool release];
}

} // namespace LLDarwin