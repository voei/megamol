/*
 * VisIttDataSource.cpp
 *
 * Copyright (C) 2009-2011 by VISUS (Universitaet Stuttgart)
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include "VisIttDataSource.h"
#include "vislib/MemmappedFile.h"
#include "param/EnumParam.h"
#include "param/FilePathParam.h"
#include "param/FloatParam.h"
#include "param/StringParam.h"
#include "MultiParticleDataCall.h"
#include "CoreInstance.h"
#include "vislib/Log.h"
#include "vislib/String.h"
#include "vislib/sysfunctions.h"
#include "vislib/SystemInformation.h"

using namespace megamol::core;


/* defines for the frame cache size */
// minimum number of frames in the cache (2 for interpolation; 1 for loading)
#define CACHE_SIZE_MIN 3
// maximum number of frames in the cache (just a nice number)
#define CACHE_SIZE_MAX 1000
// factor multiplied to the frame size for estimating the overhead to the pure data.
#define CACHE_FRAME_FACTOR 1.15f

/*****************************************************************************/

/*
 * moldyn::VisIttDataSource::Frame::Frame
 */
moldyn::VisIttDataSource::Frame::Frame(view::AnimDataModule& owner)
        : view::AnimDataModule::Frame(owner), dat(), size(0) {
    // intentionally empty
}


/*
 * moldyn::VisIttDataSource::Frame::~Frame
 */
moldyn::VisIttDataSource::Frame::~Frame() {
    // intentionally empty
}

/*****************************************************************************/


/*
 * moldyn::VisIttDataSource::VisIttDataSource
 */
moldyn::VisIttDataSource::VisIttDataSource(void) : view::AnimDataModule(),
        filename("filename", "The path to the trisoup file to load."),
        radius("radius", "The radius to be assumed for the particles"),
        filter("filter::type", "The filter to be applied"),
        filterColumn("filter::column", "The filter column to be applied"),
        filterValue("filter::value", "The filter value to be applied"),
        getData("getdata", "Slot to request data from this data source."),
        file(NULL), dataHash(0), frameTable(), header(), headerIdx(),
        filterIndex(UINT_MAX) {

    this->filename.SetParameter(new param::FilePathParam(""));
    this->filename.SetUpdateCallback(&VisIttDataSource::filenameChanged);
    this->MakeSlotAvailable(&this->filename);

    this->radius << new param::FloatParam(0.5f, 0.00001f);
    this->MakeSlotAvailable(&this->radius);

    param::EnumParam *filterTypes = new param::EnumParam(0);
    filterTypes->SetTypePair(0, "none");
    filterTypes->SetTypePair(1, "=");
    filterTypes->SetTypePair(6, "!=");
    filterTypes->SetTypePair(2, "<");
    filterTypes->SetTypePair(3, ">");
    filterTypes->SetTypePair(4, "<=");
    filterTypes->SetTypePair(5, ">=");
    this->filter.SetParameter(filterTypes);
    this->filter.SetUpdateCallback(&VisIttDataSource::filterChanged);
    this->MakeSlotAvailable(&this->filter);

    this->filterColumn << new param::StringParam("");
    this->filterColumn.SetUpdateCallback(&VisIttDataSource::filterChanged);
    this->MakeSlotAvailable(&this->filterColumn);

    this->filterValue << new param::FloatParam(0.0f);
    this->filterValue.SetUpdateCallback(&VisIttDataSource::filterChanged);
    this->MakeSlotAvailable(&this->filterValue);

    this->getData.SetCallback("MultiParticleDataCall", "GetData",
        &VisIttDataSource::getDataCallback);
    this->getData.SetCallback("MultiParticleDataCall", "GetExtent",
        &VisIttDataSource::getExtentCallback);
    this->MakeSlotAvailable(&this->getData);

    this->setFrameCount(1);
    this->initFrameCache(1);
}


/*
 * moldyn::VisIttDataSource::~VisIttDataSource
 */
moldyn::VisIttDataSource::~VisIttDataSource(void) {
    this->Release(); // implicitly calls 'release'
}


