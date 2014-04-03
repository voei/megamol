/*
 * TrayIcon.cpp
 *
 * Copyright (C) 2006 - 2007 by Universitaet Stuttgart (VIS). 
 * Alle Rechte vorbehalten.
 */

#include "vislib/TrayIcon.h"

#include "the/assert.h"
#include "the/string.h"
#include "the/text/string_converter.h"
#include "vislib/sysfunctions.h"
#include "the/system/system_exception.h"
#include "the/trace.h"


#ifdef _WIN32

/*
 * vislib::sys::TrayIcon::TrayIcon
 */
vislib::sys::TrayIcon::TrayIcon(void) {
    this->init(::GetModuleHandleW(NULL));
}


/*
 * vislib::sys::TrayIcon::TrayIcon
 */
vislib::sys::TrayIcon::TrayIcon(HINSTANCE hInstance) {
    this->init(hInstance);
}


/*
 * vislib::sys::TrayIcon::~TrayIcon
 */
vislib::sys::TrayIcon::~TrayIcon(void) {
    try {
        this->Destroy();
    } catch (the::system::system_exception e) {
        THE_TRACE(THE_TRCCHL_DEFAULT, THE_TRCLVL_WARN, "Unexpected exception in dtor "
            "of tray icon. The exception message is: %s\n", e.what());
    }

    if (this->hWnd != NULL) {
        ::DestroyWindow(this->hWnd);
        this->hWnd = NULL;
    }
}


/* 
 * vislib::sys::TrayIcon::Create
 */
void vislib::sys::TrayIcon::Create(HWND targetWnd, const unsigned int callbackMessage,
        const unsigned int id, const wchar_t *toolTip, const HICON icon,
        const bool initiallyHidden, const wchar_t *balloonText, 
        const wchar_t *balloonTitle, const unsigned int balloonIcon, 
        const unsigned int balloonTimeout) {

    /* Destroy old icon, if any. */
    if (this->iconState != ICON_NOT_INSTALLED) {
        this->Destroy();
    }
    THE_ASSERT(this->iconState == ICON_NOT_INSTALLED);

    /*
     * Create the invisible window for controlling the tray icon, if the user
     * did not pass a valid window to receive notifications.
     */
    if ((targetWnd == NULL) || !::IsWindow(targetWnd)) {
        
        /* Register window class, if necessary. */
        if (!this->registerWndClass()) {
            throw the::system::system_exception(__FILE__, __LINE__);
        }

        /* Destroy old window, if any. */
        if (this->hWnd != NULL) {
            ::DestroyWindow(this->hWnd);
        }
        
        /* Create control window. */    
        if ((this->hWnd = ::CreateWindowW(TrayIcon::WNDCLASSNAME, L"", WS_POPUP,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                NULL, 0, this->hInstance, 0)) == NULL) {
            throw the::system::system_exception(__FILE__, __LINE__);
        }

        /* Register pointer to this object as user data. */
#pragma warning(disable: 4311)
        ::SetWindowLongPtrW(this->hWnd, GWLP_USERDATA, 
#ifdef _WIN64
            reinterpret_cast<LONG_PTR>
#else  /* _WIN64 */
            reinterpret_cast<LONG>
#endif /* _WIN64 */
            (this));
#pragma warning(default: 4311)

    } else {
        /* Use user-defined window. */
        this->hWnd = NULL;
    }
    THE_ASSERT((this->hWnd != NULL) || (targetWnd != NULL));

    /* Initialise the tray icon structure. */
    this->nid.cbSize = sizeof(this->nid);
    this->nid.hWnd = (targetWnd != NULL) ? targetWnd : this->hWnd;
    this->nid.uID = id;
    this->nid.uFlags = 0;
    this->configureIcon(icon);
    this->configureCallbackMessage(callbackMessage);
    this->configureToolTip(toolTip);
    this->configureBalloonHelp(balloonText, balloonTitle, balloonIcon, 
        balloonTimeout);

    if (initiallyHidden) {
        if (this->configureHidden(true)) {
            /* Is configured to be hidden, so can add it. */
            this->notify(NIM_ADD);
            this->iconState = ICON_HIDDEN;
        }
    } else {
        this->notify(NIM_ADD);
        this->iconState = ICON_VISIBLE;
    }
}


/*
 * vislib::sys::TrayIcon::Create
 */
