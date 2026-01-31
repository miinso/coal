# Upstream README
> For the full project docs and the original README, see:
> https://github.com/coal-library/coal

# coal-lite
coal-lite is a minimal, dependency-light fork of Coal (the collision detection
library previously known as HPP-FCL/FCL). This fork focuses on the core C++
library so it can be embedded cleanly in CMake or Bazel builds across Windows,
macOS, and Linux.

## Why this fork exists
- Keep the build simple and deterministic with standard CMake.
- Focus on the C++ core for application integration.
- Avoid extra tooling and ecosystems that are not required for lite usage.

## Differences vs upstream
- Only the C++ core library and tests are built.
- Python bindings, ROS packaging, and documentation toolchains are not included
  in the build.
- Optional deps kept: Assimp (mesh loader), Octomap, Qhull, and Tracy.
- No jrl-cmakemodules; use standard `find_package` instead.
- Mesh loader headers are installed only when `COAL_WITH_ASSIMP=ON`.

## Build
Required: CMake >= 3.22, a C++ compiler, Eigen3, and Boost
(filesystem, serialization).

Optional: Assimp, Octomap, Qhull, Tracy.

Example:
```
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release
```

Windows note: if Boost is not in a default search path, set `BOOST_ROOT` and
`BOOST_LIBRARYDIR` when configuring.

## CMake targets
- `coal::coal`
- `coal::coal_assimp` (only when `COAL_WITH_ASSIMP=ON`)
