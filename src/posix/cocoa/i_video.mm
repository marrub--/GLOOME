/*
 ** i_video.mm
 **
 **---------------------------------------------------------------------------
 ** Copyright 2012-2015 Alexey Lysiuk
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions
 ** are met:
 **
 ** 1. Redistributions of source code must retain the above copyright
 **    notice, this list of conditions and the following disclaimer.
 ** 2. Redistributions in binary form must reproduce the above copyright
 **    notice, this list of conditions and the following disclaimer in the
 **    documentation and/or other materials provided with the distribution.
 ** 3. The name of the author may not be used to endorse or promote products
 **    derived from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 ** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 ** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 ** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 ** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 ** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 ** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **---------------------------------------------------------------------------
 **
 */

#include "i_common.h"

#import <Carbon/Carbon.h>
#import <OpenGL/gl.h>

// Avoid collision between DObject class and Objective-C
#define Class ObjectClass

#include "bitmap.h"
#include "c_dispatch.h"
#include "doomstat.h"
#include "hardware.h"
#include "i_system.h"
#include "m_argv.h"
#include "r_renderer.h"
#include "r_swrenderer.h"
#include "stats.h"
#include "textures.h"
#include "v_palette.h"
#include "v_pfx.h"
#include "v_text.h"
#include "v_video.h"
#include "version.h"

#undef Class


EXTERN_CVAR(Bool, ticker   )
EXTERN_CVAR(Bool, vid_vsync)
EXTERN_CVAR(Bool, vid_hidpi)

CUSTOM_CVAR(Bool, fullscreen, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	extern int NewWidth, NewHeight, NewBits, DisplayBits;

	NewWidth      = screen->GetWidth();
	NewHeight     = screen->GetHeight();
	NewBits       = DisplayBits;
	setmodeneeded = true;
}

CUSTOM_CVAR(Bool, vid_autoswitch, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL)
{
	Printf("You must restart " GAMENAME " to apply graphics switching mode\n");
}


RenderBufferOptions rbOpts;


// ---------------------------------------------------------------------------


namespace
{
	const NSInteger LEVEL_FULLSCREEN = NSMainMenuWindowLevel + 1;
	const NSInteger LEVEL_WINDOWED   = NSNormalWindowLevel;

	const NSUInteger STYLE_MASK_FULLSCREEN = NSBorderlessWindowMask;
	const NSUInteger STYLE_MASK_WINDOWED   = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
}


// ---------------------------------------------------------------------------


@interface CocoaWindow : NSWindow
{
}

- (BOOL)canBecomeKeyWindow;

@end


@implementation CocoaWindow

- (BOOL)canBecomeKeyWindow
{
	return true;
}

@end


// ---------------------------------------------------------------------------


@interface CocoaView : NSOpenGLView
{
	NSCursor* m_cursor;
}

- (void)resetCursorRects;

- (void)setCursor:(NSCursor*)cursor;

@end


@implementation CocoaView

- (void)resetCursorRects
{
	[super resetCursorRects];

	NSCursor* const cursor = nil == m_cursor
		? [NSCursor arrowCursor]
		: m_cursor;

	[self addCursorRect:[self bounds]
				 cursor:cursor];
}

- (void)setCursor:(NSCursor*)cursor
{
	m_cursor = cursor;
}

@end



// ---------------------------------------------------------------------------


class CocoaVideo : public IVideo
{
public:
	explicit CocoaVideo(int multisample);

	virtual EDisplayType GetDisplayType() { return DISPLAY_Both; }
	virtual void SetWindowedScale(float scale);

	virtual DFrameBuffer* CreateFrameBuffer(int width, int height, bool fs, DFrameBuffer* old);

	virtual void StartModeIterator(int bits, bool fullscreen);
	virtual bool NextMode(int* width, int* height, bool* letterbox);

	static bool IsFullscreen();
	static void UseHiDPI(bool hiDPI);
	static void SetCursor(NSCursor* cursor);
	static void SetWindowVisible(bool visible);

private:
	struct ModeIterator
	{
		size_t index;
		int    bits;
		bool   fullscreen;
	};

