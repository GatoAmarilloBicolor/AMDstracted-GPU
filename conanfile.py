from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake


class AmdgpuConan(ConanFile):
    name = "amdgpu_abstracted"
    version = "1.0.0"
    description = "Userland AMD GPU driver abstraction"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "meson.build", "meson_options.txt", "**/*.c", "**/*.h"

    def build_requirements(self):
        self.tool_requires("meson/1.2.1")

    def requirements(self):
        # Add any external dependencies here
        # For now, none required as it's pure C
        pass

    def layout(self):
        # Use meson layout
        pass

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["amdgpu"]
