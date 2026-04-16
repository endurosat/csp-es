# How to install LibCSP

```{contents}
:depth: 3
```

CSP supports three build systems:

- [the Meson build system](https://mesonbuild.com/)
- [the Waf build system](https://waf.io/)
- [the CMake build system](https://cmake.org/)

## Using Meson

In order to compile CSP with `meson`, you
run the following commands:

```shell
meson setup builddir
cd builddir
ninja
```

You can use `meson configure` to change the
core options as well as compiler or project options.

## Using Waf

In order to compile CSP with `waf`, you
first need to configure the toolchain, what operating system to compile
for, the location of required libraries and whether to enable certain
optional features.

To configure CSP to build with the AVR32 toolchain for FreeRTOS and
output the compiled libcsp.a and header files to the install directory,
issue:

```shell
./waf configure --toolchain=avr32- --with-os=freertos --prefix=install
```

When compiling for FreeRTOS, the path to the FreeRTOS header files must
be specified with `--includes=PATH`.

A number of optional features can be enabled by from the configure
script.
`./waf configure --help` to list the
available configure options.

The CAN driver (based on socketcan) can be enabled by appending the
configure option `--enable-can-socketcan`.

To build and copy the library to the location specified with --prefix,
use:

```shell
./waf build install
```

## Using CMake

Make sure [Ninja](https://ninja-build.org/) is installed on your system first.
You can now run the following commands to compile CSP with `cmake`:

```shell
cmake -G Ninja -B builddir
cmake --build builddir
```

Please note that other build system generators might work as well, but `Ninja` is the officially
supported and tested build system when using CMake.
To install the compiled libcsp.so and header files to the install directory,
you run the following command:

```shell
cmake --install builddir
```

Please note that `sudo` might be required to install files into the default install directories.
By default, it will be installed in `/usr/local/lib` and `/usr/local/include`,
but if you wish to change it, you can specify `-DCMAKE_INSTALL_PREFIX=<path>`
during the build process, and it will be installed in `<path>/lib` and `<path>/include`.

To install only the libcsp.so runtime library,
use the following command:

```shell
cmake --install builddir --component runtime
```

### Building All Samples with CMake

if you want to build tools and samples, define `CSP_BUILD_SAMPLES=ON`
when you run `cmake`.

```shell
cmake -B builddir -DCSP_BUILD_SAMPLES=ON
```

### Python Bindings with CMake

If you want to build Python bindings, define
`CSP_ENABLE_PYTHON3_BINDINGS=ON` when you run `cmake`. You also need
to enable the routing table (`CSP_USE_RTABLE`) when building the
Python bindings.

```shell
cmake -B builddir -DCSP_ENABLE_PYTHON3_BINDINGS=ON -DCSP_USE_RTABLE=ON
```

To use the bindings, you need to install them to a location where
Python searches by default or specify the path to Python:

```
PYTHONPATH=builddir python3 -c 'import libcsp_py3 as csp'
```


### Version Reporting with CMake

The library exposes a runtime version API (`csp_get_version()`) and compile-time
preprocessor constants (`CSP_VERSION_MAJOR`, `CSP_VERSION_MINOR`,
`CSP_VERSION_PATCH`) declared in `include/csp/csp_version.h`.  These values are
populated by the build system — they are not hard-coded in the source — so your
project is responsible for injecting them.

#### How the injection works

After `add_subdirectory(csp_es)` (or after building the library standalone), call
`target_compile_definitions` on the `csp_es` target to set the following symbols:

| Symbol | Scope | Purpose |
|---|---|---|
| `CSP_VERSION_MAJOR` | PUBLIC | Major version integer — propagated to all consumers of `csp_es` |
| `CSP_VERSION_MINOR` | PUBLIC | Minor version integer — propagated to all consumers of `csp_es` |
| `CSP_VERSION_PATCH` | PUBLIC | Patch version integer — propagated to all consumers of `csp_es` |
| `SW_VERSION_MAJOR` | PRIVATE | Same value, used internally by `csp_version.c` |
| `SW_VERSION_MINOR` | PRIVATE | Same value, used internally by `csp_version.c` |
| `SW_VERSION_PATCH` | PRIVATE | Same value, used internally by `csp_version.c` |
| `SW_VERSION_LABEL` | PRIVATE | Pre-release or build metadata label (e.g. `rc.1`), empty string for releases |
| `SW_VERSION_STRING` | PRIVATE | Full version string passed to `csp_get_version()->version_string` |
| `SW_BUILD_TIMESTAMP` | PRIVATE | Unix timestamp of the build (seconds since epoch) |

PUBLIC definitions are automatically inherited by any target that links against
`csp_es`, so consumers can use `CSP_VERSION_MAJOR` in `#if` preprocessor guards
without any extra setup.

If the definitions are not injected, all fields fall back to `0` and
`csp_get_version()->version_string` returns `"0.0.0-unknown"`.

#### Minimal example — version from a CMake variable

```cmake
set(MY_VERSION_MAJOR 1)
set(MY_VERSION_MINOR 2)
set(MY_VERSION_PATCH 3)
set(MY_VERSION_LABEL "")          # empty for a clean release
set(MY_VERSION_STRING "1.2.3")

add_subdirectory(csp_es)

target_compile_definitions(csp_es
    PUBLIC
        "CSP_VERSION_MAJOR=${MY_VERSION_MAJOR}"
        "CSP_VERSION_MINOR=${MY_VERSION_MINOR}"
        "CSP_VERSION_PATCH=${MY_VERSION_PATCH}"
    PRIVATE
        "SW_VERSION_MAJOR=${MY_VERSION_MAJOR}"
        "SW_VERSION_MINOR=${MY_VERSION_MINOR}"
        "SW_VERSION_PATCH=${MY_VERSION_PATCH}"
        "SW_VERSION_LABEL=${MY_VERSION_LABEL}"
        "SW_VERSION_STRING=\"${MY_VERSION_STRING}\""
        "SW_BUILD_TIMESTAMP=0"
)
```

#### CI pipeline example — version from git tags

A common pattern is to derive the version from `git describe --tags` in CI and
pass it to CMake via `-D` flags.  The shell fragment below works with any CI
system (GitHub Actions, GitLab CI, Jenkins, etc.):

```bash
# In your CI pipeline, before the cmake configure step:

# git describe produces strings like "1.2.3" (on a tag) or "1.2.3-4-gabcdef0"
# (between tags).  Split on "-" to separate the numeric part from the label.
GIT_DESC=$(git describe --tags --always 2>/dev/null || echo "0.0.0-unknown")

# Extract MAJOR.MINOR.PATCH — everything up to (but not including) the first "-"
VERSION_CORE="${GIT_DESC%%-*}"         # e.g. "1.2.3"
VERSION_LABEL="${GIT_DESC#${VERSION_CORE}}"   # e.g. "-4-gabcdef0" or ""
VERSION_LABEL="${VERSION_LABEL#-}"            # strip the leading dash → "4-gabcdef0"

IFS='.' read -r MAJOR MINOR PATCH <<< "${VERSION_CORE}"

BUILD_TS=$(date +%s)

cmake -B build -S . \
    -DVERSION_MAJOR="${MAJOR}" \
    -DVERSION_MINOR="${MINOR}" \
    -DVERSION_PATCH="${PATCH}" \
    -DVERSION_LABEL="${VERSION_LABEL}" \
    -DVERSION_STRING="${GIT_DESC}" \
    -DBUILD_TIMESTAMP="${BUILD_TS}"
```

Then in your `CMakeLists.txt`, consume those cache variables:

```cmake
add_subdirectory(csp_es)

target_compile_definitions(csp_es
    PUBLIC
        "CSP_VERSION_MAJOR=${VERSION_MAJOR}"
        "CSP_VERSION_MINOR=${VERSION_MINOR}"
        "CSP_VERSION_PATCH=${VERSION_PATCH}"
    PRIVATE
        "SW_VERSION_MAJOR=${VERSION_MAJOR}"
        "SW_VERSION_MINOR=${VERSION_MINOR}"
        "SW_VERSION_PATCH=${VERSION_PATCH}"
        "SW_VERSION_LABEL=${VERSION_LABEL}"
        "SW_VERSION_STRING=\"${VERSION_STRING}\""
        "SW_BUILD_TIMESTAMP=${BUILD_TIMESTAMP}"
)
```

#### Offline and shallow-clone builds

If your build environment has no network access or uses a shallow clone
(`--depth 1`), `git describe` may fail or return only a commit hash.  The
recommended mitigation is to write the resolved version string to a plain-text
`version` file (one line, e.g. `1.2.3-rc.1`) at tag time and commit it to your
repository.  Your CMake logic can then read that file as a fallback:

```cmake
if(NOT DEFINED VERSION_STRING OR VERSION_STRING STREQUAL "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/version")
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version" VERSION_STRING)
        string(STRIP "${VERSION_STRING}" VERSION_STRING)
    else()
        set(VERSION_STRING "0.0.0-unknown")
    endif()
endif()
```

This ensures that builds from release archives or pre-populated CI caches always
report a meaningful version without requiring git at build time.

## Reproducible Builds

libcsp supports Reproducible Builds. To enable it, set
`CSP_REPRODUCIBLE_BUILDS` to `1`.

Please note that, when reproducible builds are enabled,
`CSP_CMP_IDENT` does not return the compilation date and time.

When reproducible builds are enabled, both the BuildID and hash values
of generated binaries remain consistent for each build. Our
reproducible builds also support building in different directories.
Thus, different users should generate precisely the same binaries,
given the same source code and build environment.

You can learn more about reproducible builds at
https://reproducible-builds.org/.

Use the following commands for each build system:

### Waf

```shell
./waf configure --enable-reproducible-builds
```

### Meson

```shell
meson setup builddir . -Denable_reproducible_builds=true
```

### CMake

```shell
cmake -G Ninja -B builddir -DCSP_REPRODUCIBLE_BUILDS=ON
```

Note: By default, CMake embeds the build directory in the binaries,
resulting in non-deterministic builds. To address this, use
`CMAKE_BUILD_RPATH_USE_ORIGIN=ON` or `CMAKE_SKIP_RPATH=ON` as follows:

```shell
cmake -G Ninja -B builddir -DCSP_REPRODUCIBLE_BUILDS=ON -DCMAKE_BUILD_RPATH_USE_ORIGIN=ON
```

See [Reproducible Builds site][1] or [CMake document][2] for more details.

[1]: https://reproducible-builds.org/docs/deterministic-build-systems/
[2]: https://cmake.org/cmake/help/latest/prop_tgt/BUILD_RPATH.html
