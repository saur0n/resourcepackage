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
* `prefix=PREFIX` to set prefix used in keys

## In programs
```
#include <ResourcePackage.hpp>

...

ResourcePackage package;
```
