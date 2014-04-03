/*
 * RawStorageSerialiser.cpp
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS). 
 * Alle Rechte vorbehalten.
 */

#include "vislib/RawStorageSerialiser.h"

#include "the/assert.h"
#include "the/invalid_operation_exception.h"


/*
 * vislib::RawStorageSerialiser::RawStorageSerialiser
 */
vislib::RawStorageSerialiser::RawStorageSerialiser(RawStorage *storage, 
        unsigned int offset) 
        : Serialiser(SERIALISER_REQUIRES_ORDER), nakedStorage(NULL), 
        nakedStorageSize(0), storage(storage), offset(offset) {
    // Intentionally empty
}


/*
 * vislib::RawStorageSerialiser::RawStorageSerialiser
 */
vislib::RawStorageSerialiser::RawStorageSerialiser(const uint8_t *storage, 
        const size_t storageSize, const unsigned int offset) 
        : Serialiser(SERIALISER_REQUIRES_ORDER), nakedStorage(storage),
        nakedStorageSize(storageSize), storage(NULL), offset(offset) {
    // Intentionally empty
    THE_ASSERT(storage != NULL);
}


/*
 * vislib::RawStorageSerialiser::RawStorageSerialiser
 */
vislib::RawStorageSerialiser::RawStorageSerialiser(
        const RawStorageSerialiser& src) 
        : Serialiser(src), nakedStorage(NULL), nakedStorageSize(0), 
        storage(NULL), offset(0) {
    *this = src;
}


/*
 * vislib::RawStorageSerialiser::~RawStorageSerialiser
 */
