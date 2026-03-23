# RegDemo C port (Win32, NT4-safe API set)

This is a straight Win32 C rewrite of the original VB4/32 project:

- `REGDEMO.FRM` -> main browser window
- `FORM2.FRM` -> modal value editor window
- `REGMODUL.BAS` -> registry helpers, formatting, and editor logic

The port keeps the original UI model instead of replacing it with a tree-view:

- left pane is still a listbox that fakes a key tree with indentation
- `+` still means a subkey has children
- `*` still means the key cannot be opened with `KEY_READ`
- right pane is still a fixed-pitch listbox with padded columns
- double-clicking a key expands/collapses one level
- double-clicking a value opens the editor
- only `REG_SZ` and `REG_DWORD` are editable, matching the VB demo
- `REG_DWORD` editing still defaults to hexadecimal with a decimal toggle

## Intentional fixes versus the original VB code

A few small fixes were made while keeping the look and behavior intact:

1. `REG_SZ` writes include the terminating NUL. The VB code wrote `Len(string)` bytes, which is not the recommended size for `RegSetValueEx`.
2. `DWORD` input is validated before save. The VB code could silently fail on malformed input.
3. Several non-editable value types are rendered more readably in the value pane instead of depending on VB string behavior around embedded NUL bytes.
4. `HKEY_CURRENT_CONFIG` is shown on NT-family systems as well. The original demo only added it on the Win9x path, even though NT exposes it.

## NT4 compatibility notes

The source itself sticks to an NT4-era Win32 API surface:

- ANSI windowing and registry APIs
- standard USER/GDI controls only
- no common controls dependency
- no shell helpers
- no visual styles
- no `SetWindowLongPtr`, no newer platform helpers

That said, **toolchain choice still matters**. A modern compiler can emit a binary that uses a runtime or codegen choices not suitable for NT4, even if the source code is API-compatible.

For actual NT4 deployment, build with an x86 toolchain that can still emit old-style Win32 binaries, for example:

- Visual C++ 6.0 / Visual Studio .NET 2003 era toolchains, or
- an x86 MinGW toolchain configured to link against `msvcrt.dll`

Also set the subsystem version to `4.0`.

## Files

- `regdemo.c` - full C source
- `regdemo.rc` - icon resource
- `resource.h` - resource IDs
- `regdemo.ico` - extracted from the original `REGDEMO.FRX`
- `build_msvc.bat` - example MSVC/VC6-style build
- `build_mingw.bat` - example MinGW build

## Build examples

### MSVC / VC6-style command prompt

```bat
build_msvc.bat
```

### MinGW

```bat
build_mingw.bat
```

## Recommended follow-up work

If you want this moved one step beyond a literal port, the next safe upgrades would be:

- optional Unicode build path
- optional x64 build path
- editable `REG_EXPAND_SZ` and `REG_MULTI_SZ`
- owner-drawn flat buttons for an even closer VB visual match
