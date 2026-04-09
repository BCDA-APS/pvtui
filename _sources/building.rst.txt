.. _building:

Building from Source
====================
This page describes how to compile PVTUI from source.
Note that PVTUI releases starting with v0.2.0 provide pre-built binaries for Linux x86-64.

Dependencies
------------

* C++17 compiler (e.g. GCC 8+)
* `CMake <https://cmake.org>`_
* `FTXUI <https://github.com/ArthurSonzogni/FTXUI>`_
* `EPICS Base <https://epics-controls.org/resources-and-support/base/>`_


Building
--------

Clone PVTUI from GitHub and build with cmake ::

    git clone https://github.com/BCDA-APS/pvtui
    cd pvtui
    cmake --preset release -DEPICS_BASE=/path/to/epics/base
    cmake --build build/release

If the ``EPICS_BASE`` environment variable is set, the ``-DEPICS_BASE`` flag can be omitted.

PVTUI makes available several optional flags when running cmake

* ``-DEPICS_BASE``: (Default from environment) The path to EPICS base installation
* ``-DEPICS_STATIC``: (Default OFF) Whether or not to link EPICS statically
* ``-DFETCH_FTXUI``: (Default ON) Whether or not to clone and compile FTXUI
* ``-DBUILD_APPS``: (Default ON) Whether or not to build applications in apps/ directory
* ``-DBUILD_TESTS``: (Default OFF) Whether or not to build tests in tests/ directory
* ``-DBUILD_DOCS``: (Default OFF) Whether or not to build Doxygen documentation

To install the cmake configuration files so other cmake projects can find the PVTUI library,
set an install prefix with ``cmake -DCMAKE_INSTALL_PREFIX=/path/to/install/prefix`` and then
run ``cmake --install <build_dir>``.
