#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>
#include <dlfcn.h>
#include <stdexcept>

struct X11Dynamic
{
	typedef int (*PFN_XDefaultDepth)(Display*, int);
	typedef Visual* (*PFN_XDefaultVisual)(Display*, int);
	typedef Colormap (*PFN_XDefaultColormap)(Display*, int);
	typedef char* (*PFN_XGetDefault)(Display*, const char*, const char*);
	typedef int (*PFN_XFreePixmap)(Display*, Pixmap);
	typedef Pixmap (*PFN_XCreateBitmapFromData)(Display*, Drawable, const char*, unsigned int, unsigned int);
	typedef Cursor (*PFN_XCreatePixmapCursor)(Display*, Pixmap, Pixmap, XColor*, XColor*, unsigned int, unsigned int);
	typedef XImage* (*PFN_XCreateImage)(Display*, Visual*, unsigned int, int, int, char*, unsigned int, unsigned int, int, int);
	typedef Pixmap (*PFN_XCreatePixmap)(Display*, Drawable, unsigned int, unsigned int, unsigned int);
	typedef int (*PFN_XDestroyImage)(XImage*);
	typedef GC (*PFN_XDefaultGC)(Display*, int);
	typedef int (*PFN_XPutImage)(Display*, Drawable, GC, XImage*, int, int, int, int, unsigned int, unsigned int);
	typedef int (*PFN_XCopyArea)(Display*, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int);
	typedef unsigned long (*PFN_XBlackPixel)(Display*, int);
	typedef unsigned long (*PFN_XWhitePixel)(Display*, int);
	typedef Bool (*PFN_XCheckTypedWindowEvent)(Display*, Window, int, XEvent*);
	typedef Display* (*PFN_XOpenDisplay)(const char*);
	typedef int (*PFN_XCloseDisplay)(Display*);
	typedef Status (*PFN_XInitThreads)();
	typedef int (*PFN_XkbSetDetectableAutoRepeat)(Display*, Bool, Bool*);
	typedef char* (*PFN_XSetLocaleModifiers)(const char*);
	typedef XIM (*PFN_XOpenIM)(Display*, struct _XrmHashBucketRec*, char*, char*);
	typedef int (*PFN_XCloseIM)(XIM);
	typedef Bool (*PFN_XQueryExtension)(Display*, const char*, int*, int*, int*);
	typedef Status (*PFN_XIQueryVersion)(Display*, int*, int*);
	typedef XIDeviceInfo* (*PFN_XIQueryDevice)(Display*, int, int*);
	typedef void (*PFN_XIFreeDeviceInfo)(XIDeviceInfo*);
	typedef Window (*PFN_XRootWindow)(Display*, int);
	typedef int (*PFN_XDefaultScreen)(Display*);
	typedef Status (*PFN_XISelectEvents)(Display*, Window, XIEventMask*, int);
	typedef int (*PFN_XFreeCursor)(Display*, Cursor);
	typedef Atom (*PFN_XInternAtom)(Display*, const char*, Bool);
	typedef int (*PFN_XPending)(Display*);
	typedef int (*PFN_XNextEvent)(Display*, XEvent*);
	typedef int (*PFN_XConnectionNumber)(Display*);
	typedef Bool (*PFN_XGetEventData)(Display*, XGenericEventCookie*);
	typedef void (*PFN_XFreeEventData)(Display*, XGenericEventCookie*);
	typedef int (*PFN_XDisplayWidth)(Display*, int);
	typedef int (*PFN_XDisplayHeight)(Display*, int);
	typedef int (*PFN_XDisplayWidthMM)(Display*, int);
	typedef Cursor (*PFN_XCreateFontCursor)(Display*, unsigned int);
	typedef Window (*PFN_XCreateWindow)(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
	typedef int (*PFN_XDestroyWindow)(Display*, Window);
	typedef int (*PFN_XMapRaised)(Display*, Window);
	typedef int (*PFN_XUnmapWindow)(Display*, Window);
	typedef int (*PFN_XSelectInput)(Display*, Window, long);
	typedef int (*PFN_XSendEvent)(Display*, Window, Bool, long, XEvent*);
	typedef int (*PFN_XSetWMProtocols)(Display*, Window, Atom*, int);
	typedef void (*PFN_XSetWMHints)(Display*, Window, XWMHints*);
	typedef void (*PFN_XSetStandardProperties)(Display*, Window, const char*, const char*, Pixmap, char**, int, XSizeHints*);
	typedef int (*PFN_XSetTransientForHint)(Display*, Window, Window);
	typedef int (*PFN_XDefineCursor)(Display*, Window, Cursor);
	typedef int (*PFN_XGetWindowAttributes)(Display*, Window, XWindowAttributes*);
	typedef int (*PFN_XSetInputFocus)(Display*, Window, int, Time);
	typedef XIC (*PFN_XCreateIC)(XIM, ...);
	typedef void (*PFN_XSetICFocus)(XIC);
	typedef void (*PFN_XUnsetICFocus)(XIC);
	typedef void (*PFN_XDestroyIC)(XIC);
	typedef int (*PFN_Xutf8LookupString)(XIC, XKeyPressedEvent*, char*, int, KeySym*, Status*);
	typedef int (*PFN_XLookupString)(XKeyPressedEvent*, char*, int, KeySym*, XComposeStatus*);
	typedef KeySym (*PFN_XkbKeycodeToKeysym)(Display*, KeyCode, int, int);
	typedef int (*PFN_XFree)(void*);
	typedef int (*PFN_XFlush)(Display*);
	typedef int (*PFN_XSync)(Display*, Bool);
	typedef int (*PFN_XTranslateCoordinates)(Display*, Window, Window, int, int, int*, int*, Window*);
	typedef int (*PFN_XGetGeometry)(Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*);
	typedef int (*PFN_XConfigureWindow)(Display*, Window, unsigned int, XWindowChanges*);
	typedef int (*PFN_XRaiseWindow)(Display*, Window);
	typedef int (*PFN_XIconifyWindow)(Display*, Window, int);
	typedef int (*PFN_XChangeProperty)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int);
	typedef int (*PFN_XSetSelectionOwner)(Display*, Atom, Window, Time);
	typedef int (*PFN_XConvertSelection)(Display*, Atom, Atom, Atom, Window, Time);
	typedef int (*PFN_XGetWindowProperty)(Display*, Window, Atom, long, long, Bool, Atom, Atom*, int*, unsigned long*, unsigned long*, unsigned char**);
	typedef int (*PFN_XGrabPointer)(Display*, Window, Bool, unsigned int, int, int, Window, Cursor, Time);
	typedef int (*PFN_XUngrabPointer)(Display*, Time);
	typedef int (*PFN_XWarpPointer)(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int);