	ModeIterator m_modeIterator;

	CocoaWindow* m_window;

	int  m_width;
	int  m_height;
	bool m_fullscreen;
	bool m_hiDPI;

	void SetStyleMask(NSUInteger styleMask);
	void SetFullscreenMode(int width, int height);
	void SetWindowedMode(int width, int height);
	void SetMode(int width, int height, bool fullscreen, bool hiDPI);

	static CocoaVideo* GetInstance();
};


class CocoaFrameBuffer : public DFrameBuffer
{
public:
	CocoaFrameBuffer(int width, int height, bool fullscreen);
	~CocoaFrameBuffer();

	virtual bool Lock(bool buffer);
	virtual void Unlock();
	virtual void Update();

	virtual PalEntry* GetPalette();
	virtual void GetFlashedPalette(PalEntry pal[256]);
	virtual void UpdatePalette();

	virtual bool SetGamma(float gamma);
	virtual bool SetFlash(PalEntry  rgb, int  amount);
	virtual void GetFlash(PalEntry &rgb, int &amount);

	virtual int GetPageCount();

	virtual bool IsFullscreen();

	virtual void SetVSync(bool vsync);

private:
	static const size_t BYTES_PER_PIXEL = 4;

	PalEntry m_palette[256];
	bool     m_needPaletteUpdate;

	BYTE     m_gammaTable[3][256];
	float    m_gamma;
	bool     m_needGammaUpdate;

	PalEntry m_flashColor;
	int      m_flashAmount;

	bool     m_isUpdatePending;

	uint8_t* m_pixelBuffer;
	GLuint   m_texture;

	void Flip();

	void UpdateColors();
};


// ---------------------------------------------------------------------------


EXTERN_CVAR(Float, Gamma)

CUSTOM_CVAR(Float, rgamma, 1.0f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (NULL != screen)
	{
		screen->SetGamma(Gamma);
	}
}

CUSTOM_CVAR(Float, ggamma, 1.0f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (NULL != screen)
	{
		screen->SetGamma(Gamma);
	}
}

CUSTOM_CVAR(Float, bgamma, 1.0f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (NULL != screen)
	{
		screen->SetGamma(Gamma);
	}
}


// ---------------------------------------------------------------------------


extern id appCtrl;


namespace
{

const struct
{
	uint16_t width;
	uint16_t height;
}
VideoModes[] =
{
	{  320,  200 },
	{  320,  240 },
	{  400,  225 },	// 16:9
	{  400,  300 },
	{  480,  270 },	// 16:9
	{  480,  360 },
	{  512,  288 },	// 16:9
	{  512,  384 },
	{  640,  360 },	// 16:9
	{  640,  400 },
	{  640,  480 },
	{  720,  480 },	// 16:10
	{  720,  540 },
	{  800,  450 },	// 16:9
	{  800,  480 },
	{  800,  500 },	// 16:10
	{  800,  600 },
	{  848,  480 },	// 16:9
	{  960,  600 },	// 16:10
	{  960,  720 },
	{ 1024,  576 },	// 16:9
	{ 1024,  600 },	// 17:10
	{ 1024,  640 },	// 16:10
	{ 1024,  768 },
	{ 1088,  612 },	// 16:9
	{ 1152,  648 },	// 16:9
	{ 1152,  720 },	// 16:10
	{ 1152,  864 },
	{ 1280,  720 },	// 16:9
	{ 1280,  854 },
	{ 1280,  800 },	// 16:10
	{ 1280,  960 },
	{ 1280, 1024 },	// 5:4
	{ 1360,  768 },	// 16:9
	{ 1366,  768 },
	{ 1400,  787 },	// 16:9
	{ 1400,  875 },	// 16:10
	{ 1400, 1050 },
	{ 1440,  900 },
	{ 1440,  960 },
	{ 1440, 1080 },
	{ 1600,  900 },	// 16:9
	{ 1600, 1000 },	// 16:10
	{ 1600, 1200 },
	{ 1680, 1050 },	// 16:10
	{ 1920, 1080 },
	{ 1920, 1200 },
	{ 2048, 1536 },
	{ 2304, 1440 },
	{ 2560, 1440 },
	{ 2560, 1600 },
	{ 2560, 2048 },
	{ 2880, 1800 },
	{ 3200, 1800 },
	{ 3840, 2160 },
	{ 3840, 2400 },
	{ 4096, 2160 },
	{ 5120, 2880 }
};


cycle_t BlitCycles;
cycle_t FlipCycles;


CocoaWindow* CreateCocoaWindow(const NSUInteger styleMask)
{
	static const CGFloat TEMP_WIDTH  = VideoModes[0].width  - 1;
	static const CGFloat TEMP_HEIGHT = VideoModes[0].height - 1;

	CocoaWindow* const window = [CocoaWindow alloc];
	[window initWithContentRect:NSMakeRect(0, 0, TEMP_WIDTH, TEMP_HEIGHT)
					  styleMask:styleMask
						backing:NSBackingStoreBuffered
						  defer:NO];
	[window setOpaque:YES];
	[window makeFirstResponder:appCtrl];
	[window setAcceptsMouseMovedEvents:YES];

	return window;
}

} // unnamed namespace