void vislib::sys::TrayIcon::Create(HWND targetWnd, const unsigned int callbackMessage, 
        const unsigned int id, const char *toolTip, const HICON icon,
        const bool initiallyHidden, const char *balloonText, 
        const char *balloonTitle, const unsigned int balloonIcon, 
        const unsigned int balloonTimeout) {
    this->Create(targetWnd, 
        callbackMessage, 
        id, 
        (toolTip != NULL) ? THE_A2W(toolTip) : NULL, 
        icon, 
        initiallyHidden, 
        (balloonText != NULL) ? THE_A2W(balloonText) : NULL, 
        (balloonTitle != NULL) ? THE_A2W(balloonTitle) : NULL, 
        balloonIcon,
        balloonTimeout);
}

/*
 * vislib::sys::TrayIcon::Create
 */
void vislib::sys::TrayIcon::Create(HWND targetWnd, const unsigned int callbackMessage, 
        const unsigned int id, HINSTANCE hResourceModule, const wchar_t *toolTip, 
        const unsigned int iconID, const bool initiallyHidden, 
        const wchar_t *balloonText, const wchar_t *balloonTitle, 
        const unsigned int balloonIcon, const unsigned int balloonTimeout) {
    HINSTANCE hInst = (hResourceModule != NULL) 
        ? hResourceModule : ::GetModuleHandleW(NULL);
    HICON icon = ::LoadIconW(hResourceModule, MAKEINTRESOURCEW(iconID));

    if (icon == NULL) {
        throw the::system::system_exception(__FILE__, __LINE__);
    }

    this->Create(targetWnd, 
        callbackMessage, 
        id, 
        toolTip, 
        icon, 
        initiallyHidden,
        balloonText, 
        balloonTitle, 
        balloonIcon, 
        balloonTimeout);
    DestroyIcon(icon);
}

/*
 * vislib::sys::TrayIcon::Create
 */
void vislib::sys::TrayIcon::Create(HWND targetWnd, const unsigned int callbackMessage, 
        const unsigned int id, HINSTANCE hResourceModule, const char *toolTip, 
        const unsigned int iconID, const bool initiallyHidden, 
        const char *balloonText, const char *balloonTitle, 
        const unsigned int balloonIcon, const unsigned int balloonTimeout) {
    this->Create(targetWnd, 
        callbackMessage, 
        id, 
        hResourceModule,
        (toolTip != NULL) ? THE_A2W(toolTip) : NULL, 
        iconID, 
        initiallyHidden, 
        (balloonText != NULL) ? THE_A2W(balloonText) : NULL, 
        (balloonTitle != NULL) ? THE_A2W(balloonTitle) : NULL, 
        balloonIcon,
        balloonTimeout);
}


/*
 * vislib::sys::TrayIcon::Create
 */
void vislib::sys::TrayIcon::Create(HWND targetWnd, const unsigned int callbackMessage,
        const unsigned int id, HINSTANCE hResourceModule, const unsigned int toolTipID, 
        const unsigned int iconID, const bool initiallyHidden, const unsigned int balloonTextID, 
        const unsigned int balloonTitleID, const unsigned int balloonIcon, 
        const unsigned int balloonTimeout) {
    the::wstring toolTip, balloonText, balloonTitle;
    HINSTANCE hInst = (hResourceModule != NULL) 
        ? hResourceModule : ::GetModuleHandleW(NULL);

    if (!the::text::string_utility::load_resource(toolTip, hInst, toolTipID)) {
        toolTip.clear();
    }
    if (!the::text::string_utility::load_resource(balloonText, hInst, balloonTextID)) {
        balloonText.clear();
    }
    if (!the::text::string_utility::load_resource(balloonTitle, hInst, balloonTitleID)) {
        balloonTitle.clear();
    }

    this->Create(targetWnd,
        callbackMessage, 
        id,
        hInst,
        (!toolTip.empty()) ? toolTip.c_str() : NULL, 
        iconID,
        initiallyHidden, 
        (!balloonText.empty()) ? balloonText.c_str() : NULL,
        (!balloonTitle.empty()) ? balloonTitle.c_str() : NULL,
        balloonIcon, 
        balloonTimeout);
}


/*
 * vislib::sys::TrayIcon::Destroy
 */
void vislib::sys::TrayIcon::Destroy(void) {
    if (this->iconState != ICON_NOT_INSTALLED) {
        this->nid.uFlags = 0;
        this->notify(NIM_DELETE);
        this->iconState = ICON_NOT_INSTALLED;
    }
}


/*
 * vislib::sys::TrayIcon::Hide
 */
