/*
 * Semaphore.cpp
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS). 
 * Alle Rechte vorbehalten.
 * Copyright (C) 2005 by Christoph M�ller. Alle Rechte vorbehalten.
 */

#include "vislib/Semaphore.h"

#ifndef _WIN32
#include <ctime>
#include <fcntl.h>  // For O_CREAT. Thank you for documenting that at sem_open.
#endif /* _!WIN32 */

#include "the/assert.h"
#include "vislib/error.h"
#include "the/argument_exception.h"
#include "vislib/sysfunctions.h"
#include "the/system/system_exception.h"
#include "the/trace.h"
#include "the/not_supported_exception.h"


/*
 * vislib::sys::Semaphore::Semaphore
 */ 
vislib::sys::Semaphore::Semaphore(long initialCount, long maxCount) {
    Semaphore::enforceParamAssertions(initialCount, maxCount);

#ifdef _WIN32
    this->handle = ::CreateSemaphore(NULL, initialCount, maxCount, NULL);
    THE_ASSERT(this->handle != NULL);

#else /* _WIN32 */
    this->handle = new sem_t;
    ::sem_init(this->handle, 0, initialCount);

#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::Semaphore
 */
vislib::sys::Semaphore::Semaphore(const char *name, long initialCount, 
        long maxCount, bool *outIsNew) {
    Semaphore::enforceParamAssertions(initialCount, maxCount);

    if (outIsNew != NULL) {
        *outIsNew = false;
    }

#ifdef _WIN32
    /* Try to open existing semaphore first. */
    if ((this->handle = ::OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, 
            FALSE, name)) == NULL) {
        this->handle = ::CreateSemaphoreA(NULL, initialCount, maxCount, name);
        if (outIsNew != NULL) {
            *outIsNew = true;
        }
    }
    THE_ASSERT(this->handle != NULL);

#else /* _WIN32 */
    if (name != NULL) {
        this->name = TranslateWinIpc2PosixName(name);
        THE_TRACE(THE_TRCCHL_DEFAULT, THE_TRCLVL_INFO, "Open named POSIX semaphore \"%s\"\n", 
            this->name.c_str());
        if ((this->handle = ::sem_open(this->name.c_str(), 0, 0, 0)) 
                == SEM_FAILED) {
            this->handle = ::sem_open(this->name.c_str(), O_CREAT, 
                DFT_PERMS, initialCount);
            if (outIsNew != NULL) {
                *outIsNew = true;
            }
        }
        THE_ASSERT(this->handle !=  SEM_FAILED);
    } else {
        this->handle = new sem_t;
        ::sem_init(this->handle, 0, initialCount); 
        if (outIsNew != NULL) {
            *outIsNew = true;
        }
    }

#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::Semaphore
 */
vislib::sys::Semaphore::Semaphore(const wchar_t *name, long initialCount, 
        long maxCount, bool *outIsNew) {
    Semaphore::enforceParamAssertions(initialCount, maxCount);

    if (outIsNew != NULL) {
        *outIsNew = false;
    }

#ifdef _WIN32
    /* Try to open existing semaphore first. */
    if ((this->handle = ::OpenSemaphoreW(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, 
            FALSE, name)) == NULL) {
        this->handle = ::CreateSemaphoreW(NULL, initialCount, maxCount, name);
        if (outIsNew != NULL) {
            *outIsNew = true;
        }
    }
    THE_ASSERT(this->handle != NULL);

#else /* _WIN32 */
    if (name != NULL) {
        this->name = TranslateWinIpc2PosixName(name);
        THE_TRACE(THE_TRCCHL_DEFAULT, THE_TRCLVL_INFO, "Open named POSIX semaphore \"%ls\"\n", 
            this->name.c_str());
        if ((this->handle = ::sem_open(this->name.c_str(), 0, 0, 0)) 
                == SEM_FAILED) {
            this->handle = ::sem_open(this->name.c_str(), O_CREAT, 
                DFT_PERMS, initialCount);
            if (outIsNew != NULL) {
                *outIsNew = true;
            }
        }
        THE_ASSERT(this->handle !=  SEM_FAILED);
    } else {
        this->handle = new sem_t;
        ::sem_init(this->handle, 0, initialCount);
        if (outIsNew != NULL) {
            *outIsNew = true;
        }
    }

#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::~Semaphore(void) 
 */
vislib::sys::Semaphore::~Semaphore(void) {
#ifdef _WIN32
    ::CloseHandle(this->handle);

#else /* _WIN32 */
    if (this->name.empty()) {
        ::sem_destroy(this->handle);
        the::safe_delete(this->handle);
    } else {
        ::sem_unlink(this->name.c_str());
        ::sem_close(this->handle);
        // mueller: I assume that this->handle is owned by the system if it was
        // returned by sem_open, but the documentation says nothing about that.
        // I know, I should not be surprised about that ...
    }

#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::Lock
 */
void vislib::sys::Semaphore::Lock(void) {
#ifdef _WIN32
    switch (::WaitForSingleObject(this->handle, INFINITE)) {

        case WAIT_OBJECT_0:
            /* falls through. */
        case WAIT_ABANDONED:
            /* Does nothing. */
            break;

        case WAIT_TIMEOUT:
            /* Waiting infinitely should not timeout. */
            THE_ASSERT(false);
            break;

        default:
            throw the::system::system_exception(__FILE__, __LINE__);
    }

#else /* _WIN32 */
    if (::sem_wait(this->handle) == -1) {
        throw the::system::system_exception(__FILE__, __LINE__);
    }
#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::TryLock
 */
bool vislib::sys::Semaphore::TryLock(void) {
#ifdef _WIN32
    return this->TryLock(0);

#else /* _WIN32 */
    if (::sem_trywait(this->handle) == -1) {
        int error = ::GetLastError(); 
        if (error == EAGAIN) {
            return false;
        } else {
            throw the::system::system_exception(error, __FILE__, __LINE__);
        }
    }

    return true;
#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::TryLock
 */
bool vislib::sys::Semaphore::TryLock(const unsigned int timeout) {
#ifdef _WIN32
    switch (::WaitForSingleObject(this->handle, timeout)) {

        case WAIT_OBJECT_0:
            /* falls through. */
        case WAIT_ABANDONED:
            return true;

        case WAIT_TIMEOUT:
            return false;

        default:
            throw the::system::system_exception(__FILE__, __LINE__);
    }

#else /* _WIN32 */
    struct timespec tsEnd;
    
    ::clock_gettime(CLOCK_REALTIME, &tsEnd);
    tsEnd.tv_sec += timeout / 1000;
    tsEnd.tv_nsec += (timeout % 1000) * 1000;

    if (::sem_timedwait(this->handle, &tsEnd) == -1) {
        int error = ::GetLastError(); 
        if ((error == EAGAIN) || (error == ETIMEDOUT)) {
            return false;
        } else {
            throw the::system::system_exception(error, __FILE__, __LINE__);
        }
    }

    return true;
#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::Unlock
 */
void vislib::sys::Semaphore::Unlock(void) {
#ifdef _WIN32
    if (::ReleaseSemaphore(this->handle, 1, NULL) != TRUE) {
        throw the::system::system_exception(__FILE__, __LINE__);
    }

#else /* _WIN32 */
    if (::sem_post(this->handle) == -1) {
        throw the::system::system_exception(__FILE__, __LINE__);
    }
#endif /* _WIN32 */
}


/*
 * vislib::sys::Semaphore::Semaphore
 */
vislib::sys::Semaphore::Semaphore(const Semaphore& rhs) {
    throw the::not_supported_exception("vislib::sys::Semaphore::Semaphore",
        __FILE__, __LINE__);
}


/*
 * vislib::sys::Semaphore::enforceParamAssertions
 */
void vislib::sys::Semaphore::enforceParamAssertions(long& inOutInitialCount, 
        long& inOutMaxCount) {
    if (inOutMaxCount <= 0) {
        inOutMaxCount = 1;
    }

    if (inOutInitialCount < 0) {
        inOutInitialCount = 0;
    } else if (inOutInitialCount > inOutMaxCount) {
        inOutInitialCount = inOutMaxCount;
    }

    THE_ASSERT(inOutMaxCount > 0);
    THE_ASSERT(inOutInitialCount >= 0);
    THE_ASSERT(inOutInitialCount <= inOutMaxCount);

}

#ifndef _WIN32
/*
 * vislib::sys::Semaphore::DFT_PERMS
 */
const int vislib::sys::Semaphore::DFT_PERMS = 0666;
#endif /* !_WIN32 */


/*
 * vislib::sys::Semaphore::operator =
 */
vislib::sys::Semaphore& vislib::sys::Semaphore::operator =(
        const Semaphore& rhs) {
    if (this != &rhs) {
        throw the::argument_exception("rhs", __FILE__, __LINE__);
    }

    return *this;
}
