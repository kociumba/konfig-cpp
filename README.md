# konfig

konfig is a lightweight C++ library for managing configuration data using TOML files.

It provides a simple,
immediate-mode API for defining and serializing configuration sections, making it easy to integrate into C++ projects.
Unlike its Go and Rust counterparts, konfig for C++ works around the lack of reflection by using a functional,
immediate-mode approach inspired by libraries like imgui and microui.

## Installation

Simply clone the repo or copy and paste the neede files into your project.

Files required by konfig to function are:

- ./konfig.h
- ./konfig.cpp
- ./thirdpart/toml++/toml.hpp

You can also provide your own version of the toml++ library as long as it is discoverable by konfig.

For use in a cmake project you can directly include the `CMakeLists.txt` like so:

```cmake
add_subdirectory(path/to/konfig)

add_executable(your_project main.cpp)

target_include_directories(your_project PUBLIC path/to/konfig)
target_link_libraries(your_project PRIVATE konfig)
```

> [!NOTE]
> cmake install is not yet supported.

## Usage

Basic use:

```c++
#include <konfig>

int int_val = 0;
std::string string_val = "";

int main() {
    auto mngr = konfig::create("config.toml");
    
    KONFIG_SECTION(mngr, "example", {
        config->field("int", &int_val);
        config->field("string", &string_val);
    });
    
    mngr->load();
    int_val = 42069;
    string_val = "changed";
    mngr->save();
    return 0;
}
```

The convenience macro `KONFIG_SECTION` should be used to define simple unchanging sections but, for more complex
scenarios the schema is just functions so you can something like this:

```c++
#include <konfig>
#include <print>

int int_val = 0;
std::string string_val = "";
auto mngr = konfig::create("config.toml");

void cfg_int() {
    mngr->field("int", &int_val);
    if (int_val == 42069) {
        sdt::print("nice\n");
    }
}

void cfg_string() {
    if (mngr->begin_scope("strings")) {
        mngr->field("string", &string_val);
        mngr->end_scope();
    }
}

void cfg_example() {
    cfg_int();
    cfg_string();
}

int main() {
    mngr->register_section("example", cfg_example);
    
    if (!mngr->load()) {
        std::print(stderr, "Error: {}\n", mngr->get_err().value());
        return 1;
    }
    
    int_val = 42069;
    string_val = "changed";
    
    if (!mngr->save()) {
        std::print(stderr, "Error: {}\n", mngr->get_err().value());
        return 1;
    }
    return 0;
}
```

## Requirements

konfig requires C++23 to work due to dependence on `std::print`, in turn the features from C++23 which konfig uses
should be supported by all the major C++ compilers.