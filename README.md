# PVTUI

[![Documentation](https://img.shields.io/badge/docs-sphinx-blue)](https://bcda-aps.github.io/pvtui/)

PVTUI is a library for creating terminal/text based user interfaces for EPICS built on [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
and [pvAccessCPP](https://github.com/epics-base/pvAccessCPP).

![pvtui_motor](https://github.com/user-attachments/assets/5e6690eb-d386-492e-ae35-2e70efdbbcfc)

## Dependencies

- C++17 compiler (e.g. GCC 8+)
- [CMake](https://cmake.org) (3.22+)
- [EPICS Base](https://epics-controls.org/resources-and-support/base/)
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) (fetched automatically by default)

## Quick Start

```bash
git clone https://github.com/BCDA-APS/pvtui
cd pvtui
cmake --preset release
cmake --build build/release
# optionally install (default prefix: `~/.local`, or specify --prefix)
# cmake --install build/release
```

See the [tutorial](https://bcda-aps.github.io/pvtui/tutorial.html) for creating your own applications.

## Documentation

Full documentation is available at [https://bcda-aps.github.io/pvtui/](https://bcda-aps.github.io/pvtui/)

## License

See [LICENSE](LICENSE) for details.
