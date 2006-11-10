/* 
 * tkTreeTheme.c --
 *
 *	This module implements platform-specific visual themes.
 *
 * Copyright (c) 2006 Tim Baker
 *
 * RCS: @(#) $Id$
 */

#ifdef WIN32
#define WINVER 0x0501 /* Cygwin */
#endif

#include "tkTreeCtrl.h"
#include "ttk/ttk-extra.h"

/* These must agree with tkTreeColumn.c */
#define COLUMN_STATE_NORMAL 0
#define COLUMN_STATE_ACTIVE 1
#define COLUMN_STATE_PRESSED 2

#ifdef WIN32xxx
#include "tkWinInt.h"

#include <uxtheme.h>
#include <tmschema.h>

#include <basetyps.h> /* Cygwin */
#ifndef TMT_CONTENTMARGINS
#define TMT_CONTENTMARGINS 3602
#endif

typedef HTHEME (STDAPICALLTYPE OpenThemeDataProc)(HWND hwnd,
    LPCWSTR pszClassList);
typedef HRESULT (STDAPICALLTYPE CloseThemeDataProc)(HTHEME hTheme);
typedef HRESULT (STDAPICALLTYPE DrawThemeBackgroundProc)(HTHEME hTheme,
    HDC hdc, int iPartId, int iStateId, const RECT *pRect,
    OPTIONAL const RECT *pClipRect);
typedef HRESULT (STDAPICALLTYPE DrawThemeBackgroundExProc)(HTHEME hTheme,
    HDC hdc, int iPartId, int iStateId, const RECT *pRect,
    DTBGOPTS *PDTBGOPTS);
typedef HRESULT (STDAPICALLTYPE DrawThemeParentBackgroundProc)(HWND hwnd,
    HDC hdc, OPTIONAL const RECT *prc);
typedef HRESULT (STDAPICALLTYPE DrawThemeEdgeProc)(HTHEME hTheme, HDC hdc,
    int iPartId, int iStateId, const RECT *pDestRect,
    UINT uEdge, UINT uFlags, RECT *pContentRect);
typedef HRESULT (STDAPICALLTYPE DrawThemeTextProc)(HTHEME hTheme, HDC hdc,
    int iPartId, int iStateId, LPCWSTR pszText, int iCharCount,
    DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect);
typedef HRESULT (STDAPICALLTYPE GetThemeBackgroundContentRectProc)(
    HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
    const RECT *pBoundingRect, RECT *pContentRect);
typedef HRESULT (STDAPICALLTYPE GetThemeBackgroundExtentProc)(HTHEME hTheme,
    HDC hdc, int iPartId, int iStateId, const RECT *pContentRect,
    RECT *pExtentRect);
typedef HRESULT (STDAPICALLTYPE GetThemeMarginsProc)(HTHEME, HDC,
    int iPartId, int iStateId, int iPropId, OPTIONAL RECT *prc,
    MARGINS *pMargins);
typedef HRESULT (STDAPICALLTYPE GetThemePartSizeProc)(HTHEME, HDC, int iPartId,
    int iStateId, RECT *prc, enum THEMESIZE eSize, SIZE *psz);
typedef HRESULT (STDAPICALLTYPE GetThemeTextExtentProc)(HTHEME hTheme, HDC hdc,
    int iPartId, int iStateId, LPCWSTR pszText, int iCharCount,
    DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pExtentRect);
typedef BOOL (STDAPICALLTYPE IsThemeActiveProc)(VOID);
typedef BOOL (STDAPICALLTYPE IsThemePartDefinedProc)(HTHEME, int, int);
typedef HRESULT (STDAPICALLTYPE IsThemeBackgroundPartiallyTransparentProc)(
    HTHEME, int, int);

typedef struct
{
    OpenThemeDataProc				*OpenThemeData;
    CloseThemeDataProc				*CloseThemeData;
    DrawThemeBackgroundProc			*DrawThemeBackground;
    DrawThemeBackgroundExProc			*DrawThemeBackgroundEx;
    DrawThemeParentBackgroundProc		*DrawThemeParentBackground;
    DrawThemeEdgeProc				*DrawThemeEdge;
    DrawThemeTextProc				*DrawThemeText;
    GetThemeBackgroundContentRectProc		*GetThemeBackgroundContentRect;
    GetThemeBackgroundExtentProc		*GetThemeBackgroundExtent;
    GetThemeMarginsProc				*GetThemeMargins;
    GetThemePartSizeProc			*GetThemePartSize;
    GetThemeTextExtentProc			*GetThemeTextExtent;
    IsThemeActiveProc				*IsThemeActive;
    IsThemePartDefinedProc			*IsThemePartDefined;
    IsThemeBackgroundPartiallyTransparentProc 	*IsThemeBackgroundPartiallyTransparent;
} XPThemeProcs;

