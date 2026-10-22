# AdaptMON – Adaptive Network Monitoring Platform

AdaptMON is a C++23 platform for building high-performance applications that monitor and
process network traffic.

> [!WARNING]
> ## Status and disclaimer
> This project is under active development. All sources are work in progress and all APIs can
> change in the future without notice. No stability guarantees are provided at this stage.

## Overview

Rather than targeting a single use case, AdaptMON provides reusable building blocks for
the whole life cycle of network traffic processing — acquisition, parsing, analysis,
filtering, editing, and generation. The platform is designed to scale from capture-file
processing to high-speed live traffic.

The repository consists of a modular C++ library (`amon-lib`) implementing these building
blocks and a set of command-line applications built on top of it. See the
[apps](apps/README.md) directory for details about the individual tools.

## Building

### Requirements

- Linux
- GCC 14 or newer (C++23)
- CMake 3.25 or newer
- DPDK (optional, enabled by default)
- libpcap (optional, enabled by default)

### Installing requirements

On Fedora/RHEL-based systems, all build requirements can be installed automatically using
the provided `prepare-machine` script:

```bash
./prepare-machine
```

The script installs the compiler toolchain, CMake, DPDK, libpcap and other development
dependencies. It also supports a `dev-container` role for preparing a Docker-based
development environment.

Alternatively, a Docker image with all dependencies preinstalled can be built and entered
with:

```bash
# Build the development container image and enter it
make enter-dev-container

# Only build the image, then run it manually
make dev-container
```

### Building the project

The project is built using CMake via the Makefile wrapper:

```bash
# Release build (default)
make

# Debug build with tests and sanitizers
make debug-build

# Run tests
make test

# Build RPM packages
make rpm
```

## License

Released under the [BSD 3-Clause](LICENSE) license, © 2026 CESNET, z.s.p.o.