/*
 * moldyn::VisIttDataSource::constructFrame
 */
view::AnimDataModule::Frame*
moldyn::VisIttDataSource::constructFrame(void) const {
    Frame *f = new Frame(*const_cast<moldyn::VisIttDataSource*>(this));
    return f;
}


/*
 * moldyn::VisIttDataSource::create
 */
bool moldyn::VisIttDataSource::create(void) {
    return true;
}


/*
 * moldyn::VisIttDataSource::loadFrame
 */
void moldyn::VisIttDataSource::loadFrame(view::AnimDataModule::Frame *frame, unsigned int idx) {
    Frame *f = dynamic_cast<Frame*>(frame);
    if (f == NULL) return;
    if (this->file == NULL) {
        f->SetSize(0);
        return;
    }
    ASSERT(idx < this->FrameCount());
    ASSERT(this->headerIdx.Count() >= 3);

    SIZE_T len = ((idx + 1 < this->frameTable.Count()) ? this->frameTable[idx + 1] : this->file->GetSize()) - this->frameTable[idx];
    this->file->Seek(this->frameTable[idx]);
    char *buf = new char[len + 2];
    len = this->file->Read(buf, len);
    buf[len] = '\n';
    buf[len + 1] = 0;
    int filterType = this->filter.Param<param::EnumParam>()->Value();
    float filterVal = this->filterValue.Param<param::FloatParam>()->Value();
    for (SIZE_T i = 0; i <= len; i++) {
        char *line = buf + i;
        if (line[0] == '#') break; // end of frame
        while ((i <= len) && (buf[i] != '\n')) i++;
        if (i > len) break;
        buf[i] = '0';

        if ((filterType > 0) && (this->filterIndex < this->header.Count())) {
            unsigned int start = 0;
            for (unsigned int j = 0; j < this->filterIndex; j++) {
                start += this->header[j].Second();
            }
            unsigned int end = start + this->header[this->filterIndex].Second();
            char endChar = line[end];
            line[end] = 0;
            bool doFilter = false;
            try {
                float v = static_cast<float>(vislib::CharTraitsA::ParseDouble(line + start));
                switch (filterType) {
                    case 1: // =
                        doFilter = vislib::math::IsEqual(v, filterVal);
                        break;
                    case 6: // !=
                        doFilter = !vislib::math::IsEqual(v, filterVal);
                        break;
                    case 2: // <
                        doFilter = (v < filterVal);
                        break;
                    case 3: // >
                        doFilter = (v > filterVal);
                        break;
                    case 4: // <=
                        doFilter = (v <= filterVal);
                        break;
                    case 5: // >=
                        doFilter = (v >= filterVal);
                        break;
                    default: // do not filter
                        break;
                }
            } catch(...) {
            }
            line[end] = endChar;
            if (doFilter) continue;
        }

        const SIZE_T PAGESIZE = 1024 * sizeof(float) * 3;
        f->Data().AssertSize(((f->Size() + 3 * sizeof(float)) / PAGESIZE + 1) * PAGESIZE);
        float *pos = f->Data().AsAt<float>(f->Size());
        for (unsigned int j = 0; j < 3; j++) {
            unsigned int hidx = this->headerIdx[j];
            unsigned int start = 0;
            for (unsigned int k = 0; k < hidx; k++) {
                start += this->header[k].Second();
            }
            unsigned int end = start + this->header[hidx].Second();
            char endChar = line[end];
            line[end] = 0;
            try {
                pos[j] = static_cast<float>(vislib::CharTraitsA::ParseDouble(line + start));
            } catch(...) {
                pos[j] = 0.0f;
            }
            line[end] = endChar;
        }
        f->SetSize(f->Size() + 3 * sizeof(float));

    }
    delete[] buf;
    f->SetFrameNumber(idx);

    vislib::sys::Log::DefaultLog.WriteInfo(100, "Frame %u loaded", idx);
}


/*
 * moldyn::VisIttDataSource::release
 */
