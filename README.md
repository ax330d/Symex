# Symex

## About and usage

This is a tool to resolve symbols of running application. You provide
application PID, module name and offset (or address) to resolve.

```
Symex\Release>Symex.exe --pid 11008 -m chrome_child.dll -o 1122334 -v
Symex - symbol resolver, v 0.3
Using default programSymbolPath = C:\Symbols
dwBaseAddr = 261095424
DBGHELP: C:\Symbols\chrome_child.dll.pdb\2BD21F87ADDF496AB15BE30C003CEBD01\chrome_child.dll.pdb - mismatched pdb
DBGHELP: C:\Symbols\chrome_child.dll.pdb\41B9BDD32ABE4E59844480CE83A0A21F1\chrome_child.dll.pdb - mismatched pdb
DBGHELP: chrome_child - private symbols & lines
        C:\Symbols\chrome_child.dll.pdb\4E70D83A8AB644FE908B17EC5187BB3C1\chrome_child.dll.pdb
blink::ScrollingCoordinator::scrollableAreaScrollbarLayerDidChange+0x262
```

If you provide no -v (vebose output), then you will receive only
resolved symbol (last line in paste).

By default Symex considers that symbols are in C:\\Symbols. You can
overide this with -s (or --symbol-path).

## Disclaimer

Some parts of code are taken somewhere from internet.
