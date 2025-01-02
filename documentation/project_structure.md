# File Extensions

## `.h` - Regular Header Files
`.h` is the extension for regular header files as you know them from other projects. We use them for function
declarations that are part of a public and internal API. These files can be included by developers and users.

## `.impl.h` - Implementation Header Files
`.impl.h` is the extensions for header files that contain inline function definitions (no declarations!). Mostly used
for definition of templated functions/classes. These files are `#included` at the end of regular header files to provide
inline function definitions for function declared in the regular header.

**They must never be included by users!**

## `.cpp` - Regular Source Files
`.cpp` is the extension for regular source files. These files are added to CMake targets and compiled.

---

# Folder Structure

