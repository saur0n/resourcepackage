/*******************************************************************************
 *  Resource management library
 *  Resource package
 ******************************************************************************/

#include <cstring>
#include "ResourcePackage.hpp"

using std::string;
using std::vector;
using upp::File;
using upp::MemoryMapping;

static inline int compare(const char * a, size_t al, const char * b, size_t bl) {
    size_t common=al<bl?al:bl;
    int retval=memcmp(a, b, common);
    if (retval==0) {
        if (al<bl)
            return -1;
        if (al>bl)
            return 1;
        return 0;
    }
    else
        return retval;
}

static inline size_t lof(File &file) {
    size_t result=file.seek(0, SEEK_END);
    file.seek(0);
    return result;
}

static inline const unsigned char * ucptr(const void * ptr) {
    return reinterpret_cast<const unsigned char *>(ptr);
}

/******************************************************************************/

Resource::Resource(const void * start, size_t length) :
        start(start), length(length) {}

Resource::Resource(const void * start, const void * end) :
        start(start), length(ucptr(end)-ucptr(start)) {}

const void * Resource::getEnd() const {
    return ucptr(start)+length;
}

string Resource::toString() const {
    return string(reinterpret_cast<const char *>(start), length);
}

/******************************************************************************/

ResourcePackage::ResourcePackage(const void * start, size_t length) :
        Resource(start, length) {}

ResourcePackage::ResourcePackage(const void * start, const void * end) :
        Resource(start, end) {}

vector<string> ResourcePackage::list() const {
    vector<string> result;
    const unsigned * header=reinterpret_cast<const unsigned *>(getStart());
    const unsigned count=*header++;
    const char * data=reinterpret_cast<const char *>(header+count);
    
    for (unsigned i=0; i<count; i++) {
        const char * entry=data+header[i];
        unsigned char keyLength=*entry++;
        result.emplace_back(entry, keyLength);
    }
    
    return result;
}

Resource ResourcePackage::get(const string &key) const {
    const unsigned * header=reinterpret_cast<const unsigned *>(getStart());
    const unsigned count=*header++;
    const char * data=reinterpret_cast<const char *>(header+count);
    
    if (count) {
        int bottom=0;
        int top=count-1;
        while (bottom<=top) {
            int mid=(bottom+top)/2;
            const char * entry=data+header[mid];
            unsigned char keyLength=*entry++;
            int cmp=compare(key.c_str(), key.length(), entry, keyLength);
            
            if (cmp==0) {
                const char * content=entry+keyLength;
                const char * end=reinterpret_cast<const char *>(getEnd());
                const char * nextEntry=mid<count-1?data+header[mid+1]:end;
                return Resource(content, nextEntry-content);
            }
            else if (cmp<0)
                top=mid-1;
            else
                bottom=mid+1;
        }
    }
    
    throw NotFound(key);
}

/******************************************************************************/

ResourceFile::ResourceFile(const char * filename) :
        file(filename),
        memoryMapping(file, lof(file), PROT_READ) {}

Resource ResourceFile::asResource() const {
    return Resource(memoryMapping.getAddress(), memoryMapping.getLength());
}

ResourcePackage ResourceFile::asResourcePackage() const {
    return ResourcePackage(memoryMapping.getAddress(), memoryMapping.getLength());
}