// ---------------------------------------------------------------------------


CocoaVideo::CocoaVideo(const int multisample)
: m_window(CreateCocoaWindow(STYLE_MASK_WINDOWED))
, m_width(-1)
, m_height(-1)
, m_fullscreen(false)
, m_hiDPI(false)
{
	memset(&m_modeIterator, 0, sizeof m_modeIterator);

	// Set attributes for OpenGL context

	NSOpenGLPixelFormatAttribute attributes[16];
	size_t i = 0;

	attributes[i++] = NSOpenGLPFADoubleBuffer;
	attributes[i++] = NSOpenGLPFAColorSize;
	attributes[i++] = NSOpenGLPixelFormatAttribute(32);
	attributes[i++] = NSOpenGLPFADepthSize;
	attributes[i++] = NSOpenGLPixelFormatAttribute(24);
	attributes[i++] = NSOpenGLPFAStencilSize;
	attributes[i++] = NSOpenGLPixelFormatAttribute(8);

	if (!vid_autoswitch)
	{
		attributes[i++] = NSOpenGLPFAAllowOfflineRenderers;
	}

	if (multisample)
	{
		attributes[i++] = NSOpenGLPFAMultisample;
		attributes[i++] = NSOpenGLPFASampleBuffers;
		attributes[i++] = NSOpenGLPixelFormatAttribute(1);
		attributes[i++] = NSOpenGLPFASamples;
		attributes[i++] = NSOpenGLPixelFormatAttribute(multisample);
	}

	attributes[i] = NSOpenGLPixelFormatAttribute(0);

	// Create OpenGL context and view

	NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];

	const NSRect contentRect = [m_window contentRectForFrameRect:[m_window frame]];
	NSOpenGLView* glView = [[CocoaView alloc] initWithFrame:contentRect
												pixelFormat:pixelFormat];
	[[glView openGLContext] makeCurrentContext];

	[m_window setContentView:glView];
}

void CocoaVideo::StartModeIterator(const int bits, const bool fullscreen)
{
	m_modeIterator.index      = 0;
	m_modeIterator.bits       = bits;
	m_modeIterator.fullscreen = fullscreen;
}

bool CocoaVideo::NextMode(int* const width, int* const height, bool* const letterbox)
{
	assert(NULL != width);
	assert(NULL != height);

	const int bits = m_modeIterator.bits;

	if (8 != bits && 16 != bits && 24 != bits && 32 != bits)
	{
		return false;
	}

	size_t& index = m_modeIterator.index;

	if (index < sizeof(VideoModes) / sizeof(VideoModes[0]))
	{
		*width  = VideoModes[index].width;
		*height = VideoModes[index].height;

		if (m_modeIterator.fullscreen && NULL != letterbox)
		{
			const NSSize screenSize  = [[m_window screen] frame].size;
			const float  screenRatio = screenSize.width / screenSize.height;
			const float  modeRatio   = float(*width) / *height;

			*letterbox = fabs(screenRatio - modeRatio) > 0.001f;
		}

		++index;

		return true;
	}

	return false;
}

