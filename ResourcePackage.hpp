/*******************************************************************************
 *  Resource management library
 *  Resource package
 *  
 *  © 2026, Sauron
 ******************************************************************************/

#ifndef __LIB_RESOURCEPACKAGE_HPP
#define __LIB_RESOURCEPACKAGE_HPP

#include <string>
#include <unix++/File.hpp>
#include <unix++/MemoryMapping.hpp>
#include <vector>

/** Represents a read-only binary resource stored in memory **/
class Resource {
public:
    /**/
    Resource();
    /** Initialize a resource entry **/
    Resource(const void * start, size_t length);
    /** Initialize a resource entry **/
    Resource(const void * start, const void * end);
    /** Returns the pointer to the start **/
    const void * getStart() const { return start; }
    /** Returns the pointer beyond the end **/
    const void * getEnd() const;
    /** Returns the length **/
    size_t getLength() const { return length; }
    /** Get the resource as string **/
    std::string toString() const;
    
private:
    const void * start;
    size_t length;
};

/** An ordered map of resources **/
class ResourcePackage : public Resource {
public:
    /** The resource with given key was not found **/
    class NotFound {
    public:
        NotFound(const std::string &key) : key(key) {}
        const std::string &getKey() const { return key; }
        
    private:
        std::string key;
    };
    /** Initialize a resource package **/
    ResourcePackage(const void * start, size_t length);
    /** Initialize a resource package **/
    ResourcePackage(const void * start, const void * end);
    /** List all keys **/
    std::vector<std::string> list() const;
    /** Find a resource by name **/
    Resource get(const std::string &key) const;
    /** Find a resource by name **/
    Resource operator [](const std::string &key) const { return get(key); }
};

/** Memory-mapped resource file **/
class ResourceFile {
public:
    /** Open a file as a resource package **/
    explicit ResourceFile(const char * filename);
    /** Get the file as a resource **/
    Resource asResource() const;
    /** Get the file as a resource package **/
    ResourcePackage asResourcePackage() const;
    
private:
    upp::File file;
    upp::MemoryMapping memoryMapping;
};

#define RESOURCE(path) \
    extern "C" { \
        extern const unsigned char _binary_##path##_start[]; \
        extern const unsigned char _binary_##path##_end[]; \
    }

#endif
