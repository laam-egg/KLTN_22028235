Go to the `resources` directory to change
the PMD engine DLL.

The DLL must export the interface functions
as described in `include/pmd_engine/engine.h`.

The current PMD engine here is an implementation
of EMBER2024 LGBM model, compiled in
[this project](https://github.com/pe-malware-detection/ember2024-lgbm)
with VS2022 on Windows 10.
