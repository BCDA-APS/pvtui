# PVTUI

[![Documentation](https://img.shields.io/badge/docs-sphinx-blue)](https://bcda-aps.github.io/pvtui/)

PVTUI is a library for creating terminal/text based user interfaces for EPICS built on [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
and [pvAccessCPP](https://github.com/epics-base/pvAccessCPP).

![pvtui_motor](https://github.com/user-attachments/assets/346f036d-8ae0-4e19-ac58-93643c132bb7)

## Dependencies

- C++17 compiler (e.g. GCC 8+)
- [CMake](https://cmake.org) (3.22+)
- [EPICS Base](https://epics-controls.org/resources-and-support/base/)
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) (fetched automatically by default)

## Quick Start

```bash
git clone https://github.com/BCDA-APS/pvtui
cd pvtui
mkdir build && cd build
cmake .. -DEPICS_BASE=/path/to/epics/base
make
```

Included applications are built by default. For example, to launch the motor record UI:

```bash
./bin/pvtui_motor --macro "P=xxx:,M=m1"
```

See the [tutorial](https://bcda-aps.github.io/pvtui/tutorial.html) for creating your own application.

## Documentation

Full documentation is available at [https://bcda-aps.github.io/pvtui/](https://bcda-aps.github.io/pvtui/)

## License

See [LICENSE](LICENSE) for details.
