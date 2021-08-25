//
// Created by cjf12 on 2019-10-13.
//

#include "include/Resource.h"
#include <sys/stat.h>

Resource::Resource(android_app *pApplication, const char *pPath) :
        mPath(std::string("/sdcard/") + pPath),
        mInputStream() {
}

status Resource::open() {
    mInputStream.open(mPath.c_str(), std::ios::in | std::ios::binary);
    return mInputStream ? STATUS_OK : STATUS_KO;
}

void Resource::close() {
    mInputStream.close();
}

status Resource::read(void *pBuffer, size_t pCount) {
    mInputStream.read((char*)pBuffer, pCount);
    return (!mInputStream.fail())? STATUS_OK: STATUS_KO;
}

bool Resource::operator==(const Resource &pOther) {
    return mPath == pOther.mPath;
}

off_t Resource::getLength() {
    struct stat filestatus;
    if (stat(mPath.c_str(), &filestatus) >= 0) {
        return filestatus.st_size;
    } else{
        return -1;
    }
}
