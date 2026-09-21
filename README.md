# resourcepackage

This library allows to pack resources into resource packages and use them. Furthermore, the resource package can be used as a separate file or built in to the executable file.

The resource package is sorted key-value storage with keys up to 255 bytes and values up to 2**32-1 bytes.

# Prerequisites
* [`libunix++`](https://github.com/saur0n/libunixpp)

# Usage
```
./mkresourcepackage OPTION1 OPTION2 .. OPTIONn
```

Where `OPTIONi` can be:
* `FILE`: Add a file or recursively add a directory
* `$key=value`: Add a text resource 
* `@key=escaped`: Add an escaped text resource 
* `:key=file`: Add a file with an explicit resource key
* `base=DIRECTORY`: Set base directory for resource keys
* `prefix=STRING`: Set the prefix for all resource keys
* `output=FILE`: Set output package filename
* `allowoverwrite=yes`: Allow duplicate keys (last value wins)
* `allowoverwrite=no`: Reject duplicate keys

Examples:
```
respack image.png
respack :logo=images/logo.png
respack $version=1.0
respack '@message=Hello\nworld'
respack base=resources resources/ output=app.respack
```

# Using a respack as an object file

A `.respack` file can be embedded into a program as a binary object using `objcopy`:

```
objcopy -I binary -O elf64-x86-64 -B i386:x86-64 app.respack app.respack.o
```

The generated object contains three symbols describing the embedded data:

```
nm app.respack.o
```

Typically, the output looks like:

```
0000000000000000 D _binary_app_respack_start
0000000000001234 D _binary_app_respack_end
0000000000001234 A _binary_app_respack_size
```

These symbols can be referenced from C/C++ to access the embedded resource package.

## Example usage

This example implies usage of `app.respack.o` from the previous chapter.

```
#include <ResourcePackage.hpp>
#include <iostream>
#include <string>

using std::cout;
using std::string;

extern "C" {
    extern const unsigned char _binary_app_respack_start[];
    extern const unsigned char _binary_app_respack_end[];
}

int main(int argc, char **argv) {
    if (argc < 2)
        return 1;

    ResourcePackage package(
        _binary_app_respack_start,
        _binary_app_respack_end
    );

    string key = argv[1];

    try {
        cout << "resource " << key << " is `" << package[key].toString() << "`\n";
    }
    catch (const ResourcePackage::NotFound &) {
        cout << "resource with key " << key << " was not found\n";
    }
    
    return 0;
}
```