DFrameBuffer* CocoaVideo::CreateFrameBuffer(const int width, const int height, const bool fullscreen, DFrameBuffer* const old)
{
	PalEntry flashColor  = 0;
	int      flashAmount = 0;

	if (NULL != old)
	{
		if (width == m_width && height == m_height)
		{
			SetMode(width, height, fullscreen, vid_hidpi);
			return old;
		}

		old->GetFlash(flashColor, flashAmount);
		old->ObjectFlags |= OF_YesReallyDelete;

		if (old == screen)
		{
			screen = NULL;
		}

		delete old;
	}

	CocoaFrameBuffer* fb = new CocoaFrameBuffer(width, height, fullscreen);
	fb->SetFlash(flashColor, flashAmount);

	SetMode(width, height, fullscreen, vid_hidpi);

	return fb;
}

void CocoaVideo::SetWindowedScale(float scale)
{
}


bool CocoaVideo::IsFullscreen()
{
	CocoaVideo* const video = GetInstance();
	return NULL == video
		? false
		: video->m_fullscreen;
}

void CocoaVideo::UseHiDPI(const bool hiDPI)
{
	if (CocoaVideo* const video = GetInstance())
	{
		video->SetMode(video->m_width, video->m_height, video->m_fullscreen, hiDPI);
	}
}

void CocoaVideo::SetCursor(NSCursor* cursor)
{
	if (CocoaVideo* const video = GetInstance())
	{
		NSWindow*  const window = video->m_window;
		CocoaView* const view   = [window contentView];

		[view setCursor:cursor];
		[window invalidateCursorRectsForView:view];
	}
}

void CocoaVideo::SetWindowVisible(bool visible)
{
	if (CocoaVideo* const video = GetInstance())
	{
		if (visible)
		{
			[video->m_window orderFront:nil];
		}
		else
		{
			[video->m_window orderOut:nil];
		}
	}
}


static bool HasModernFullscreenAPI()
{
	// The following value shoud be equal to NSAppKitVersionNumber10_6
	// and it's hard-coded in order to build on earlier SDKs

	return NSAppKitVersionNumber >= 1038;
}

void CocoaVideo::SetStyleMask(const NSUInteger styleMask)
{
	// Before 10.6 it's impossible to change window's style mask
	// To workaround this new window should be created with required style mask
	// This method should not be called when running on Snow Leopard or newer

	assert(!HasModernFullscreenAPI());

	CocoaWindow* tempWindow = CreateCocoaWindow(styleMask);
	[tempWindow setContentView:[m_window contentView]];

	[m_window close];
	m_window = tempWindow;
}

void CocoaVideo::SetFullscreenMode(const int width, const int height)
{
	NSScreen* screen = [m_window screen];

	const NSRect screenFrame = [screen frame];
	const NSRect displayRect = vid_hidpi
		? [screen convertRectToBacking:screenFrame]
		: screenFrame;

	const float  displayWidth  = displayRect.size.width;
	const float  displayHeight = displayRect.size.height;

	const float pixelScaleFactorX = displayWidth  / static_cast<float>(width );
	const float pixelScaleFactorY = displayHeight / static_cast<float>(height);

	rbOpts.pixelScale = MIN(pixelScaleFactorX, pixelScaleFactorY);

	rbOpts.width  = width  * rbOpts.pixelScale;
	rbOpts.height = height * rbOpts.pixelScale;

	rbOpts.shiftX = (displayWidth  - rbOpts.width ) / 2.0f;
	rbOpts.shiftY = (displayHeight - rbOpts.height) / 2.0f;

	if (!m_fullscreen)
	{
		if (HasModernFullscreenAPI())
		{
			[m_window setLevel:LEVEL_FULLSCREEN];
			[m_window setStyleMask:STYLE_MASK_FULLSCREEN];
		}
		else
		{
			// Old Carbon-based way to make fullscreen window above dock and menu
			// It's supported on 64-bit, but on 10.6 and later the following is preferred:
			// [NSWindow setLevel:NSMainMenuWindowLevel + 1]

			SetSystemUIMode(kUIModeAllHidden, 0);
			SetStyleMask(STYLE_MASK_FULLSCREEN);
		}

		[m_window setHidesOnDeactivate:YES];
	}

	[m_window setFrame:displayRect display:YES];
	[m_window setFrameOrigin:NSMakePoint(0.0f, 0.0f)];
}