typedef struct
{
    HINSTANCE hlibrary;
    XPThemeProcs *procs;
    int registered;
    int themeEnabled;
    SIZE buttonOpen;
    SIZE buttonClosed;
} XPThemeData;

static XPThemeProcs *procs = NULL;
static XPThemeData *themeData = NULL; 
TCL_DECLARE_MUTEX(themeMutex)

/*
 *----------------------------------------------------------------------
 *
 * LoadXPThemeProcs --
 *	Initialize XP theming support.
 *
 *	XP theme support is included in UXTHEME.DLL
 *	We dynamically load this DLL at runtime instead of linking
 *	to it at build-time.
 *
 * Returns:
 *	A pointer to an XPThemeProcs table if successful, NULL otherwise.
 */

static XPThemeProcs *
LoadXPThemeProcs(HINSTANCE *phlib)
{
    OSVERSIONINFO os;

    /*
     * We have to check whether we are running at least on Windows XP.
     * In order to determine this we call GetVersionEx directly, although
     * it would be a good idea to wrap it inside a function similar to
     * TkWinGetPlatformId...
     */
    ZeroMemory(&os, sizeof(os));
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);
    if (os.dwMajorVersion >= 5 && os.dwMinorVersion >= 1) {
	/*
	 * We are running under Windows XP or a newer version.
	 * Load the library "uxtheme.dll", where the native widget
	 * drawing routines are implemented.
	 */
	HINSTANCE handle;
	*phlib = handle = LoadLibrary("uxtheme.dll");
	if (handle != 0)
	{
	    /*
	     * We have successfully loaded the library. Proceed in storing the
	     * addresses of the functions we want to use.
	     */
	    XPThemeProcs *procs = (XPThemeProcs*)ckalloc(sizeof(XPThemeProcs));
#define LOADPROC(name) \
	(0 != (procs->name = (name ## Proc *)GetProcAddress(handle, #name) ))

	    if (   LOADPROC(OpenThemeData)
		&& LOADPROC(CloseThemeData)
		&& LOADPROC(DrawThemeBackground)
		&& LOADPROC(DrawThemeBackgroundEx)
		&& LOADPROC(DrawThemeParentBackground)
		&& LOADPROC(DrawThemeEdge)
		&& LOADPROC(DrawThemeText)
		&& LOADPROC(GetThemeBackgroundContentRect)
		&& LOADPROC(GetThemeBackgroundExtent)
		&& LOADPROC(GetThemeMargins)
		&& LOADPROC(GetThemePartSize)
		&& LOADPROC(GetThemeTextExtent)
		&& LOADPROC(IsThemeActive)
		&& LOADPROC(IsThemePartDefined)
		&& LOADPROC(IsThemeBackgroundPartiallyTransparent)
	    )
	    {
		return procs;
	    }
#undef LOADPROC
	    ckfree((char*)procs);
	}
    }
    return 0;
}

int TreeTheme_DrawHeaderItem(TreeCtrl *tree, Drawable drawable, int state,
    int arrow, int x, int y, int width, int height)
{
    Window win = Tk_WindowId(tree->tkwin);
    HWND hwnd = Tk_GetHWND(win);
    HTHEME hTheme;
    HDC hDC;
    TkWinDCState dcState;
    RECT rc;
    HRESULT hr;

    int iPartId = HP_HEADERITEM;
    int iStateId = HIS_NORMAL;

    switch (state)
    {
	case COLUMN_STATE_ACTIVE:  iStateId = HIS_HOT; break;
	case COLUMN_STATE_PRESSED: iStateId = HIS_PRESSED; break;
    }

    if (!themeData->themeEnabled || !procs)
	return TCL_ERROR;

    hTheme = procs->OpenThemeData(hwnd, L"HEADER");
    if (!hTheme)
	return TCL_ERROR;

#if 0 /* Always returns FALSE */
    if (!procs->IsThemePartDefined(
	hTheme,
	iPartId,
	iStateId))
    {
	procs->CloseThemeData(hTheme);
	return TCL_ERROR;
    }
#endif

    rc.left = x;
    rc.top = y;
    rc.right = x + width;
    rc.bottom = y + height;

    /* Is transparent for the default XP style. */
    if (procs->IsThemeBackgroundPartiallyTransparent(
	hTheme,
	iPartId,
	iStateId))
    {
#if 1
	/* What color should I use? */
	Tk_Fill3DRectangle(tree->tkwin, drawable, tree->border, x, y, width, height, 0, TK_RELIEF_FLAT);
#else
	/* This draws nothing, maybe because the parent window is not
	 * themed */
	procs->DrawThemeParentBackground(
	    hwnd,
	    hDC,
	    &rc);
#endif
    }

    hDC = TkWinGetDrawableDC(tree->display, drawable, &dcState);

#if 0
    {
	/* Default XP theme gives rect 3 pixels narrower than rc */
	RECT contentRect, extentRect;
	hr = procs->GetThemeBackgroundContentRect(
	    hTheme,
	    hDC,
	    iPartId,
	    iStateId,
	    &rc,
	    &contentRect
	);
	dbwin("GetThemeBackgroundContentRect width=%d height=%d\n",
	    contentRect.right - contentRect.left,
	    contentRect.bottom - contentRect.top);

	/* Gives rc */
	hr = procs->GetThemeBackgroundExtent(
	    hTheme,
	    hDC,
	    iPartId,
	    iStateId,
	    &contentRect,
	    &extentRect
	);
	dbwin("GetThemeBackgroundExtent width=%d height=%d\n",
	    extentRect.right - extentRect.left,
	    extentRect.bottom - extentRect.top);
    }
#endif

    hr = procs->DrawThemeBackground(
	hTheme,
	hDC,
	iPartId,
	iStateId,
	&rc,
	NULL);

    procs->CloseThemeData(hTheme);
    TkWinReleaseDrawableDC(drawable, hDC, &dcState);

    if (hr != S_OK)
	return TCL_ERROR;

    return TCL_OK;
}

int TreeTheme_GetHeaderContentMargins(TreeCtrl *tree, int state, int arrow, int bounds[4])
{
    Window win = Tk_WindowId(tree->tkwin);
    HWND hwnd = Tk_GetHWND(win);
    HTHEME hTheme;
    HDC hDC;
    TkWinDCState dcState;
    HRESULT hr;
    MARGINS margins;

    int iPartId = HP_HEADERITEM;
    int iStateId = HIS_NORMAL;

    switch (state)
    {
	case COLUMN_STATE_ACTIVE:  iStateId = HIS_HOT; break;
	case COLUMN_STATE_PRESSED: iStateId = HIS_PRESSED; break;
    }

    if (!themeData->themeEnabled || !procs)
	return TCL_ERROR;

    hTheme = procs->OpenThemeData(hwnd, L"HEADER");
    if (!hTheme)
	return TCL_ERROR;

    hDC = TkWinGetDrawableDC(tree->display, win, &dcState);

    /* The default XP themes give 3,0,0,0 which makes little sense since
     * it is the *right* side that should not be drawn over by text; the
     * 2-pixel wide header divider is on the right */
    hr = procs->GetThemeMargins(
	hTheme,
	hDC,
	iPartId,
	iStateId,
	TMT_CONTENTMARGINS,
	NULL,
	&margins);

    procs->CloseThemeData(hTheme);
    TkWinReleaseDrawableDC(win, hDC, &dcState);

    if (hr != S_OK)
	return TCL_ERROR;

    bounds[0] = margins.cxLeftWidth;
    bounds[1] = margins.cyTopHeight;
    bounds[2] = margins.cxRightWidth;
    bounds[3] = margins.cyBottomHeight;
/*
dbwin("margins %d %d %d %d\n", bounds[0], bounds[1], bounds[2], bounds[3]);
*/
    return TCL_OK;
}

int TreeTheme_DrawHeaderArrow(TreeCtrl *tree, Drawable drawable, int up,
    int x, int y, int width, int height)
{
#if 1
    XColor *color;
    GC gc;
    int i;

    if (!themeData->themeEnabled || !procs)
	return TCL_ERROR;

    color = Tk_GetColor(tree->interp, tree->tkwin, "#ACA899");
    gc = Tk_GCForColor(color, drawable);

    if (up) {
	for (i = 0; i < height; i++) {
	    XDrawLine(tree->display, drawable, gc,
		x + width / 2 - i, y + i,
		x + width / 2 + i + 1, y + i);
	}
    } else {
	for (i = 0; i < height; i++) {
	    XDrawLine(tree->display, drawable, gc,
		x + width / 2 - i, y + (height - 1) - i,
		x + width / 2 + i + 1, y + (height - 1) - i);
	}
    }

    Tk_FreeColor(color);
    return TCL_OK;
#else
    /* Doesn't seem that Microsoft actually implemented this */
    Window win = Tk_WindowId(tree->tkwin);
    HWND hwnd = Tk_GetHWND(win);
    HTHEME hTheme;
    HDC hDC;
    TkWinDCState dcState;
    RECT rc;
    HRESULT hr;

    int iPartId = HP_HEADERSORTARROW;
    int iStateId = up ? HSAS_SORTEDUP : HSAS_SORTEDDOWN;

    if (!themeData->themeEnabled || !procs)
	return TCL_ERROR;

    hTheme = procs->OpenThemeData(hwnd, L"HEADER");
    if (!hTheme)
	return TCL_ERROR;

    if (!procs->IsThemePartDefined(
	hTheme,
	iPartId,
	iStateId))
    {
	procs->CloseThemeData(hTheme);
	return TCL_ERROR;
    }

    hDC = TkWinGetDrawableDC(tree->display, drawable, &dcState);

    rc.left = x;
    rc.top = y;
    rc.right = x + width;
    rc.bottom = y + height;

    hr = procs->DrawThemeBackground(
	hTheme,
	hDC,
	iPartId,
	iStateId,
	&rc,
	NULL);

    procs->CloseThemeData(hTheme);
    TkWinReleaseDrawableDC(drawable, hDC, &dcState);
    return TCL_OK;
#endif /* 0 */
}

int TreeTheme_DrawButton(TreeCtrl *tree, Drawable drawable, int open,
    int x, int y, int width, int height)
{
    Window win = Tk_WindowId(tree->tkwin);
    HWND hwnd = Tk_GetHWND(win);
    HTHEME hTheme;
    HDC hDC;
    TkWinDCState dcState;
    RECT rc;
    HRESULT hr;
    int iPartId, iStateId;

    if (!themeData->themeEnabled || !procs)
	return TCL_ERROR;

    iPartId  = TVP_GLYPH;
    iStateId = open ? GLPS_OPENED : GLPS_CLOSED;

    hTheme = procs->OpenThemeData(hwnd, L"TREEVIEW");
    if (!hTheme)
	return TCL_ERROR;

#if 0 /* Always returns FALSE */
    if (!procs->IsThemePartDefined(
	hTheme,
	iPartId,
	iStateId))
    {
	procs->CloseThemeData(hTheme);
	return TCL_ERROR;
    }
#endif

    hDC = TkWinGetDrawableDC(tree->display, drawable, &dcState);

    rc.left = x;
    rc.top = y;
    rc.right = x + width;
    rc.bottom = y + height;
    hr = procs->DrawThemeBackground(
	hTheme,
	hDC,
	iPartId,
	iStateId,
	&rc,
	NULL);

    procs->CloseThemeData(hTheme);
    TkWinReleaseDrawableDC(drawable, hDC, &dcState);

    if (hr != S_OK)
	return TCL_ERROR;

    return TCL_OK;
}

int TreeTheme_GetButtonSize(TreeCtrl *tree, Drawable drawable, int open,
    int *widthPtr, int *heightPtr)
{
    Window win = Tk_WindowId(tree->tkwin);
    HWND hwnd = Tk_GetHWND(win);
    HTHEME hTheme;
    HDC hDC;
    TkWinDCState dcState;
    HRESULT hr;
    SIZE size;
    int iPartId, iStateId;

    if (!themeData->themeEnabled || !procs)
	return TCL_ERROR;

    /* Use cached values */
    size = open ? themeData->buttonOpen : themeData->buttonClosed;
    if (size.cx > 1) {
	*widthPtr = size.cx;
	*heightPtr = size.cy;
	return TCL_OK;
    }

    iPartId  = TVP_GLYPH;
    iStateId = open ? GLPS_OPENED : GLPS_CLOSED;

    hTheme = procs->OpenThemeData(hwnd, L"TREEVIEW");
    if (!hTheme)
	return TCL_ERROR;

#if 0 /* Always returns FALSE */
    if (!procs->IsThemePartDefined(
	hTheme,
	iPartId,
	iStateId))
    {
	procs->CloseThemeData(hTheme);
	return TCL_ERROR;
    }
#endif

    hDC = TkWinGetDrawableDC(tree->display, drawable, &dcState);

    /* Returns 9x9 for default XP style */
    hr = procs->GetThemePartSize(
	hTheme,
	hDC,
	iPartId,
	iStateId,
	NULL,
	TS_DRAW,
	&size
    );

    procs->CloseThemeData(hTheme);
    TkWinReleaseDrawableDC(drawable, hDC, &dcState);

    /* With RandomN of 10000, I eventually get hr=E_HANDLE, invalid handle */
    if (hr != S_OK)
	return TCL_ERROR;

    /* Gave me 0,0 for a non-default theme, even though glyph existed */
    if ((size.cx <= 1) && (size.cy <= 1))
	return TCL_ERROR;

    /* Cache the values */
    if (open)
	themeData->buttonOpen = size;
    else
	themeData->buttonClosed = size;

    *widthPtr = size.cx;
    *heightPtr = size.cy;
    return TCL_OK;
}

int TreeTheme_GetArrowSize(TreeCtrl *tree, Drawable drawable, int up, int *widthPtr, int *heightPtr)
{
    if (!themeData->themeEnabled || !procs)
	return TCL_ERROR;

    *widthPtr = 9;
    *heightPtr = 5;
    return TCL_OK;
}

#if !defined(WM_THEMECHANGED)
#define WM_THEMECHANGED 0x031A
#endif

static LRESULT WINAPI
WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Tcl_Interp *interp = (Tcl_Interp *)GetWindowLong(hwnd, GWL_USERDATA);

    switch (msg) {
	case WM_THEMECHANGED:
	    Tcl_MutexLock(&themeMutex);
	    themeData->themeEnabled = procs->IsThemeActive();
	    themeData->buttonClosed.cx = themeData->buttonOpen.cx = -1;
	    Tcl_MutexUnlock(&themeMutex);
	    Tree_TheWorldHasChanged(interp);
	    break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

static CHAR windowClassName[32] = "TreeCtrlMonitorClass";

static BOOL
RegisterThemeMonitorWindowClass(HINSTANCE hinst)
{
    WNDCLASSEX wc;
    
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hinst;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName  = windowClassName;
    wc.lpszClassName = windowClassName;
    
    return RegisterClassEx(&wc);
}

static HWND
CreateThemeMonitorWindow(HINSTANCE hinst, Tcl_Interp *interp)
{
    CHAR title[32] = "TreeCtrlMonitorWindow";
    HWND hwnd;

    hwnd = CreateWindow(windowClassName, title, WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	NULL, NULL, hinst, NULL);
    if (!hwnd)
	return NULL;

    SetWindowLong(hwnd, GWL_USERDATA, (LONG)interp);
    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);

    return hwnd;
}

typedef struct PerInterpData PerInterpData;
struct PerInterpData
{
    HWND hwnd;
};

static void FreeAssocData(ClientData clientData, Tcl_Interp *interp)
{
    PerInterpData *data = (PerInterpData *) clientData;

    DestroyWindow(data->hwnd);
    ckfree((char *) data);
}

int TreeTheme_Init(Tcl_Interp *interp)
{
    HWND hwnd;
    PerInterpData *data;

    Tcl_MutexLock(&themeMutex);

    /* This is done once per-application */
    if (themeData == NULL)
    {
	themeData = (XPThemeData *) ckalloc(sizeof(XPThemeData));
	themeData->procs = LoadXPThemeProcs(&themeData->hlibrary);
	themeData->registered = FALSE;
	themeData->themeEnabled = FALSE;
	themeData->buttonClosed.cx = themeData->buttonOpen.cx = -1;

	procs = themeData->procs;

	if (themeData->procs) {
	    /* Check this again if WM_THEMECHANGED arrives */
	    themeData->themeEnabled = procs->IsThemeActive();

	    themeData->registered =
		RegisterThemeMonitorWindowClass(Tk_GetHINSTANCE());
	}
    }

    Tcl_MutexUnlock(&themeMutex);

    if (!procs || !themeData->registered)
	return TCL_ERROR;

    /* Per-interp */
    hwnd = CreateThemeMonitorWindow(Tk_GetHINSTANCE(), interp);
    if (!hwnd)
	return TCL_ERROR;

    data = (PerInterpData *) ckalloc(sizeof(PerInterpData));
    data->hwnd = hwnd;
    Tcl_SetAssocData(interp, "TreeCtrlTheme", FreeAssocData, (ClientData) data);

    return TCL_OK;
}

#elif defined(MAC_OSX_TK)

#include <Carbon/Carbon.h>
#include "tkMacOSXInt.h"

static RgnHandle oldClip = NULL, boundsRgn = NULL;

int TreeTheme_DrawHeaderItem(TreeCtrl *tree, Drawable drawable, int state,
    int arrow, int x, int y, int width, int height)
{
    MacDrawable *macWin = (MacDrawable *) drawable;
    Rect bounds;
    ThemeButtonDrawInfo info;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;

    bounds.left = macWin->xOff + x;
    bounds.top = macWin->yOff + y;
    bounds.right = bounds.left + width;
    bounds.bottom = bounds.top + height;

    switch (state) {
	case COLUMN_STATE_ACTIVE:  info.state = kThemeStateActive /* kThemeStateRollover */; break;
	case COLUMN_STATE_PRESSED: info.state = kThemeStatePressed; break;
	default:		   info.state = kThemeStateActive; break;
    }
    /* Background window */
    if (!tree->isActive)
	info.state = kThemeStateInactive;
    info.value = (arrow != 0) ? kThemeButtonOn : kThemeButtonOff;
    info.adornment = (arrow == 1) ? kThemeAdornmentHeaderButtonSortUp : kThemeAdornmentNone;

    destPort = TkMacOSXGetDrawablePort(drawable);
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, 0);
    TkMacOSXSetUpClippingRgn(drawable);

    /* Save the old clipping region because we are going to modify it. */
    if (oldClip == NULL)
	oldClip = NewRgn();
    GetClip(oldClip);

    /* Create a clipping region as big as the header. */
    if (boundsRgn == NULL)
	boundsRgn = NewRgn();
    RectRgn(boundsRgn, &bounds);

    /* Set the clipping region to the intersection of the two regions. */
    SectRgn(oldClip, boundsRgn, boundsRgn);
    SetClip(boundsRgn);

    /* Draw the left edge outside of the clipping region. */
    bounds.left -= 1;

    (void) DrawThemeButton(&bounds, kThemeListHeaderButton, &info,
	NULL,	/*prevInfo*/
	NULL,	/*eraseProc*/
	NULL,	/*labelProc*/
	NULL);	/*userData*/

    SetClip(oldClip);
    SetGWorld(saveWorld,saveDevice);

    return TCL_OK;
}

/* List headers are a fixed height on Aqua */
int TreeTheme_GetHeaderFixedHeight(TreeCtrl *tree, int *heightPtr)
{
    SInt32 metric;

    GetThemeMetric(kThemeMetricListHeaderHeight, &metric);
    *heightPtr = metric;
    return TCL_OK;
}

int TreeTheme_GetHeaderContentMargins(TreeCtrl *tree, int state, int arrow, int bounds[4])
{
    Rect inBounds, outBounds;
    ThemeButtonDrawInfo info;
    SInt32 metric;

    inBounds.left = 0;
    inBounds.top = 0;
    inBounds.right = 100;
    GetThemeMetric(kThemeMetricListHeaderHeight, &metric);
    inBounds.bottom = metric;

    switch (state) {
	case COLUMN_STATE_ACTIVE:  info.state = kThemeStateActive /* kThemeStateRollover */; break;
	case COLUMN_STATE_PRESSED: info.state = kThemeStatePressed; break;
	default:		   info.state = kThemeStateActive; break;
    }
    /* Background window */
    if (!tree->isActive)
	info.state = kThemeStateInactive;
    info.value = (arrow != 0) ? kThemeButtonOn : kThemeButtonOff;
    info.adornment = (arrow == 1) ? kThemeAdornmentHeaderButtonSortUp : kThemeAdornmentNone;

    (void) GetThemeButtonContentBounds(
	&inBounds,
	kThemeListHeaderButton,
	&info,
	&outBounds);

    bounds[0] = outBounds.left - inBounds.left;
    bounds[1] = outBounds.top - inBounds.top;
    bounds[2] = inBounds.right - outBounds.right;
    bounds[3] = inBounds.bottom - outBounds.bottom;

    return TCL_OK;
}

int TreeTheme_DrawHeaderArrow(TreeCtrl *tree, Drawable drawable, int up, int x, int y, int width, int height)
{
    return TCL_ERROR;
}

int TreeTheme_DrawButton(TreeCtrl *tree, Drawable drawable, int open, int x, int y, int width, int height)
{
    MacDrawable *macWin = (MacDrawable *) drawable;
    Rect bounds;
    ThemeButtonDrawInfo info;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;

    bounds.left = macWin->xOff + x;
    bounds.top = macWin->yOff + y;
    bounds.right = bounds.left + width;
    bounds.bottom = bounds.top + height;

    info.state = kThemeStateActive;
    info.value = open ? kThemeDisclosureDown : kThemeDisclosureRight;
    info.adornment = kThemeAdornmentNone;

    destPort = TkMacOSXGetDrawablePort(drawable);
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, 0);
    TkMacOSXSetUpClippingRgn(drawable);

    /* Drawing the disclosure triangles produces a white background.
     * To avoid this, set the clipping region to the exact area where
     * pixels are drawn. */

    /* Save the old clipping region because we are going to modify it. */
    if (oldClip == NULL)
	oldClip = NewRgn();
    GetClip(oldClip);

    /* Create a clipping region containing the pixels of the button. */
    if (boundsRgn == NULL)
	boundsRgn = NewRgn();
    (void) GetThemeButtonRegion(&bounds, kThemeDisclosureButton, &info,
	boundsRgn);

    /* Set the clipping region to the intersection of the two regions. */
    SectRgn(oldClip, boundsRgn, boundsRgn);
    SetClip(boundsRgn);

    (void) DrawThemeButton(&bounds, kThemeDisclosureButton, &info,
	NULL,	/*prevInfo*/
	NULL,	/*eraseProc*/
	NULL,	/*labelProc*/
	NULL);	/*userData*/

    /* Restore the original clipping region. */
    SetClip(oldClip);

    SetGWorld(saveWorld,saveDevice);

    return TCL_OK;
}

