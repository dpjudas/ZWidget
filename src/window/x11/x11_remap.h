#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>

#undef None
#undef Success
#undef Always
#undef GrayScale

#include "x11_dynamic.h"

#undef XDefaultDepth
#undef XDefaultVisual
#undef XDefaultColormap
#undef XGetDefault
#undef XFreePixmap
#undef XCreateBitmapFromData
#undef XCreatePixmapCursor
#undef XCreateImage
#undef XCreatePixmap
#undef XDestroyImage
#undef XDefaultGC
#undef XPutImage
#undef XCopyArea
#undef XBlackPixel
#undef XWhitePixel
#undef XCheckTypedWindowEvent
#undef XOpenDisplay
#undef XCloseDisplay
#undef XInitThreads
#undef XkbSetDetectableAutoRepeat
#undef XSetLocaleModifiers
#undef XOpenIM
#undef XCloseIM
#undef XQueryExtension
#undef XIQueryVersion
#undef XIQueryDevice
#undef XIFreeDeviceInfo
#undef XRootWindow
#undef XDefaultScreen
#undef XISelectEvents
#undef XFreeCursor
#undef XInternAtom
#undef XPending
#undef XNextEvent
#undef XConnectionNumber
#undef XGetEventData
#undef XFreeEventData
#undef XDisplayWidth
#undef XDisplayHeight
#undef XDisplayWidthMM
#undef XCreateFontCursor
#undef XCreateWindow
#undef XDestroyWindow
#undef XMapRaised
#undef XUnmapWindow
#undef XSelectInput
#undef XSendEvent
#undef XSetWMProtocols
#undef XSetWMHints
#undef XSetStandardProperties
#undef XSetTransientForHint
#undef XDefineCursor
#undef XGetWindowAttributes
#undef XSetInputFocus
#undef XCreateIC
#undef XSetICFocus
#undef XUnsetICFocus
#undef XDestroyIC
#undef Xutf8LookupString
#undef XLookupString
#undef XkbKeycodeToKeysym
#undef XFree
#undef XFlush
#undef XSync
#undef XTranslateCoordinates
#undef XGetGeometry
#undef XConfigureWindow
#undef XRaiseWindow
#undef XIconifyWindow
#undef XChangeProperty
#undef XSetSelectionOwner
#undef XConvertSelection
#undef XGetWindowProperty
#undef XGrabPointer
#undef XUngrabPointer
#undef XWarpPointer