void vislib::sys::TrayIcon::Hide(void) {
    if (this->iconState == ICON_VISIBLE) {
        if (this->configureHidden(true)) {
            THE_ASSERT(this->capabilities >= CAPABILITIES_V2);
            THE_ASSERT(_WIN32_IE >= 0x0500);
#if (_WIN32_IE >= 0x0500)
            this->notify(NIM_MODIFY);
            this->iconState = ICON_HIDDEN;
#endif /* (_WIN32_IE >= 0x0500) */
        } else {
            this->Destroy();
        }
    } /* end if (this->iconState == ICON_VISIBLE) */
    THE_ASSERT(this->iconState != ICON_VISIBLE);
}


/*
 * vislib::sys::TrayIcon::SetIcon
 */
void vislib::sys::TrayIcon::SetIcon(HINSTANCE hResourceModule,
                                    const unsigned int iconID) {
    HINSTANCE hInst = (hResourceModule != NULL) 
        ? hResourceModule : ::GetModuleHandleW(NULL);
    HICON icon = ::LoadIconW(hResourceModule, MAKEINTRESOURCEW(iconID));
    this->SetIcon(icon);
    DestroyIcon(icon);
}


/*
 * vislib::sys::TrayIcon::Show
 */
void vislib::sys::TrayIcon::Show(void) {
    if (this->iconState != ICON_VISIBLE) {
        if ((this->iconState == ICON_HIDDEN) && this->configureHidden(false)) {
            THE_ASSERT(this->capabilities >= CAPABILITIES_V2);
            THE_ASSERT(_WIN32_IE >= 0x0500);
#if (_WIN32_IE >= 0x0500)
            this->notify(NIM_MODIFY);
#endif /* (_WIN32_IE >= 0x0500) */
        } else {
            this->notify(NIM_ADD);
        }

        this->iconState = ICON_VISIBLE;
    } /* end if (this->iconState != ICON_VISIBLE) */
    THE_ASSERT(this->iconState == ICON_VISIBLE);
}


/*
 * vislib::sys::TrayIcon::ShowBalloonHelp
 */
bool vislib::sys::TrayIcon::ShowBalloonHelp(const wchar_t *balloonText, 
        const wchar_t *balloonTitle, const unsigned int balloonIcon, 
        const unsigned int balloonTimeout) {
    unsigned int flags = 0;
    bool retval = false;
    
    flags = this->nid.uFlags;   // Preserve old flags.
    this->nid.uFlags = 0;       // Reset to have only balloon flags set.
    retval = this->configureBalloonHelp(balloonText, balloonTitle, 
        balloonIcon, balloonTimeout);
    
    if (retval) {
        try {
            this->notify(NIM_MODIFY);
        } catch (...) {
            retval = false;
        }
    }

    this->nid.uFlags = flags;   // Restore old flags.
    return retval;
}


/*
 * vislib::sys::TrayIcon::ShowBalloonHelp
 */
bool vislib::sys::TrayIcon::ShowBalloonHelp(const char *balloonText, 
        const char *balloonTitle, const unsigned int balloonIcon, 
        const unsigned int balloonTimeout) {
    return this->ShowBalloonHelp(
        (balloonText != NULL) ? THE_A2W(balloonText) : NULL,
        (balloonTitle != NULL) ? THE_A2W(balloonTitle) : NULL, 
        balloonIcon, balloonTimeout);
}


/*
 * vislib::sys::TrayIcon::ShowBalloonHelp
 */
bool vislib::sys::TrayIcon::ShowBalloonHelp(HINSTANCE hResourceModule,
        const unsigned int balloonTextID, const unsigned int balloonTitleID, 
        const unsigned int balloonIcon, const unsigned int balloonTimeout) {
    the::wstring balloonText, balloonTitle;
    HINSTANCE hInst = (hResourceModule != NULL) 
        ? hResourceModule : ::GetModuleHandleW(NULL);

    if (!the::text::string_utility::load_resource(balloonText, hInst, balloonTextID)) {
        balloonText.clear();
    }
    if (!the::text::string_utility::load_resource(balloonTitle, hInst, balloonTitleID)) {
        balloonTitle.clear();
    }

    return this->ShowBalloonHelp(
        (!balloonText.empty()) ? balloonText.c_str() : NULL,
        (!balloonTitle.empty()) ? balloonTitle.c_str() : NULL,
        balloonIcon, balloonTimeout);    
}


/*
 * vislib::sys::TrayIcon::onNotify
 */
LRESULT vislib::sys::TrayIcon::onNotify(WPARAM wParam, LPARAM lParam) {
    return static_cast<LRESULT>(0);
}


