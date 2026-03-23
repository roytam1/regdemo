# Port audit: original VB behavior mapped into C

## Original project structure

- `REGDEMO.FRM`
  - main form titled `Win32 RegDemo`
  - left listbox used as a pseudo-tree of registry keys
  - right listbox shows value names and formatted value data
  - resize logic keeps the original two-pane layout
- `FORM2.FRM`
  - modal editor window for `REG_SZ` and `REG_DWORD`
  - `REG_DWORD` editor exposes hexadecimal/decimal radio buttons
- `REGMODUL.BAS`
  - all registry API declarations
  - root-key mapping
  - key/value enumeration
  - edit dialog data marshaling
  - error translation

## Visual behavior preserved

- gray form background
- fixed-pitch `Courier New` listboxes
- bold `MS Sans Serif` labels/buttons
- same captions and helper text
- same left/right pane arrangement
- same resize rules as the VB `Form_Resize` handler

## Functional behavior preserved

- top-level roots are loaded at startup
- key click loads values for the selected key
- key double-click expands or collapses one level
- `+` marker means the subkey has children
- `*` marker means the subkey cannot be opened for read
- value double-click opens the editor
- `REG_SZ` and `REG_DWORD` are editable
- other types are browse-only
- horizontal scrolling is based on measured text width

## Notable original VB quirks

The original code had a few quirks that are important for a faithful port:

- it did **not** use a real tree control; it parsed indentation and marker characters from listbox text
- it formatted list columns by padding with spaces and depending on a fixed font
- it handled `REG_DWORD` values as unsigned even though VB `Long` is signed
- it capped editable/displayed data at about 20 KB
- it only edited `REG_SZ` and `REG_DWORD`

## C rewrite strategy

The C version preserves the visible behavior but uses metadata internally instead of reparsing the display strings. That keeps the UI equivalent while making the implementation safer and easier to maintain.

- listbox rows still look like the VB version
- row metadata stores the real root/path/value identity
- editor save refreshes the right pane after a successful write
- write calls use proper Win32 buffer sizes for registry data
