# FTK

FTK is a 2D toolkit library in development written using Vulkan Graphics API.

## Why Vulkan

Vulkan is the replacement API for OpenGL. It allows more optimization opportunities. Which we make use extensively in this project.

## Getting Started

Download FTK git repositories at https://github.com/expertisesolutions/ftk .

Then, install boost-build (apt-get install libboost1.67-tools-dev, replace version accordingly, for Ubuntu and yay -S boost-build in Arch Linux).

And install conan (https://docs.conan.io/en/1.8/installation.html).

Add the pdeps remote to conan and run install to bring and build dependencies, then run b2 to compile:

```
$ conan remote add pdeps https://api.bintray.com/conan/pdeps/deps
$ conan install . --build
$ b2 --use-package-manager=conan
```