/*
 * vislib::sys::TrayIcon::wndProc
 */
LRESULT WINAPI vislib::sys::TrayIcon::wndProc(HWND hWnd, unsigned int msg, 
        WPARAM wParam, LPARAM lParam) {
#pragma warning(disable: 4312)
    TrayIcon *thisPtr = reinterpret_cast<TrayIcon *>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));
#pragma warning(default: 4312)

    if ((thisPtr != NULL) && (msg == thisPtr->nid.uCallbackMessage)) {
        return thisPtr->onNotify(wParam, lParam);
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


/* 
 * vislib::sys::TrayIcon::MAX_BALLOON_LEN
 */
const unsigned int vislib::sys::TrayIcon::MAX_BALLOON_LEN = 256;

/* 
 * vislib::sys::TrayIcon::MAX_BALLOON_TITLE_LEN
 */
const unsigned int vislib::sys::TrayIcon::MAX_BALLOON_TITLE_LEN = 64;
 

/*
 * vislib::sys::TrayIcon::MAX_TOOLTIP_LEN
 */
const unsigned int vislib::sys::TrayIcon::MAX_TOOLTIP_LEN = 64;


/*
 * vislib::sys::TrayIcon::WNDCLASSNAME
 */
const wchar_t *vislib::sys::TrayIcon::WNDCLASSNAME = L"VISLIBTRAYICONWNDCLASS";


/*
 * vislib::sys::TrayIcon::configureBalloonHelp
 */ 
bool vislib::sys::TrayIcon::configureBalloonHelp(const wchar_t *balloonText,
        const wchar_t *balloonTitle, const unsigned int balloonIcon, 
        const unsigned int balloonTimeout) {
    THE_ASSERT((balloonText == NULL)
        || (::wcslen(balloonText) < MAX_BALLOON_LEN));
    THE_ASSERT((balloonTitle == NULL) 
        || (::wcslen(balloonTitle) < MAX_BALLOON_TITLE_LEN));
    THE_ASSERT((balloonTimeout >= 10) && (balloonTimeout <= 30));
#if (_WIN32_IE >= 0x0500)
    THE_ASSERT((balloonIcon == NIIF_NONE) || (balloonIcon == NIIF_INFO)
        || (balloonIcon == NIIF_WARNING) || (balloonIcon == NIIF_ERROR));
#endif /* (_WIN32_IE >= 0x0500) */

    bool retval = false;

#if (_WIN32_IE >= 0x0500)
    if (this->capabilities >= CAPABILITIES_V2) {

        if (balloonText != NULL) {
#if (_MSC_VER >= 1400)
            ::wcsncpy_s(this->nid.szInfo, MAX_BALLOON_LEN, balloonText, 
                MAX_BALLOON_LEN);
#else  /*(_MSC_VER >= 1400) */
            ::wcsncpy(this->nid.szInfo, balloonText, MAX_BALLOON_LEN);
#endif /*(_MSC_VER >= 1400) */
            this->nid.szInfo[MAX_BALLOON_LEN - 1] = 0;
        } else {
            this->nid.szInfo[0] = 0;
        }

        if (balloonTitle != NULL) {
#if (_MSC_VER >= 1400)
            ::wcsncpy_s(this->nid.szInfoTitle, MAX_BALLOON_TITLE_LEN,
                balloonTitle, MAX_BALLOON_TITLE_LEN);
#else  /*(_MSC_VER >= 1400) */
            ::wcsncpy(this->nid.szInfoTitle, balloonTitle, 
                MAX_BALLOON_TITLE_LEN);
#endif /*(_MSC_VER >= 1400) */
        this->nid.szInfoTitle[MAX_BALLOON_TITLE_LEN - 1] = 0;

        } else {
            this->nid.szInfoTitle[0] = 0;
        }

        this->nid.uTimeout = balloonTimeout * 1000;
        this->nid.dwInfoFlags = balloonIcon;

        this->nid.uFlags |= NIF_INFO;

        retval = true;
    } /* end if (this->capabilities >= CAPABILITIES_V2) */

#endif /* (_WIN32_IE >= 0x0500) */

    return retval;
}


/*
 * vislib::sys::TrayIcon::configureCallbackMessage
 */
bool vislib::sys::TrayIcon::configureCallbackMessage(
        const unsigned int callbackMessage) {
    THE_ASSERT(callbackMessage >= WM_APP);

    this->nid.uCallbackMessage = callbackMessage;
    this->nid.uFlags |= NIF_MESSAGE;

    return true;
}


/*
 * vislib::sys::TrayIcon::configureIcon
 */
bool vislib::sys::TrayIcon::configureIcon(const HICON icon) {
    THE_ASSERT(icon != NULL);

    this->nid.hIcon = icon;
    this->nid.uFlags |= NIF_ICON;

    return true;
}


/*
 * vislib::sys::TrayIcon::configureHidden
 */
bool vislib::sys::TrayIcon::configureHidden(const bool isHidden) {
    bool retval = false;

#if (_WIN32_IE >= 0x0500)
    if (this->capabilities >= CAPABILITIES_V2) {
        if (isHidden) {
            this->nid.dwState |= NIS_HIDDEN;
        } else {
            this->nid.dwState &= ~NIS_HIDDEN;
        }
        this->nid.dwStateMask = NIS_HIDDEN;

        this->nid.uFlags |= NIF_STATE;

        retval = true;
    }
#endif /* (_WIN32_IE >= 0x0500) */

    return retval;
}


/*
 * vislib::sys::TrayIcon::configureToolTip
 */
bool vislib::sys::TrayIcon::configureToolTip(const wchar_t *toolTip) {
    THE_ASSERT(::wcslen(toolTip) < MAX_TOOLTIP_LEN);

#if (_MSC_VER >= 1400)
    ::wcsncpy_s(this->nid.szTip, MAX_TOOLTIP_LEN, toolTip, MAX_TOOLTIP_LEN);
#else  /*(_MSC_VER >= 1400) */
    ::wcsncpy(this->nid.szTip, toolTip, MAX_TOOLTIP_LEN);
#endif /*(_MSC_VER >= 1400) */
    this->nid.szTip[MAX_TOOLTIP_LEN - 1] = 0;

    this->nid.uFlags |= NIF_TIP;

    return true;
}


/*
 * vislib::sys::TrayIcon::init
 */
bool vislib::sys::TrayIcon::init(HINSTANCE hInstance) {
    DLLVERSIONINFO shellVersion;
    ::ZeroMemory(&shellVersion, sizeof(DLLVERSIONINFO));
    shellVersion.cbSize = sizeof(DLLVERSIONINFO);

    /* Simple initialisations. */
    this->capabilities = CAPABILITIES_NONE;
    this->hInstance = hInstance;
    this->hWnd = NULL;
    this->iconState = ICON_NOT_INSTALLED;
    ::ZeroMemory(&this->nid, sizeof(this->nid));

#if (_WIN32_IE >= 0x0500)
    /* Determine shell version. */
    try {
        if (GetDLLVersion(shellVersion, "shell32.dll") != NOERROR) {
            return false;
        }
    } catch (the::system::system_exception e) {
        THE_TRACE(THE_TRCCHL_DEFAULT, THE_TRCLVL_ERROR, "Retrieving shell version failed: "
            "%s (%u).\n", e.what(), e.get_error().native_error());
        return false;
    }

    if (shellVersion.dwMajorVersion >= 5) {
        this->capabilities = CAPABILITIES_V2;
    } else {
        this->capabilities = CAPABILITIES_V1;
    }

#else /* (_WIN32_IE >= 0x0500) */
    this->capabilities = CAPABILITIES_V1;
#endif /* (_WIN32_IE >= 0x0500) */

    return true;
}


/*
 * vislib::sys::TrayIcon::notify
 */
void vislib::sys::TrayIcon::notify(const unsigned int message) {
    if (!::Shell_NotifyIconW(message, &this->nid)) {
        throw the::system::system_exception(__FILE__, __LINE__);
    }
    this->nid.uFlags = 0;
}


/*
 * vislib::sys::TrayIcon::registerWndClass
 */
bool vislib::sys::TrayIcon::registerWndClass(void) {
    WNDCLASSEXW wndClass;

    if (!::GetClassInfoExW(this->hInstance, WNDCLASSNAME, &wndClass)) {
        wndClass.cbSize = sizeof(WNDCLASSEX); 

        wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wndClass.lpfnWndProc = reinterpret_cast<WNDPROC>(TrayIcon::wndProc);
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra	= 0;
        wndClass.hInstance = this->hInstance;
        wndClass.hIcon = 0;
        wndClass.hCursor = 0;
        wndClass.hbrBackground = 0;
        wndClass.lpszMenuName = 0;
        wndClass.lpszClassName = WNDCLASSNAME;
        wndClass.hIconSm = 0;

        return (::RegisterClassExW(&wndClass) != FALSE);

    } else {
        /* Window class already registered. */
        return true;
    }
}

#endif /* _WIN32 */