#define XDefaultDepth X11Dynamic::Get()->p_DefaultDepth
#define XDefaultVisual X11Dynamic::Get()->p_DefaultVisual
#define XDefaultColormap X11Dynamic::Get()->p_DefaultColormap
#define XGetDefault X11Dynamic::Get()->p_GetDefault
#define XFreePixmap X11Dynamic::Get()->p_FreePixmap
#define XCreateBitmapFromData X11Dynamic::Get()->p_CreateBitmapFromData
#define XCreatePixmapCursor X11Dynamic::Get()->p_CreatePixmapCursor
#define XCreateImage X11Dynamic::Get()->p_CreateImage
#define XCreatePixmap X11Dynamic::Get()->p_CreatePixmap
#define XDestroyImage X11Dynamic::Get()->p_DestroyImage
#define XDefaultGC X11Dynamic::Get()->p_DefaultGC
#define XPutImage X11Dynamic::Get()->p_PutImage
#define XCopyArea X11Dynamic::Get()->p_CopyArea
#define XBlackPixel X11Dynamic::Get()->p_BlackPixel
#define XWhitePixel X11Dynamic::Get()->p_WhitePixel
#define XCheckTypedWindowEvent X11Dynamic::Get()->p_CheckTypedWindowEvent
#define XOpenDisplay X11Dynamic::Get()->p_OpenDisplay
#define XCloseDisplay X11Dynamic::Get()->p_CloseDisplay
#define XInitThreads X11Dynamic::Get()->p_InitThreads
#define XkbSetDetectableAutoRepeat X11Dynamic::Get()->p_SetDetectableAutoRepeat
#define XSetLocaleModifiers X11Dynamic::Get()->p_SetLocaleModifiers
#define XOpenIM X11Dynamic::Get()->p_OpenIM
#define XCloseIM X11Dynamic::Get()->p_CloseIM
#define XQueryExtension X11Dynamic::Get()->p_QueryExtension
#define XIQueryVersion X11Dynamic::Get()->p_IQueryVersion
#define XIQueryDevice X11Dynamic::Get()->p_IQueryDevice
#define XIFreeDeviceInfo X11Dynamic::Get()->p_IFreeDeviceInfo
#define XRootWindow X11Dynamic::Get()->p_RootWindow
#define XDefaultScreen X11Dynamic::Get()->p_DefaultScreen
#define XISelectEvents X11Dynamic::Get()->p_ISelectEvents
#define XFreeCursor X11Dynamic::Get()->p_FreeCursor
#define XInternAtom X11Dynamic::Get()->p_InternAtom
#define XPending X11Dynamic::Get()->p_Pending
#define XNextEvent X11Dynamic::Get()->p_NextEvent
#define XConnectionNumber X11Dynamic::Get()->p_ConnectionNumber
#define XGetEventData X11Dynamic::Get()->p_GetEventData
#define XFreeEventData X11Dynamic::Get()->p_FreeEventData
#define XDisplayWidth X11Dynamic::Get()->p_DisplayWidth
#define XDisplayHeight X11Dynamic::Get()->p_DisplayHeight
#define XDisplayWidthMM X11Dynamic::Get()->p_DisplayWidthMM
#define XCreateFontCursor X11Dynamic::Get()->p_CreateFontCursor
#define XCreateWindow X11Dynamic::Get()->p_CreateWindow
#define XDestroyWindow X11Dynamic::Get()->p_DestroyWindow
#define XMapRaised X11Dynamic::Get()->p_MapRaised
#define XUnmapWindow X11Dynamic::Get()->p_UnmapWindow
#define XSelectInput X11Dynamic::Get()->p_SelectInput
#define XSendEvent X11Dynamic::Get()->p_SendEvent
#define XSetWMProtocols X11Dynamic::Get()->p_SetWMProtocols
#define XSetWMHints X11Dynamic::Get()->p_SetWMHints
#define XSetStandardProperties X11Dynamic::Get()->p_SetStandardProperties
#define XSetTransientForHint X11Dynamic::Get()->p_SetTransientForHint
#define XDefineCursor X11Dynamic::Get()->p_DefineCursor
#define XGetWindowAttributes X11Dynamic::Get()->p_GetWindowAttributes
#define XSetInputFocus X11Dynamic::Get()->p_SetInputFocus
#define XCreateIC X11Dynamic::Get()->p_CreateIC
#define XSetICFocus X11Dynamic::Get()->p_SetICFocus
#define XUnsetICFocus X11Dynamic::Get()->p_UnsetICFocus
#define XDestroyIC X11Dynamic::Get()->p_DestroyIC
#define Xutf8LookupString X11Dynamic::Get()->p_utf8LookupString
#define XLookupString X11Dynamic::Get()->p_LookupString
#define XkbKeycodeToKeysym X11Dynamic::Get()->p_kbKeycodeToKeysym
#define XFree X11Dynamic::Get()->p_Free
#define XFlush X11Dynamic::Get()->p_Flush
#define XSync X11Dynamic::Get()->p_Sync
#define XTranslateCoordinates X11Dynamic::Get()->p_TranslateCoordinates
#define XGetGeometry X11Dynamic::Get()->p_GetGeometry
#define XConfigureWindow X11Dynamic::Get()->p_ConfigureWindow
#define XRaiseWindow X11Dynamic::Get()->p_RaiseWindow
#define XIconifyWindow X11Dynamic::Get()->p_IconifyWindow
#define XChangeProperty X11Dynamic::Get()->p_ChangeProperty
#define XSetSelectionOwner X11Dynamic::Get()->p_SetSelectionOwner
#define XConvertSelection X11Dynamic::Get()->p_ConvertSelection
#define XGetWindowProperty X11Dynamic::Get()->p_GetWindowProperty
#define XGrabPointer X11Dynamic::Get()->p_GrabPointer
#define XUngrabPointer X11Dynamic::Get()->p_UngrabPointer
#define XWarpPointer X11Dynamic::Get()->p_WarpPointer