void CocoaVideo::SetWindowedMode(const int width, const int height)
{
	rbOpts.pixelScale = 1.0f;

	rbOpts.width  = static_cast<float>(width );
	rbOpts.height = static_cast<float>(height);

	rbOpts.shiftX = 0.0f;
	rbOpts.shiftY = 0.0f;

	const NSSize windowPixelSize = NSMakeSize(width, height);
	const NSSize windowSize = vid_hidpi
		? [[m_window contentView] convertSizeFromBacking:windowPixelSize]
		: windowPixelSize;

	if (m_fullscreen)
	{
		if (HasModernFullscreenAPI())
		{
			[m_window setLevel:LEVEL_WINDOWED];
			[m_window setStyleMask:STYLE_MASK_WINDOWED];
		}
		else
		{
			SetSystemUIMode(kUIModeNormal, 0);
			SetStyleMask(STYLE_MASK_WINDOWED);
		}

		[m_window setHidesOnDeactivate:NO];
	}

	[m_window setContentSize:windowSize];
	[m_window center];

	NSButton* closeButton = [m_window standardWindowButton:NSWindowCloseButton];
	[closeButton setAction:@selector(terminate:)];
	[closeButton setTarget:NSApp];
}

void CocoaVideo::SetMode(const int width, const int height, const bool fullscreen, const bool hiDPI)
{
	if (fullscreen == m_fullscreen
		&& width   == m_width
		&& height  == m_height
		&& hiDPI   == m_hiDPI)
	{
		return;
	}

	if (I_IsHiDPISupported())
	{
		NSOpenGLView* const glView = [m_window contentView];
		[glView setWantsBestResolutionOpenGLSurface:hiDPI];
	}

	if (fullscreen)
	{
		SetFullscreenMode(width, height);
	}
	else
	{
		SetWindowedMode(width, height);
	}

	rbOpts.dirty = true;

	const NSSize viewSize = I_GetContentViewSize(m_window);

	glViewport(0, 0, static_cast<GLsizei>(viewSize.width), static_cast<GLsizei>(viewSize.height));
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	[[NSOpenGLContext currentContext] flushBuffer];

	static NSString* const TITLE_STRING =
	[NSString stringWithFormat:@"%s %s", GAMESIG, GetVersionString()];
	[m_window setTitle:TITLE_STRING];

	if (![m_window isKeyWindow])
	{
		[m_window makeKeyAndOrderFront:nil];
	}

	m_fullscreen = fullscreen;
	m_width      = width;
	m_height     = height;
	m_hiDPI      = hiDPI;
}


CocoaVideo* CocoaVideo::GetInstance()
{
	return static_cast<CocoaVideo*>(Video);
}


CocoaFrameBuffer::CocoaFrameBuffer(int width, int height, bool fullscreen)
: DFrameBuffer(width, height)
, m_needPaletteUpdate(false)
, m_gamma(0.0f)
, m_needGammaUpdate(false)
, m_flashAmount(0)
, m_isUpdatePending(false)
, m_pixelBuffer(new uint8_t[width * height * BYTES_PER_PIXEL])
, m_texture(0)
{
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texture);
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);

	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, height, 0.0, -1.0, 1.0);

	GPfx.SetFormat(32, 0x000000FF, 0x0000FF00, 0x00FF0000);

	for (size_t i = 0; i < 256; ++i)
	{
		m_gammaTable[0][i] = m_gammaTable[1][i] = m_gammaTable[2][i] = i;
	}

	memcpy(m_palette, GPalette.BaseColors, sizeof(PalEntry) * 256);
	UpdateColors();

	SetVSync(vid_vsync);
}


CocoaFrameBuffer::~CocoaFrameBuffer()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &m_texture);

	delete[] m_pixelBuffer;
}

