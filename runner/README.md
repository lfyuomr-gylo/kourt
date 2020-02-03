## How to build

### Prerequisites

* CMake 3.15 or newer
* clang 9.0 or newer (should be set as default C++ compiler via ``CXX`` environment variable)

### Build runner

To build runner just follow standard CMake project build procedure:
```bash
mkdir -p build
cd build
cmake .. # this step takes some time to download third-party dependencies
make
```

After these steps you'll get a ``runner`` executable.

### Run tests

In order to run tests execute
```bash
make test
```

**Note**: ``test`` target does not depend on ``all`` target, so you have to manually rebuild 
runner before you run tests each time you have changed runner implementation.