void moldyn::VisIttDataSource::release(void) {
    this->resetFrameCache();
    if (this->file != NULL) {
        vislib::sys::File *f = this->file;
        this->file = NULL;
        f->Close();
        delete f;
    }
    this->frameTable.Clear();
    this->header.Clear();
    this->headerIdx.Clear();
}


/*
 * moldyn::VisIttDataSource::buildFrameTable
 */
void moldyn::VisIttDataSource::buildFrameTable(void) {
    ASSERT(this->file != NULL);

    const unsigned int bufSize = 1024 * 1024;
    char *buf = new char[bufSize];
    unsigned int size = 1;
    char lCh1 = 0, lCh2 = 0;
    vislib::sys::File::FileSize pos = 0;
    unsigned int i;

    while (!this->file->IsEOF()) {
        size = static_cast<unsigned int>(this->file->Read(buf, bufSize));
        if (size == 0) {
            break;
        }

        if (lCh1 == '#') {
            if (buf[0] == '#') {
                break; // end of data
            }
            if ((lCh2 == 0x0D) || (lCh2 == 0x0A)) {
                this->frameTable.Add(pos - 1);
            }
        }

        for (i = 0; i < size - 1; i++) {
            if (buf[i] == '#') {
                if (buf[i + 1] == '#') {
                    break; // end of data
                }
                if (((i == 0) && ((lCh1 == 0x0D) || (lCh1 == 0x0A)))
                        || ((i > 0) && ((buf[i - 1] == 0x0D) || (buf[i - 1] == 0x0A)))) {
                    this->frameTable.Add(pos + i);
                }
            }
        }
        if ((i < size - 1) && (buf[i] == '#') && (buf[i + 1] == '#')) {
            break; // end of data
        }

        if (size > 1) {
            lCh2 = buf[size - 2];
            lCh1 = buf[size - 1];
        } else if (size == 1) {
            lCh2 = lCh1;
            lCh1 = buf[0];
        }

        pos += size;
    }

    this->file->SeekToBegin(); // seek back to the beginning of the file for the real loading
    this->file->Read(buf, 1);  // paranoia for fixing IsEOF under Linux
    this->file->SeekToBegin();

    delete[] buf;

    for (SIZE_T i = 1; i < this->frameTable.Count(); i++) {
        this->frameTable[i] += this->frameTable[0] + 2; // header offset
    }

}


/*
 * moldyn::VisIttDataSource::filenameChanged
 */
