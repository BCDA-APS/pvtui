.. _building:

Building from Source
====================
At this time no binaries for PVTUI are provided so you must build it from source.


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
    mkdir build && cd build
    cmake ..
    make

PVTUI makes available several optional flags when running cmake

* ``-DEPICS_BASE``: (Default from environment) The path to EPICS base installation
* ``-DFETCH_FTXUI``: (Default ON) Whether or not to clone and compile FTXUI
* ``-DBUILD_APPS``: (Default ON) Whether or not to build applications in apps/ directory
* ``-DBUILD_TESTS``: (Default OFF) Whether or not to build tests in tests/ directory
* ``-DBUILD_DOCS``: (Default OFF) Whether or not to build Doxygen documentation

To install the cmake configuration files so other cmake projects can find the PVTUI library,
set an install prefix with ``cmake -DCMAKE_INSTALL_PREFIX=/path/to/install/prefix`` and then
run ``make install``.