int CocoaFrameBuffer::GetPageCount()
{
	return 1;
}

bool CocoaFrameBuffer::Lock(bool buffered)
{
	return DSimpleCanvas::Lock(buffered);
}

void CocoaFrameBuffer::Unlock()
{
	if (m_isUpdatePending && LockCount == 1)
	{
		Update();
	}
	else if (--LockCount <= 0)
	{
		Buffer = NULL;
		LockCount = 0;
	}
}

void CocoaFrameBuffer::Update()
{
	if (LockCount != 1)
	{
		if (LockCount > 0)
		{
			m_isUpdatePending = true;
			--LockCount;
		}
		return;
	}

	DrawRateStuff();

	Buffer = NULL;
	LockCount = 0;
	m_isUpdatePending = false;

	BlitCycles.Reset();
	FlipCycles.Reset();
	BlitCycles.Clock();

	GPfx.Convert(MemBuffer, Pitch, m_pixelBuffer, Width * BYTES_PER_PIXEL,
		Width, Height, FRACUNIT, FRACUNIT, 0, 0);

	FlipCycles.Clock();
	Flip();
	FlipCycles.Unclock();

	BlitCycles.Unclock();

	if (m_needGammaUpdate)
	{
		CalcGamma(rgamma == 0.0f ? m_gamma : m_gamma * rgamma, m_gammaTable[0]);
		CalcGamma(ggamma == 0.0f ? m_gamma : m_gamma * ggamma, m_gammaTable[1]);
		CalcGamma(bgamma == 0.0f ? m_gamma : m_gamma * bgamma, m_gammaTable[2]);

		m_needGammaUpdate  = false;
		m_needPaletteUpdate = true;
	}

	if (m_needPaletteUpdate)
	{
		m_needPaletteUpdate = false;
		UpdateColors();
	}
}

void CocoaFrameBuffer::UpdateColors()
{
	PalEntry palette[256];

	for (size_t i = 0; i < 256; ++i)
	{
		palette[i].r = m_gammaTable[0][m_palette[i].r];
		palette[i].g = m_gammaTable[1][m_palette[i].g];
		palette[i].b = m_gammaTable[2][m_palette[i].b];
	}

	if (0 != m_flashAmount)
	{
		DoBlending(palette, palette, 256,
			m_gammaTable[0][m_flashColor.r],
			m_gammaTable[1][m_flashColor.g],
			m_gammaTable[2][m_flashColor.b],
			m_flashAmount);
	}

	GPfx.SetPalette(palette);
}

PalEntry* CocoaFrameBuffer::GetPalette()
{
	return m_palette;
}

void CocoaFrameBuffer::UpdatePalette()
{
	m_needPaletteUpdate = true;
}

bool CocoaFrameBuffer::SetGamma(float gamma)
{
	m_gamma           = gamma;
	m_needGammaUpdate = true;

	return true;
}

bool CocoaFrameBuffer::SetFlash(PalEntry rgb, int amount)
{
	m_flashColor        = rgb;
	m_flashAmount       = amount;
	m_needPaletteUpdate = true;

	return true;
}

void CocoaFrameBuffer::GetFlash(PalEntry &rgb, int &amount)
{
	rgb    = m_flashColor;
	amount = m_flashAmount;
}

void CocoaFrameBuffer::GetFlashedPalette(PalEntry pal[256])
{
	memcpy(pal, m_palette, sizeof m_palette);

	if (0 != m_flashAmount)
	{
		DoBlending(pal, pal, 256,
			m_flashColor.r, m_flashColor.g, m_flashColor.b,
			m_flashAmount);
	}
}

bool CocoaFrameBuffer::IsFullscreen()
{
	return CocoaVideo::IsFullscreen();
}

void CocoaFrameBuffer::SetVSync(bool vsync)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED < 1050
	const long value = vsync ? 1 : 0;
#else // 10.5 or newer
	const GLint value = vsync ? 1 : 0;
#endif // prior to 10.5

	[[NSOpenGLContext currentContext] setValues:&value
								   forParameter:NSOpenGLCPSwapInterval];
}