bool moldyn::VisIttDataSource::filenameChanged(param::ParamSlot& slot) {
    this->resetFrameCache();
    this->dataHash++;

    if (this->file == NULL) {
        this->file = new vislib::sys::MemmappedFile();
    } else {
        this->file->Close();
    }
    ASSERT(this->filename.Param<param::FilePathParam>() != NULL);

    if (!this->file->Open(this->filename.Param<param::FilePathParam>()->Value(),
            vislib::sys::File::READ_ONLY, vislib::sys::File::SHARE_READ, vislib::sys::File::OPEN_ONLY)) {
        this->GetCoreInstance()->Log().WriteError("Unable to open VisItt-File \"%s\".",
            vislib::StringA(this->filename.Param<param::FilePathParam>()->Value()).PeekBuffer());

        SAFE_DELETE(this->file);
        this->setFrameCount(1);
        this->initFrameCache(1);

        return true;
    }

    this->frameTable.Clear();
    this->header.Clear();
    this->headerIdx.Clear();

    // read header line
    this->file->SeekToBegin();
    vislib::StringA header = vislib::sys::ReadLineFromFileA(*this->file);
    if (!this->parseHeader(header)) {
        this->GetCoreInstance()->Log().WriteError("Unable to parse VisItt-file header line");

        this->file->Close();
        SAFE_DELETE(this->file);
        this->setFrameCount(1);
        this->initFrameCache(1);

        return true;
    }

    this->frameTable.Add(this->file->Tell());
    this->buildFrameTable();
    this->setFrameCount(static_cast<unsigned int>(this->frameTable.Count()));
    Frame tmpFrame(*this);
    this->loadFrame(&tmpFrame, 0);

    // calculating the bounding box from frame 0
    this->bbox.Set(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0);
    if (tmpFrame.Size() > sizeof(float) * 3) {
        float *pos = tmpFrame.Data().As<float>();
        this->bbox.Set(pos[0], pos[1], pos[2], pos[0], pos[1], pos[2]);
        unsigned int cnt = static_cast<unsigned int>(tmpFrame.Size() / (sizeof(float) * 3));
        for (unsigned int i = 1; i < cnt; i++) {
            pos += 3;
            this->bbox.GrowToPoint(pos[0], pos[1], pos[2]);
        }
    }

    //tmpFrame.SetTypeCount(this->typeCnt);
    // use frame zero to estimate the frame size in memory to calculate the
    // frame cache size
    SIZE_T frameSize = tmpFrame.Data().GetSize();
    tmpFrame.Data().EnforceSize(0);
    tmpFrame.SetSize(0);
    frameSize = static_cast<SIZE_T>(float(frameSize) * CACHE_FRAME_FACTOR);
    UINT64 mem = vislib::sys::SystemInformation::AvailableMemorySize();
    unsigned int cacheSize = static_cast<unsigned int>(mem / frameSize);

    if (cacheSize > CACHE_SIZE_MAX) {
        cacheSize = CACHE_SIZE_MAX;
    }
    if (cacheSize < CACHE_SIZE_MIN) {
        vislib::StringA msg;
        msg.Format("Frame cache size forced to %i. Calculated size was %u.\n",
            CACHE_SIZE_MIN, cacheSize);
        this->GetCoreInstance()->Log().WriteMsg(vislib::sys::Log::LEVEL_WARN, msg);
        cacheSize = CACHE_SIZE_MIN;
    } else {
        vislib::StringA msg;
        msg.Format("Frame cache size set to %i.\n", cacheSize);
        this->GetCoreInstance()->Log().WriteMsg(vislib::sys::Log::LEVEL_INFO, msg);
    }

    if (this->frameTable.Count() > 0) {
        // refine bounding box using more frames
        this->loadFrame(&tmpFrame, static_cast<unsigned int>(this->frameTable.Count() - 1));
        float *pos = tmpFrame.Data().As<float>();
        unsigned int cnt = static_cast<unsigned int>(tmpFrame.Size() / (sizeof(float) * 3));
        for (unsigned int i = 0; i < cnt; i++, pos += 3) {
            this->bbox.GrowToPoint(pos[0], pos[1], pos[2]);
        }
        this->loadFrame(&tmpFrame, static_cast<unsigned int>(this->frameTable.Count() / 2));
        pos = tmpFrame.Data().As<float>();
        cnt = static_cast<unsigned int>(tmpFrame.Size() / (sizeof(float) * 3));
        for (unsigned int i = 0; i < cnt; i++, pos += 3) {
            this->bbox.GrowToPoint(pos[0], pos[1], pos[2]);
        }
    }

    this->initFrameCache(cacheSize);

    return true; // to reset the dirty flag of the param slot
}


/*
 * moldyn::VisIttDataSource::filterChanged
 */
bool moldyn::VisIttDataSource::filterChanged(param::ParamSlot& slot) {
    // Power Dovel: On!
    unsigned int cs = this->CacheSize();
    unsigned int fc = this->FrameCount();
    this->resetFrameCache();
    this->setFrameCount(fc);

    this->filterIndex = UINT_MAX;
    vislib::StringA filtCol(this->filterColumn.Param<param::StringParam>()->Value());
    filtCol.TrimSpaces();
    for (SIZE_T i = 0; i < this->header.Count(); i++) {
        vislib::Pair<vislib::StringA, unsigned int>& hi = this->header[i];
        if (hi.First().Equals(filtCol)) {
            this->filterIndex = static_cast<unsigned int>(i);
            break;
        }
    }
    if (this->filterIndex == UINT_MAX) {
        try {
            int idx = vislib::CharTraitsA::ParseInt(filtCol);
            if ((idx >= 0) && (idx < this->header.Count())) {
                this->filterIndex = idx;
            }
        } catch(...) {
        }
    }

    this->initFrameCache(cs);
    return true;
}


