/*******************************************************************************
 *  Resource management library
 *  Resource package creation tool
 *  
 *  © 2026, Sauron
 ******************************************************************************/

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <unix++/Directory.hpp>
#include <unix++/File.hpp>
#include <unix++/FileSystem.hpp>
#include <vector>

using std::cerr;
using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;
using upp::Directory;
using upp::File;

using ByteArray=vector<uint8_t>;

static const char * DEFAULT_OUTPUT_FILENAME="output.respack";

extern string unescape(const string &input);

static void putInteger(ByteArray &store, uint32_t value) {
    store.push_back(value&0xFF);
    store.push_back((value>>8)&0xFF);
    store.push_back((value>>16)&0xFF);
    store.push_back((value>>24)&0xFF);
}

static void putData(ByteArray &store, const string &value) {
    store.insert(store.end(), value.begin(), value.end());
}

static void putData(ByteArray &store, const ByteArray &value) {
    store.insert(store.end(), value.begin(), value.end());
}

static void printException(const std::exception& e, int level=0) {
    std::cerr << std::string(level, '\t') << "error: " << e.what() << '\n';
    try
    {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nestedException)
    {
        printException(nestedException, level+1);
    }
    catch (...) {}
}

static string getRealPath(const string &path) {
    char * resolved=::realpath(path.c_str(), nullptr);
    if (!resolved)
        throw std::system_error(errno, std::generic_category());
    
    string result(resolved);
    free(resolved);
    return result;
}

static std::optional<std::string_view> subtract(std::string_view parent, std::string_view path) {
    while ((parent.size()>1)&&(parent.back()=='/'))
        parent.remove_suffix(1);

    if (path.size()<parent.size())
        return std::nullopt;
    else if (path.compare(0, parent.size(), parent)!=0)
        return std::nullopt;
    else if (path.size()==parent.size())
        return std::string_view{};
    else if (path[parent.size()]!='/')
        return std::nullopt;
    else
        return path.substr(parent.size()+1);
}

class ResourceBuilder {
public:
    /** Set the directory to the current one **/
    ResourceBuilder();
    /** Set whether duplciate keys are allowed **/
    void setAllowKeyOverwriting(bool allowKeyOverwriting);
    /** Set the directory as a base **/
    void setBaseDirectory(const string &directory);
    /** Set prefix for keys **/
    void setPrefix(const string &prefix);
    /** Add resource with given content **/
    void add(const string &key, const ByteArray &content);
    /** Add resource with given content **/
    void add(const string &key, const string &content);
    /** Read resource from file and add it **/
    void addFile(const string &filename);
    /** Read resource from file and add it **/
    void addFile(const string &key, const string &filename);
    /** Write the package to file **/
    void build(const string &outputFilename) const;
    
private:
    bool allowKeyOverwriting;
    string base;
    string prefix;
    map<string, ByteArray> items;
};

ResourceBuilder::ResourceBuilder() : allowKeyOverwriting(false) {}

void ResourceBuilder::setAllowKeyOverwriting(bool allowKeyOverwriting) {
    this->allowKeyOverwriting=allowKeyOverwriting;
}

void ResourceBuilder::setBaseDirectory(const string &directory) {
    base=getRealPath(directory);
}

void ResourceBuilder::setPrefix(const string &prefix) {
    this->prefix=prefix;
}

void ResourceBuilder::add(const string &key, const ByteArray &data) {
    auto combinedKey=prefix+key;
    if (combinedKey.length()>255)
        throw std::runtime_error("key is too long");
    if (!items.try_emplace(combinedKey, data).second)
        if (!allowKeyOverwriting)
            throw std::runtime_error("duplicate key "+combinedKey);
}

void ResourceBuilder::add(const string &key, const string &content) {
    add(key, ByteArray(content.begin(), content.end()));
}