void CocoaFrameBuffer::Flip()
{
	assert(NULL != screen);

	if (rbOpts.dirty)
	{
		glViewport(rbOpts.shiftX, rbOpts.shiftY, rbOpts.width, rbOpts.height);

		// TODO: Figure out why the following glClear() call is needed
		// to avoid drawing of garbage in fullscreen mode when
		// in-game's aspect ratio is different from display one
		glClear(GL_COLOR_BUFFER_BIT);

		rbOpts.dirty = false;
	}

#ifdef __LITTLE_ENDIAN__
	static const GLenum format = GL_RGBA;
#else // __BIG_ENDIAN__
	static const GLenum format = GL_ABGR_EXT;
#endif // __LITTLE_ENDIAN__

	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
		Width, Height, 0, format, GL_UNSIGNED_BYTE, m_pixelBuffer);

	glBegin(GL_QUADS);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glTexCoord2f(Width, 0.0f);
	glVertex2f(Width, 0.0f);
	glTexCoord2f(Width, Height);
	glVertex2f(Width, Height);
	glTexCoord2f(0.0f, Height);
	glVertex2f(0.0f, Height);
	glEnd();

	glFlush();

	[[NSOpenGLContext currentContext] flushBuffer];
}


ADD_STAT(blit)
{
	FString result;
	result.Format("blit=%04.1f ms  flip=%04.1f ms", BlitCycles.TimeMS(), FlipCycles.TimeMS());
	return result;
}


IVideo* Video;


void I_ShutdownGraphics()
{
	if (NULL != screen)
	{
		screen->ObjectFlags |= OF_YesReallyDelete;
		delete screen;
		screen = NULL;
	}

	delete Video;
	Video = NULL;
}

void I_InitGraphics()
{
	UCVarValue val;

	val.Bool = !!Args->CheckParm("-devparm");
	ticker.SetGenericRepDefault(val, CVAR_Bool);

	Video = new CocoaVideo(0);
	atterm(I_ShutdownGraphics);
}


static void I_DeleteRenderer()
{
	delete Renderer;
	Renderer = NULL;
}

void I_CreateRenderer()
{
	if (NULL == Renderer)
	{
		Renderer = new FSoftwareRenderer;
		atterm(I_DeleteRenderer);
	}
}


DFrameBuffer* I_SetMode(int &width, int &height, DFrameBuffer* old)
{
	return Video->CreateFrameBuffer(width, height, fullscreen, old);
}

bool I_CheckResolution(const int width, const int height, const int bits)
{
	int twidth, theight;

	Video->StartModeIterator(bits, fullscreen);

	while (Video->NextMode(&twidth, &theight, NULL))
	{
		if (width == twidth && height == theight)
		{
			return true;
		}
	}

	return false;
}

void I_ClosestResolution(int *width, int *height, int bits)
{
	int twidth, theight;
	int cwidth = 0, cheight = 0;
	int iteration;
	DWORD closest = DWORD(-1);

	for (iteration = 0; iteration < 2; ++iteration)
	{
		Video->StartModeIterator(bits, fullscreen);

		while (Video->NextMode(&twidth, &theight, NULL))
		{
			if (twidth == *width && theight == *height)
			{
				return;
			}

			if (iteration == 0 && (twidth < *width || theight < *height))
			{
				continue;
			}

			const DWORD dist = (twidth - *width) * (twidth - *width)
				+ (theight - *height) * (theight - *height);

			if (dist < closest)
			{
				closest = dist;
				cwidth = twidth;
				cheight = theight;
			}
		}

		if (closest != DWORD(-1))
		{
			*width = cwidth;
			*height = cheight;
			return;
		}
	}
}


EXTERN_CVAR(Int, vid_maxfps);
EXTERN_CVAR(Bool, cl_capfps);

// So Apple doesn't support POSIX timers and I can't find a good substitute short of
// having Objective-C Cocoa events or something like that.
void I_SetFPSLimit(int limit)
{
}

CUSTOM_CVAR(Int, vid_maxfps, 200, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (vid_maxfps < TICRATE && vid_maxfps != 0)
	{
		vid_maxfps = TICRATE;
	}
	else if (vid_maxfps > 1000)
	{
		vid_maxfps = 1000;
	}
	else if (cl_capfps == 0)
	{
		I_SetFPSLimit(vid_maxfps);
	}
}