int TreeTheme_GetButtonSize(TreeCtrl *tree, Drawable drawable, int open, int *widthPtr, int *heightPtr)
{
    SInt32 metric;

    (void) GetThemeMetric(
	kThemeMetricDisclosureTriangleWidth,
	&metric);
    *widthPtr = metric;

    (void) GetThemeMetric(
	kThemeMetricDisclosureTriangleHeight,
	&metric);
    *heightPtr = metric;

    return TCL_OK;
}

int TreeTheme_GetArrowSize(TreeCtrl *tree, Drawable drawable, int up, int *widthPtr, int *heightPtr)
{
    return TCL_ERROR;
}

int TreeTheme_Init(Tcl_Interp *interp)
{
    return TCL_OK;
}

#else /* MAC_OSX_TK */

int TreeTheme_DrawHeaderItem(TreeCtrl *tree, Drawable drawable, int state, int arrow, int x, int y, int width, int height)
{
    Ttk_Layout layout = tree->headingLayout;
    Ttk_State ttk_state = 0;
    Ttk_Box box;

    if (layout == NULL)
	return TCL_ERROR;

    box = Ttk_MakeBox(x, y, width, height);

    switch (state)
    {
	case COLUMN_STATE_ACTIVE:  ttk_state = TTK_STATE_ACTIVE; break;
	case COLUMN_STATE_PRESSED: ttk_state = TTK_STATE_PRESSED; break;
    }

    eTtk_RebindSublayout(layout, NULL); /* !!! rebind to column */
    eTtk_PlaceLayout(layout, ttk_state, box);
    eTtk_DrawLayout(layout, ttk_state, drawable);

    return TCL_OK;
}