/*
 * moldyn::VisIttDataSource::parseHeader
 */
bool moldyn::VisIttDataSource::parseHeader(const vislib::StringA& header) {
    ASSERT(this->header.Count() == 0);
    ASSERT(this->headerIdx.Count() == 0);
    unsigned int len = header.Length();
    if (len == 0) return false;
    for (unsigned int p = 0; p < len;) {
        unsigned int start = p;
        while ((p < len) && vislib::CharTraitsA::IsSpace(header[p])) p++;
        while ((p < len) && !vislib::CharTraitsA::IsSpace(header[p])) p++;
        if ((p - start) > 0) {
            vislib::StringA label = header.Substring(start, p - start);
            label.TrimSpaces();
            if (label.Length() > 0) {
                this->header.Add(vislib::Pair<vislib::StringA, unsigned int>(label, (p - start)));
            }
        }
    }
    if (this->header.Count() == 0) return false;

    for (SIZE_T i = 0; i < this->header.Count(); i++) {
        if (this->header[i].First().Equals("x", false)) {
            this->headerIdx.Add(static_cast<unsigned int>(i));
        }
    }
    for (SIZE_T i = 0; i < this->header.Count(); i++) {
        if (this->header[i].First().Equals("y", false)) {
            this->headerIdx.Add(static_cast<unsigned int>(i));
        }
    }
    for (SIZE_T i = 0; i < this->header.Count(); i++) {
        if (this->header[i].First().Equals("z", false)) {
            this->headerIdx.Add(static_cast<unsigned int>(i));
        }
    }
    if (this->headerIdx.Count() == 0) return false;

    while (this->headerIdx.Count() < 3) { // stupid, but makes things easier for now
        this->headerIdx.Add(this->headerIdx[0]);
    }

    return true;
}


/*
 * moldyn::VisIttDataSource::getDataCallback
 */
bool moldyn::VisIttDataSource::getDataCallback(Call& caller) {
    MultiParticleDataCall *c2 = dynamic_cast<MultiParticleDataCall*>(&caller);

    Frame *f = NULL;
    if (c2 != NULL) {
        f = dynamic_cast<Frame *>(this->requestLockedFrame(c2->FrameID()));
        if (f == NULL) return false;

        c2->SetDataHash((this->file == NULL) ? 0 : this->dataHash);
        c2->SetUnlocker(new Unlocker(*f));
        c2->SetFrameID(f->FrameNumber());
        c2->SetParticleListCount(1);
        c2->AccessParticles(0).SetGlobalRadius(this->radius.Param<param::FloatParam>()->Value());
        c2->AccessParticles(0).SetGlobalColour(192, 192, 192);
        c2->AccessParticles(0).SetCount(f->Size() / (sizeof(float) * 3));
        c2->AccessParticles(0).SetVertexData(MultiParticleDataCall::Particles::VERTDATA_FLOAT_XYZ, f->Data());

        return true;
    }

    return false;
}


/*
 * moldyn::VisIttDataSource::getExtentCallback
 */
bool moldyn::VisIttDataSource::getExtentCallback(Call& caller) {
    MultiParticleDataCall *c2 = dynamic_cast<MultiParticleDataCall*>(&caller);

    if (c2 != NULL) {
        float border = this->radius.Param<param::FloatParam>()->Value();

        c2->SetDataHash((this->file == NULL) ? 0 : this->dataHash);
        c2->SetFrameCount(static_cast<unsigned int>(this->frameTable.Count()));
        c2->AccessBoundingBoxes().Clear();
        c2->AccessBoundingBoxes().SetObjectSpaceBBox(this->bbox);
        c2->AccessBoundingBoxes().SetObjectSpaceClipBox(
            this->bbox.Left() - border, this->bbox.Bottom() - border, this->bbox.Back() - border, 
            this->bbox.Right() + border, this->bbox.Top() + border, this->bbox.Front() + border);

        return true;
    }

    return false;
}