	PFN_XOpenDisplay p_OpenDisplay;
	PFN_XCloseDisplay p_CloseDisplay;
	PFN_XInitThreads p_InitThreads;
	PFN_XkbSetDetectableAutoRepeat p_SetDetectableAutoRepeat;
	PFN_XSetLocaleModifiers p_SetLocaleModifiers;
	PFN_XOpenIM p_OpenIM;
	PFN_XCloseIM p_CloseIM;
	PFN_XQueryExtension p_QueryExtension;
	PFN_XIQueryVersion p_IQueryVersion;
	PFN_XIQueryDevice p_IQueryDevice;
	PFN_XIFreeDeviceInfo p_IFreeDeviceInfo;
	PFN_XRootWindow p_RootWindow;
	PFN_XDefaultScreen p_DefaultScreen;
	PFN_XDefaultDepth p_DefaultDepth;
	PFN_XDefaultVisual p_DefaultVisual;
	PFN_XDefaultColormap p_DefaultColormap;
	PFN_XGetDefault p_GetDefault;
	PFN_XISelectEvents p_ISelectEvents;
	PFN_XFreeCursor p_FreeCursor;
	PFN_XFreePixmap p_FreePixmap;
	PFN_XCreateBitmapFromData p_CreateBitmapFromData;
	PFN_XCreatePixmapCursor p_CreatePixmapCursor;
	PFN_XCreateImage p_CreateImage;
	PFN_XCreatePixmap p_CreatePixmap;
	PFN_XDestroyImage p_DestroyImage;
	PFN_XDefaultGC p_DefaultGC;
	PFN_XPutImage p_PutImage;
	PFN_XCopyArea p_CopyArea;
	PFN_XBlackPixel p_BlackPixel;
	PFN_XWhitePixel p_WhitePixel;
	PFN_XCheckTypedWindowEvent p_CheckTypedWindowEvent;
	PFN_XInternAtom p_InternAtom;
	PFN_XPending p_Pending;
	PFN_XNextEvent p_NextEvent;
	PFN_XConnectionNumber p_ConnectionNumber;
	PFN_XGetEventData p_GetEventData;
	PFN_XFreeEventData p_FreeEventData;
	PFN_XDisplayWidth p_DisplayWidth;
	PFN_XDisplayHeight p_DisplayHeight;
	PFN_XDisplayWidthMM p_DisplayWidthMM;
	PFN_XCreateFontCursor p_CreateFontCursor;
	PFN_XCreateWindow p_CreateWindow;
	PFN_XDestroyWindow p_DestroyWindow;
	PFN_XMapRaised p_MapRaised;
	PFN_XUnmapWindow p_UnmapWindow;
	PFN_XSelectInput p_SelectInput;
	PFN_XSendEvent p_SendEvent;
	PFN_XSetWMProtocols p_SetWMProtocols;
	PFN_XSetWMHints p_SetWMHints;
	PFN_XSetStandardProperties p_SetStandardProperties;
	PFN_XSetTransientForHint p_SetTransientForHint;
	PFN_XDefineCursor p_DefineCursor;
	PFN_XGetWindowAttributes p_GetWindowAttributes;
	PFN_XSetInputFocus p_SetInputFocus;
	PFN_XCreateIC p_CreateIC;
	PFN_XSetICFocus p_SetICFocus;
	PFN_XUnsetICFocus p_UnsetICFocus;
	PFN_XDestroyIC p_DestroyIC;
	PFN_Xutf8LookupString p_utf8LookupString;
	PFN_XLookupString p_LookupString;
	PFN_XkbKeycodeToKeysym p_kbKeycodeToKeysym;
	PFN_XFree p_Free;
	PFN_XFlush p_Flush;
	PFN_XSync p_Sync;
	PFN_XTranslateCoordinates p_TranslateCoordinates;
	PFN_XGetGeometry p_GetGeometry;
	PFN_XConfigureWindow p_ConfigureWindow;
	PFN_XRaiseWindow p_RaiseWindow;
	PFN_XIconifyWindow p_IconifyWindow;
	PFN_XChangeProperty p_ChangeProperty;
	PFN_XSetSelectionOwner p_SetSelectionOwner;
	PFN_XConvertSelection p_ConvertSelection;
	PFN_XGetWindowProperty p_GetWindowProperty;
	PFN_XGrabPointer p_GrabPointer;
	PFN_XUngrabPointer p_UngrabPointer;
	PFN_XWarpPointer p_WarpPointer;