int TreeTheme_GetHeaderContentMargins(TreeCtrl *tree, int state, int arrow, int bounds[4])
{
    return TCL_ERROR;
}

int TreeTheme_DrawHeaderArrow(TreeCtrl *tree, Drawable drawable, int up, int x, int y, int width, int height)
{
    return TCL_ERROR;
}

int TreeTheme_DrawButton(TreeCtrl *tree, Drawable drawable, int open, int x, int y, int width, int height)
{
    return TCL_ERROR;
}

int TreeTheme_GetButtonSize(TreeCtrl *tree, Drawable drawable, int open, int *widthPtr, int *heightPtr)
{
    return TCL_ERROR;
}

int TreeTheme_GetArrowSize(TreeCtrl *tree, Drawable drawable, int up, int *widthPtr, int *heightPtr)
{
    return TCL_ERROR;
}

static Tk_OptionSpec HeadingOptionSpecs[] =
{
    {TK_OPTION_END, 0,0,0, NULL, -1,-1, 0,0,0}
};

Ttk_Layout
TreeCtrlGetLayout(
    Tcl_Interp *interp,
    Ttk_Theme themePtr,
    void *recordPtr
    )
{
    TreeCtrl *tree = recordPtr;
    Ttk_Layout treeLayout, newLayout;

    /* This does not get called when resizing/redisplaying the widget. */
    /* This does get called when the theme changes. */
    dbwin("TreeCtrlGetLayout %s\n", Tk_PathName(tree->tkwin));

    if (tree->headingOptionTable == NULL)
	tree->headingOptionTable = Tk_CreateOptionTable(interp, HeadingOptionSpecs);

    /* Create a new layout record based on widget -style or class */
    treeLayout = Ttk_WidgetGetLayout(interp, themePtr, recordPtr);

    /* Create a sublayout for drawing the column headers. The sublayout is
     * called "TreeCtrl.Heading" by default. The actual layout specification
     * was defined by Ttk_RegisterLayout("Heading") below. */
    newLayout = eTtk_CreateSublayout(interp, themePtr, treeLayout,
	    ".Heading", tree->headingOptionTable);
    if (newLayout) {
	if (tree->headingLayout != NULL)
	    eTtk_FreeLayout(tree->headingLayout);
	tree->headingLayout = newLayout;
    }

    Tree_RelayoutWindow(tree);

    return newLayout ? treeLayout : NULL;
}

