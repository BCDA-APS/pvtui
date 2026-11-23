# PVTUI

[![Documentation](https://img.shields.io/badge/docs-sphinx-blue)](https://bcda-aps.github.io/pvtui/)

PVTUI is a library for creating terminal/text based user interfaces for EPICS built on [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

![pvtui_motor](https://github.com/user-attachments/assets/9351f025-f1f6-4d99-acf6-e0e78a7e4adc)

## Pre-built binaries

Static binaries for linux-x86_64 (glibc 2.34+, e.g. RHEL 9) are attached to each
[release](https://github.com/BCDA-APS/pvtui/releases):

```bash
curl -LO https://github.com/BCDA-APS/pvtui/releases/download/v0.4.0/pvtui-v0.4.0-linux-x86_64.tar.gz
tar -xzf pvtui-v0.4.0-linux-x86_64.tar.gz
```

The applications are in `pvtui-v0.4.0-linux-x86_64/bin/`.

## Building from source

Dependencies:

- C++17 compiler (e.g. GCC 8+)
- [CMake](https://cmake.org) (3.22+)
- [EPICS Base](https://epics-controls.org/resources-and-support/base/)
- [PVXS](https://github.com/epics-base/pvxs)
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) (fetched automatically by default)

Set `EPICS_BASE`, `PVXS` and `EPICS_HOST_ARCH` in the environment or pass them with `-D`, then:

```bash
git clone https://github.com/BCDA-APS/pvtui
cd pvtui
cmake --preset release
cmake --build build/release
# optionally install
# cmake --install build/release --prefix /path/to/prefix
```

See [building](https://bcda-aps.github.io/pvtui/building.html) for all build options.

See the [tutorial](https://bcda-aps.github.io/pvtui/tutorial.html) for creating your own applications.

## Documentation

Full documentation is available at [https://bcda-aps.github.io/pvtui/](https://bcda-aps.github.io/pvtui/)

## License

See [LICENSE](LICENSE) for details.
