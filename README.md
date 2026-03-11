# Academic ARM Linker

This project involves the development of a subset of features constituting a linker capable of merging multiple ELF object files for Big Endian ARM processors. The result of the merge is a relocatable file that can be used in future compilation stages.

## Project structure and main functions

The source code is divided into two modules separating ELF file parsing logic from merging logic.

- `src/ElfFile.c`: Implements various functions for ELF file manipulation:
    - `ElfFileNew`: Reads an ELF file from disk and creates an intermediate memory representation that will be manipulated during the various other implemented operations.
    - `ElfFileEmptyNew`: Acts as a default constructor, generating a minimal ELF file with only its header configured for an ARM32 BE architecture.
    - `ElfFileDestroy`: Frees all memory used by an `ElfFile` structure.
    - `ElfFileWriteToDisk`: Serializes an `ElfFile` structure to disk, respecting alignment constraints specific to each section.
    - `ElfFileDisplayHeader`, `ElfFileDisplaySections`, `ElfFileDisplayRelocations`, etc.: Various displays.
- `src/Linker.c`: Groups the business logic of linking and the implementation of the relocation engine.
    - `MergeFiles`: Merge orchestrator. Initialises a `LinkerContext` representing the internal state of the linker and sequentially executes the merging of sections, symbols, and the resolution of relocations.
    - `LinkerMergeSections`: Merges all sections from the two input files while keeping track of their movements via `LinkerContext::f1_sec_to_out_idx` and `LinkerContext::f2_sec_to_out_idx`.
    - `GenerateMergedSymbolsTable`: Merges symbol tables by applying name resolution rules, while updating the section indices of moved symbols.
    - `LinkerProcessRelocations`: Iterates over all relocation entries (SHT_REL). For each entry, it identifies the target symbol, calculates the patch value, and updates the relocation metadata for the final file.
    - `LinkerPatchRelocationBinary`: Operates on ARM instructions in the data buffers by implementing specific calculations for each type.
- `include/`: Function declarations.
- `include/uthash.h`: External library allowing the use of hash tables.
- `tests/`: Unit tests for the parser.
- `linker_behavioural_tests`: Behavioural tests for the Linker.

## Developed utilities

Two programs `ld` and `readelf` were designed to respectively allow merging and inspecting ELF files.

They can be compiled respectively by the following commands, after automake configuration:

```bash
autoreconf -vif && ./configure
make ld
make readelf
```

## Instructions for use

The merger is executed with the command :
```bash
./ld -o [output_file] -f [input_file1] [input_file2]
```

The readelf utility allows inspecting the internal structures of ELF files to validate the parsing and merging steps.

Available options:
- `−H, --header`: Displays the ELF header.
- `-S, --section-table`: Displays the section table.
- `-s, --symbols`: Displays the symbol table.
- `-r, --relocations`: Displays the relocation entries.
- `-x, --hex-dump [section]`: Displays the hexadecimal content of a specific section.

## Contributors
- [Mohand Tahar Belkebir](https://github.com/mtbelkebir): Project Architect and Lead Developer. ELF Parsing engine, Symbols resolution, ARM relocation logic, Integration tests.
- [Guillaume Huard](mailto:Guillaume.Huard@imag.fr): Build infrastructure, starter code.
- [Thomas Medina](https://github.com/Thomas-mdn-88): Unit tests.
## Attributions
- [Troy D. Hanson](https://troydhanson.github.io/uthash/): UTHash library.
- [ThrowTheSwitch](https://throwtheswitch.org): Unity unit testing library.