	static X11Dynamic* Get()
	{
		static X11Dynamic instance;
		return &instance;
	}

private:
	X11Dynamic()
	{
		void* x11 = dlopen("libX11.so.6", RTLD_NOW | RTLD_GLOBAL);
		if (!x11) {
			fprintf(stderr, "X11Dynamic: Failed to load libX11.so.6: %s\n", dlerror());
			throw std::runtime_error("Could not load libX11.so.6");
		}
		fprintf(stderr, "X11Dynamic: Successfully loaded libX11.so.6\n");

		void* xi = dlopen("libXi.so.6", RTLD_NOW | RTLD_GLOBAL);

		#define LOAD_SYM(lib, name) p_##name = (PFN_X##name)dlsym(lib, "X" #name);

		LOAD_SYM(x11, OpenDisplay);
		LOAD_SYM(x11, CloseDisplay);
		LOAD_SYM(x11, InitThreads);
		p_SetDetectableAutoRepeat = (PFN_XkbSetDetectableAutoRepeat)dlsym(x11, "XkbSetDetectableAutoRepeat");
		LOAD_SYM(x11, SetLocaleModifiers);
		LOAD_SYM(x11, OpenIM);
		LOAD_SYM(x11, CloseIM);
		LOAD_SYM(x11, QueryExtension);
		LOAD_SYM(x11, RootWindow);
		LOAD_SYM(x11, DefaultScreen);
		LOAD_SYM(x11, DefaultDepth);
		LOAD_SYM(x11, DefaultVisual);
		LOAD_SYM(x11, DefaultColormap);
		LOAD_SYM(x11, GetDefault);
		LOAD_SYM(x11, FreeCursor);
		LOAD_SYM(x11, FreePixmap);
		LOAD_SYM(x11, CreateBitmapFromData);
		LOAD_SYM(x11, CreatePixmapCursor);
		LOAD_SYM(x11, CreateImage);
		LOAD_SYM(x11, CreatePixmap);
		LOAD_SYM(x11, DestroyImage);
		LOAD_SYM(x11, DefaultGC);
		LOAD_SYM(x11, PutImage);
		LOAD_SYM(x11, CopyArea);
		LOAD_SYM(x11, BlackPixel);
		LOAD_SYM(x11, WhitePixel);
		LOAD_SYM(x11, CheckTypedWindowEvent);
		LOAD_SYM(x11, InternAtom);
		LOAD_SYM(x11, Pending);
		LOAD_SYM(x11, NextEvent);
		LOAD_SYM(x11, ConnectionNumber);
		LOAD_SYM(x11, GetEventData);
		LOAD_SYM(x11, FreeEventData);
		LOAD_SYM(x11, DisplayWidth);
		LOAD_SYM(x11, DisplayHeight);
		LOAD_SYM(x11, DisplayWidthMM);
		LOAD_SYM(x11, CreateFontCursor);
		LOAD_SYM(x11, CreateWindow);
		LOAD_SYM(x11, DestroyWindow);
		LOAD_SYM(x11, MapRaised);
		LOAD_SYM(x11, UnmapWindow);
		LOAD_SYM(x11, SelectInput);
		LOAD_SYM(x11, SendEvent);
		LOAD_SYM(x11, SetWMProtocols);
		LOAD_SYM(x11, SetWMHints);
		LOAD_SYM(x11, SetStandardProperties);
		LOAD_SYM(x11, SetTransientForHint);
		LOAD_SYM(x11, DefineCursor);
		LOAD_SYM(x11, GetWindowAttributes);
		LOAD_SYM(x11, SetInputFocus);
		LOAD_SYM(x11, CreateIC);
		LOAD_SYM(x11, SetICFocus);
		LOAD_SYM(x11, UnsetICFocus);
		LOAD_SYM(x11, DestroyIC);
		LOAD_SYM(x11, utf8LookupString);
		LOAD_SYM(x11, LookupString);
		p_kbKeycodeToKeysym = (PFN_XkbKeycodeToKeysym)dlsym(x11, "XkbKeycodeToKeysym");
		LOAD_SYM(x11, Free);
		LOAD_SYM(x11, Flush);
		LOAD_SYM(x11, Sync);
		LOAD_SYM(x11, TranslateCoordinates);
		LOAD_SYM(x11, GetGeometry);
		LOAD_SYM(x11, ConfigureWindow);
		LOAD_SYM(x11, RaiseWindow);
		LOAD_SYM(x11, IconifyWindow);
		LOAD_SYM(x11, ChangeProperty);
		LOAD_SYM(x11, SetSelectionOwner);
		LOAD_SYM(x11, ConvertSelection);
		LOAD_SYM(x11, GetWindowProperty);
		LOAD_SYM(x11, GrabPointer);
		LOAD_SYM(x11, UngrabPointer);
		LOAD_SYM(x11, WarpPointer);

		if (xi)
		{
			p_IQueryVersion = (PFN_XIQueryVersion)dlsym(xi, "XIQueryVersion");
			p_IQueryDevice = (PFN_XIQueryDevice)dlsym(xi, "XIQueryDevice");
			p_IFreeDeviceInfo = (PFN_XIFreeDeviceInfo)dlsym(xi, "XIFreeDeviceInfo");
			p_ISelectEvents = (PFN_XISelectEvents)dlsym(xi, "XISelectEvents");
		}
	}
};
