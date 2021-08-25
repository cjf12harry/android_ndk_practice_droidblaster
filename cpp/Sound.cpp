//
// Created by cjf12 on 2019-10-22.
//

#include "include/Sound.h"
#include "include/Log.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

Sound::Sound(android_app *pApplication, Resource *pResource) :
        mResource(pResource),
        mBuffer(NULL),
        mLength(0) {}

Sound::Sound(android_app *pApplication, int32_t pLength) :
        mResource(NULL),
        mBuffer(NULL),
        mLength(pLength) {}


const char *Sound::getPath() {
    return mResource->getPath();
}

status Sound::load() {
    Log::info("Loading sound %s", mResource->getPath());
    status result;

    //Opens sound file.
    if (mResource == NULL) {
        Log::info("Allocating sound");
        mBuffer = new uint8_t[mLength];
    } else {
        Log::info("Loading sound %s", mResource->getPath());
        if (mResource->open() != STATUS_OK) {
            goto ERROR;
        }
        //Reads sound file
        mLength = mResource->getLength();
        mBuffer = new uint8_t[mLength];
        result = mResource->read(mBuffer, mLength);
        mResource->close();
    }
    return STATUS_OK;

    ERROR:
    Log::error("Error while reading PCM sound");
    return STATUS_KO;
}

status Sound::unload() {
    delete[] mBuffer;
    mBuffer = NULL;
    mLength = 0;
    return STATUS_KO;
}
