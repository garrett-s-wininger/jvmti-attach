# JVM TI - Attach

This application is meant to enable the remote troubleshooting of JVM processes
on the local machine by attaching to them, loading agents, and performing other
activities which may be helpful before finally detaching and ending the the
troubleshooting session, leaving the remote process intact.

## System Requirements

MacOS is the only supported operating system that the code can handle at this
time. Linux support is planned as the next addition.

## Compilation

The following tools are required to build the application:

* C++ Compiler (23+)
* CMake (3.21+)
* JDK (21)

If you have JDK installed at the system level, the following command should be
all you need to generate the resulting executable:

`cmake . && cmake --build .`

For Homebrew-based JDK installations, you'll want to use:

`cmake --preset darwin-homebrew-openjdk21`

Other combinations can be achieved through a custom `CMakeUserPresets.json`
file described in CMake's
[upstream documentation](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)

## Execution

Simply run `attachment/jvm-ti` after the build is complete to be presented with a
listing of currently running JVM processes on the system. Upon selecting a PID to
monitor, the application will attempt to attach to the remote JVM and perform
it's monitoring/interrogation tasks.