CUSTOM_CVAR(Bool, vid_hidpi, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (I_IsHiDPISupported())
	{
		CocoaVideo::UseHiDPI(self);
	}
	else if (0 != self)
	{
		self = 0;
	}
}


CCMD(vid_listmodes)
{
	if (Video == NULL)
	{
		return;
	}

	static const char* const ratios[5] = { "", " - 16:9", " - 16:10", " - 17:10", " - 5:4" };
	int width, height;
	bool letterbox;

	Video->StartModeIterator(32, screen->IsFullscreen());

	while (Video->NextMode(&width, &height, &letterbox))
	{
		const bool current = width == DisplayWidth && height == DisplayHeight;
		const int  ratio   = CheckRatio(width, height);

		Printf(current ? PRINT_BOLD : PRINT_HIGH, "%s%4d x%5d x%3d%s%s\n",
			current || !(ratio & 3) ? "" : TEXTCOLOR_GOLD,
			width, height, 32, ratios[ratio],
			current || !letterbox ? "" : TEXTCOLOR_BROWN " LB");
	}
}

CCMD(vid_currentmode)
{
	Printf("%dx%dx%d\n", DisplayWidth, DisplayHeight, DisplayBits);
}


// ---------------------------------------------------------------------------


bool I_SetCursor(FTexture* cursorpic)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSCursor* cursor = nil;

	if (NULL != cursorpic && FTexture::TEX_Null != cursorpic->UseType)
	{
		// Create bitmap image representation

		const NSInteger imageWidth  = cursorpic->GetWidth();
		const NSInteger imageHeight = cursorpic->GetHeight();
		const NSInteger imagePitch  = imageWidth * 4;

		NSBitmapImageRep* bitmapImageRep = [NSBitmapImageRep alloc];
		[bitmapImageRep initWithBitmapDataPlanes:NULL
									  pixelsWide:imageWidth
									  pixelsHigh:imageHeight
								   bitsPerSample:8
								 samplesPerPixel:4
										hasAlpha:YES
										isPlanar:NO
								  colorSpaceName:NSDeviceRGBColorSpace
									 bytesPerRow:imagePitch
									bitsPerPixel:0];

		// Load bitmap data to representation

		BYTE* buffer = [bitmapImageRep bitmapData];
		memset(buffer, 0, imagePitch * imageHeight);

		FBitmap bitmap(buffer, imagePitch, imageWidth, imageHeight);
		cursorpic->CopyTrueColorPixels(&bitmap, 0, 0);

		// Swap red and blue components in each pixel

		for (size_t i = 0; i < size_t(imageWidth * imageHeight); ++i)
		{
			const size_t offset = i * 4;

			const BYTE temp    = buffer[offset    ];
			buffer[offset    ] = buffer[offset + 2];
			buffer[offset + 2] = temp;
		}

		// Create image from representation and set it as cursor

		NSData* imageData = [bitmapImageRep representationUsingType:NSPNGFileType
														 properties:nil];
		NSImage* cursorImage = [[NSImage alloc] initWithData:imageData];

		cursor = [[NSCursor alloc] initWithImage:cursorImage
										 hotSpot:NSMakePoint(0.0f, 0.0f)];
	}
	
	CocoaVideo::SetCursor(cursor);
	
	[pool release];
	
	return true;
}


NSSize I_GetContentViewSize(const NSWindow* const window)
{
	const NSView* const view = [window contentView];
	const NSSize frameSize   = [view frame].size;

	// TODO: figure out why [NSView frame] returns different values in "fullscreen" and in window
	// In "fullscreen" the result is multiplied by [NSScreen backingScaleFactor], but not in window

	return (vid_hidpi && !fullscreen)
		? [view convertSizeToBacking:frameSize]
		: frameSize;
}

void I_SetMainWindowVisible(bool visible)
{
	CocoaVideo::SetWindowVisible(visible);
	I_SetNativeMouse(!visible);
}