vislib::RawStorageSerialiser::~RawStorageSerialiser(void) {
    this->nakedStorage = NULL;  // DO NOT DELETE!
    this->nakedStorageSize = 0;
    this->storage = NULL;       // DO NOT DELETE!
    this->offset = 0;
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(bool& outValue, 
        const char *name) {
    unsigned char c;
    this->restore(&c, 1);
    outValue = (c != 0);
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(bool& outValue, 
        const wchar_t *name) {
    unsigned char c;
    this->restore(&c, 1);
    outValue = (c != 0);
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(wchar_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(wchar_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int8_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(int8_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int8_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(int8_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint8_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(uint8_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint8_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(uint8_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int16_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(int16_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int16_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(int16_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint16_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(uint16_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint16_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(uint16_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int32_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(int32_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int32_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(int32_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint32_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(uint32_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint32_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(uint32_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int64_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(int64_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(int64_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(int64_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint64_t& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(uint64_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(uint64_t& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(uint64_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(float& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(float));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(float& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(float));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(double& outValue, 
        const char *name) {
    this->restore(&outValue, sizeof(double));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(double& outValue, 
        const wchar_t *name) {
    this->restore(&outValue, sizeof(double));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(the::astring& outValue,
        const char *name) {
    unsigned int len;
    this->restore(&len, sizeof(unsigned int));
    outValue = the::astring(len, ' ');
    this->restore(const_cast<char*>(outValue.c_str()), len * sizeof(char));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(the::astring& outValue,
        const wchar_t *name) {
    unsigned int len;
    this->restore(&len, sizeof(unsigned int));
    outValue = the::astring(len, ' ');
    this->restore(const_cast<char*>(outValue.c_str()), len * sizeof(char));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(the::wstring& outValue,
        const char *name) {
    unsigned int len;
    this->restore(&len, sizeof(unsigned int));
    outValue = the::wstring(len, ' ');
    this->restore(const_cast<wchar_t*>(outValue.c_str()), len * sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::Deserialise
 */
void vislib::RawStorageSerialiser::Deserialise(the::wstring& outValue,
        const wchar_t *name) {
    unsigned int len;
    this->restore(&len, sizeof(unsigned int));
    outValue = the::wstring(len, ' ');
    this->restore(const_cast<wchar_t*>(outValue.c_str()), len * sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const bool value, 
        const char *name) {
    unsigned char c = value ? 1 : 0;
    this->store(&c, 1);
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const bool value, 
        const wchar_t *name) {
    unsigned char c = value ? 1 : 0;
    this->store(&c, 1);
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const wchar_t value, 
        const char *name) {
    this->store(&value, sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const wchar_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int8_t value, 
        const char *name) {
    this->store(&value, sizeof(int8_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int8_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(int8_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint8_t value, 
        const char *name) {
    this->store(&value, sizeof(uint8_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint8_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(uint8_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int16_t value, 
        const char *name) {
    this->store(&value, sizeof(int16_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int16_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(int16_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint16_t value, 
        const char *name) {
    this->store(&value, sizeof(uint16_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint16_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(uint16_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int32_t value, 
        const char *name) {
    this->store(&value, sizeof(int32_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int32_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(int32_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint32_t value, 
        const char *name) {
    this->store(&value, sizeof(uint32_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint32_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(uint32_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int64_t value, 
        const char *name) {
    this->store(&value, sizeof(int64_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const int64_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(int64_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint64_t value, 
        const char *name) {
    this->store(&value, sizeof(uint64_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const uint64_t value, 
        const wchar_t *name) {
    this->store(&value, sizeof(uint64_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const float value, 
        const char *name) {
    this->store(&value, sizeof(float));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const float value, 
        const wchar_t *name) {
    this->store(&value, sizeof(float));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const double value, 
        const char *name) {
    this->store(&value, sizeof(double));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const double value, 
        const wchar_t *name) {
    this->store(&value, sizeof(double));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const the::astring& value, 
        const char *name) {
    unsigned int len = static_cast<unsigned int>(value.size());
    this->store(&len, sizeof(unsigned int));
    this->store(value.c_str(), len * sizeof(char));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const the::astring& value, 
        const wchar_t *name) {
    unsigned int len = static_cast<unsigned int>(value.size());
    this->store(&len, sizeof(unsigned int));
    this->store(value.c_str(), len * sizeof(char));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const the::wstring& value, 
        const char *name) {
    unsigned int len = static_cast<unsigned int>(value.size());
    this->store(&len, sizeof(unsigned int));
    this->store(value.c_str(), len * sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::Serialise
 */
void vislib::RawStorageSerialiser::Serialise(const the::wstring& value, 
        const wchar_t *name) {
    unsigned int len = static_cast<unsigned int>(value.size());
    this->store(&len, sizeof(unsigned int));
    this->store(value.c_str(), len * sizeof(wchar_t));
}


/*
 * vislib::RawStorageSerialiser::SetOffset
 */
void vislib::RawStorageSerialiser::SetOffset(unsigned int offset) {
    this->offset = offset;
}


/*
 * vislib::RawStorageSerialiser::SetStorage
 */
void vislib::RawStorageSerialiser::SetStorage(vislib::RawStorage *storage) {
    this->storage = storage;
}


/*
 * vislib::RawStorageSerialiser::operator=
 */
vislib::RawStorageSerialiser& vislib::RawStorageSerialiser::operator =(
        const vislib::RawStorageSerialiser& rhs) {
    this->nakedStorage = rhs.nakedStorage;
    this->nakedStorageSize = rhs.nakedStorageSize;
    this->storage = rhs.storage;
    this->offset = rhs.offset;
    return *this;
}


/*
 * vislib::RawStorageSerialiser::store
 */
void vislib::RawStorageSerialiser::store(const void *data, unsigned int size) {
    if (this->storage == NULL) {
        throw the::invalid_operation_exception("No RawStorage object set",
            __FILE__, __LINE__);
    }
    this->storage->AssertSize(this->offset + size, true);
    memcpy(this->storage->As<char>() + this->offset, data, size);
    this->offset += size;
}


/*
 * vislib::RawStorageSerialiser::restore
 */
void vislib::RawStorageSerialiser::restore(void *data, unsigned int size) {
    /* Consolidate pointers. */
    if (this->storage != NULL) {
        this->nakedStorage = this->storage->As<uint8_t>();
        this->nakedStorageSize = this->storage->GetSize();
    }

    /* Sanity checks. */
    if (this->nakedStorage == NULL) {
        throw the::invalid_operation_exception("Either a RawStorage object or a "
            "naked data pointer must be provided for deserialisation.",
            __FILE__, __LINE__);
    }
    if (this->nakedStorageSize < this->offset + size) {
        throw the::exception("Not enough data in storage object to "
            "deserialise", __FILE__, __LINE__);
    }

    ::memcpy(data, this->nakedStorage + this->offset, size);
    this->offset += size;
}