void
TreeCtrlDoLayout(
    void *recordPtr
    )
{
    TreeCtrl *tree = recordPtr;
    Ttk_LayoutNode *clientNode = eTtk_LayoutFindNode(tree->core.layout, "client");
    Ttk_Box winBox = Ttk_WinBox(tree->tkwin);

    /* This seems to get called for every draw, even when the layout
     * has not changed. */
    dbwin("TreeCtrlDoLayout %s\n", Tk_PathName(tree->tkwin));

    eTtk_PlaceLayout(tree->core.layout, tree->core.state, winBox);

    if (clientNode != NULL)
	tree->clientBox = eTtk_LayoutNodeInternalParcel(tree->core.layout,
		clientNode);
    else
	tree->clientBox = winBox;
}

/* HeaderElement is used for Treeheading.cell. The platform-specific code
 * will draw the native heading. */
typedef struct
{
    Tcl_Obj *backgroundObj;
} HeaderElement;

static Ttk_ElementOptionSpec HeaderElementOptions[] =
{
    { "-background", TK_OPTION_COLOR,
	Tk_Offset(HeaderElement, backgroundObj), DEFAULT_BACKGROUND },
    {NULL}
};

static void HeaderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    HeaderElement *e = elementRecord;
    XColor *color = Tk_GetColorFromObj(tkwin, e->backgroundObj);
    GC gc = Tk_GCForColor(color, d);
    XFillRectangle(Tk_Display(tkwin), d, gc,
	    b.x, b.y, b.width, b.height);
}