void ResourceBuilder::addFile(const string &filename) {
    if (!base.empty()) {
        auto realPath=getRealPath(filename);
        auto relPath=subtract(base, realPath);
        if (relPath) {
            string relPathStr(*relPath);
            addFile(relPathStr, realPath);
        }
        else
            throw std::runtime_error("'"+filename+"' should be inside '"+base+"'");
    }
    else if (filename==".")
        addFile("", ".");
    else
        addFile(filename, filename);
}

void ResourceBuilder::addFile(const string &key, const string &filename) {
    auto statbuf=upp::FileSystem::stat(filename.c_str());
    if (S_ISDIR(statbuf.st_mode)) {
        File file(filename.c_str(), O_RDONLY|O_DIRECTORY);
        Directory directory(file);
        while (Directory::Entry * entry=directory.read()) {
            string name=entry->d_name;
            if ((name!=".")&&(name!="..")) {
                if (key.empty())
                    addFile(name, filename+'/'+name);
                else
                    addFile(key+'/'+name, filename+'/'+name);
            }
        }
    }
    else if (S_ISREG(statbuf.st_mode)) {
        File file(filename.c_str());
        size_t length=file.seek(0, SEEK_END);
        file.seek(0);
        ByteArray content(length, '\0');
        file.read(&content[0], content.size());
        add(key, content);
    }
    else
        throw std::runtime_error(key+" is not a directory or a regular file");
}

void ResourceBuilder::build(const string &outputFilename) const {
    vector<unsigned char> headStore;
    vector<unsigned char> dataStore;
    
    headStore.reserve(sizeof(uint32_t)*items.size()+sizeof(uint32_t));
    putInteger(headStore, items.size());
    for (auto i=items.begin(); i!=items.end(); ++i) {
        auto &key=i->first;
        auto &value=i->second;
        
        if (dataStore.size()>UINT32_MAX)
            throw std::runtime_error("resource package is too large");
        putInteger(headStore, dataStore.size());
        
        dataStore.push_back((unsigned char)key.length());
        putData(dataStore, key);
        putData(dataStore, value);
    }
    
    File output(outputFilename.c_str(), O_WRONLY|O_CREAT|O_TRUNC);
    output.write(headStore.data(), headStore.size());
    output.write(dataStore.data(), dataStore.size());
}

int main(int argc, char ** argv) {
    try {
        ResourceBuilder builder;
        std::string outputFilename;
        
        for (int i=1; i<argc; i++) {
            string arg=argv[i];
            try {
                size_t eqPos=arg.find('=');
                if (eqPos==string::npos) {
                    builder.addFile(arg);
                }
                else {
                    string option=arg.substr(0, eqPos);
                    string operand=arg.substr(eqPos+1);
                    
                    if (option.empty())
                        builder.addFile(operand);
                    else if (option[0]=='$')
                        builder.add(option.substr(1), operand);
                    else if (option[0]=='@')
                        builder.add(option.substr(1), unescape(operand));
                    else if (option[0]==':')
                        builder.addFile(option.substr(1), operand);
                    else if (option=="allowoverwrite") {
                        if (operand=="yes")
                            builder.setAllowKeyOverwriting(true);
                        else if (operand=="no")
                            builder.setAllowKeyOverwriting(false);
                        else
                            throw std::runtime_error("operand should be 'yes' or 'no'");
                    }
                    else if (option=="base")
                        builder.setBaseDirectory(operand);
                    else if (option=="prefix")
                        builder.setPrefix(operand);
                    else if (option=="output")
                        outputFilename=operand;
                    else
                        throw std::runtime_error("unknown option "+option);
                }
            }
            catch (const std::exception &e) {
                std::throw_with_nested(std::runtime_error(arg));
            }
        }
        
        if (outputFilename.empty()) {
            cerr << "Warning: output filename is not specified, using " <<
                DEFAULT_OUTPUT_FILENAME << " as output filename!\n";
            outputFilename=DEFAULT_OUTPUT_FILENAME;
        }
        
        builder.build(outputFilename);
        
        return 0;
    }
    catch (const std::exception &e) {
        printException(e);
        return 1;
    }
    catch (...) {
        cerr << argv[0] << ": unknown error" << endl;
        return 1;
    }
}