static Ttk_ElementSpec HeaderElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(HeaderElement),
    HeaderElementOptions,
    Ttk_NullElementGeometry,
    HeaderElementDraw
};

TTK_BEGIN_LAYOUT(HeadingLayout)
    TTK_NODE("Treeheading.cell", TTK_FILL_BOTH)
    TTK_NODE("Treeheading.border", TTK_FILL_BOTH)
TTK_END_LAYOUT

TTK_BEGIN_LAYOUT(TreeCtrlLayout)
    TTK_GROUP("TreeCtrl.field", TTK_FILL_BOTH|TTK_BORDER,
	TTK_GROUP("TreeCtrl.padding", TTK_FILL_BOTH,
	    TTK_NODE("TreeCtrl.client", TTK_FILL_BOTH)))
TTK_END_LAYOUT

int TreeTheme_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme = Ttk_GetDefaultTheme(interp);

    Ttk_RegisterLayout(theme, "TreeCtrl", TreeCtrlLayout);

    /* Uses the Ttk name */
    Ttk_RegisterElement(interp, theme, "Treeheading.cell", &HeaderElementSpec, 0);
    Ttk_RegisterLayout(theme, "Heading", HeadingLayout);

    return TCL_OK;
}

#endif /* !WIN32 && !MAC_OSX_TK */

