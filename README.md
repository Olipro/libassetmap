# LibAssetMap

LibAssetMap is a library designed to bundle together a hierarchy of files into a singular format. A CLI utility is provided which supports compression, decompression and printing information about an archive. All code is written in C++17.

At compile time, a numeric size must be selected for storing metadata used for seeking to offsets. This defaults to `uint32_t` but can be customised by passing `-DLIBARCHIVEMAP_SIZE_TYPE` to CMake. With the default, you are limited to a total archive size of `2^32` - if you intend to create archives larger than 4GB set it to something bigger. You can freely also drop this to something like `uint16_t` if your archives will be very small. This value is used in `MemOps.h`.

In any case however, exceeding the size type's maximum **will** cause unsigned integer overflow and break things.

Availability of the (officially optional) `uint_Xt` types in `<cstdint>` is assumed. If your platform lacks this, feel free to submit a patch.

Whilst it would be possible to compute perfect hashes of file paths, or potentially even compress file paths themselves, this currently does not happen - although it is planned as an optional feature.

## Overview

In simple terms: let's say you have a directory with a bunch of files in it. You'd like to compress them because either they contain lots of repetitive data which is slow to retrieve from disk or you want to save space. You additionally want the ability to load any file from your generated archive reasonably quickly - this is the tool for you.

## Features

Files are memory-mapped during compression and the archive file itself is memory mapped during decompression. Currently, only Windows and Linux are tested.

Only CityHash and zstd are implemented for hashing & (de)compression.

Once fully initialised, calls to retrieve a file from an archive can be executed concurrently across multiple threads; no form of locking exists and the library class instances must live at least as long as the threads retrieving data.

## Building

The project consists of a couple of git submodules. if your version of git doesn't automatically check them out: `git submodule init && git submodule update`

With the submodules checked out, building the source follows the standard CMake convention. The library is compiled to object files. For an example of how to use these, examine the `assetmapcli` target in `CMakeLists.txt` - you are of course free to compile the object target into a static or shared library via your own `add_library`. `assetmapcli` is also a good code example for how to use the library.

## Usage

`assetmapcli` can be called with `--help` for a list of options. Depending on your purpose, you can select various compression levels, compression strategies, dictionary sizes and bucket sizes.

Depending on filename strings, you can tune and/or experiment with the above parameters to obtain the values that work best for your situation.

There is absolutely no comprehensive verification of archive integrity; if you input a file that happens to have the byte `1` or `0` at the end, it **will** try to process it and result in Undefined Behaviour.

## Future Goals

* Implement logic to identify the ideal bucket size to obtain a particular average or maximum number of entries per bucket.
* Add tail trimming of the bucket table - empty bucket fields above the last used bucket are unnecessary.
* Support alternative hashing algorithms - this is relatively trivial as all you are required to do is implement an interface.
* Implement determining the ideal dictionary size. Currently, you can do this yourself manually through the CLI tool.
* Support perfect hashing. This would require a transformation step, filenames would be destroyed and replaced with a generated source file that symbolises the strings into constants. (e.g. "path/to/file" becomes something `constexpr auto PATH_TO_FILE = <bucket>` or an enum value - undecided; the principle remains the same.)
* Add hugepage support if Linux ever gets around to supporting it for files on common filesystems.
* Add a "fake" `IMemoryMapper` implementation which merely copies bytes from a file stream into memory for platforms which lack the capability to directly map files.

## Non-Goals

* Archive Integrity.
  * If you need to protect an archive from corruption, compute a digest and verify it before access or use a filesystem and/or transmission mechanism that guarantees this for you.
* Malicious Inputs.
  * If you can't trust the input data, compute a cryptographic hash and sign it or use a filesystem and/or transmission mechanism that guarantees this for you.
* Compatibility
  * Archives generated with (for example) `uint64_t` as the desired bucket count, bucket offset and element size are **not** compatible with other sizes. The CLI tool is no exception to this. 
    * However, two binaries built with **identical** configuration and source code for **different** CPU architectures is expected to work with the exception of esoteric systems that do not read and write a single `uint8_t` in big-endian (e.g. the byte value 128 should be written as `10000000`)
  * The same will apply to changes in hash algorithm or compression. In short, readability of an archive is deliberately tied to the parameters chosen to compile the source code.
  * Future versions may change the layout significantly; if this occurs, as a trivial sanity mechanism, the final byte of the archive will never be `0` or `1` and older versions will refuse to read such archives.

## Dictionaries

Depending on your data, it may benefit from using a dictionary and then again, it may not. Dictionaries can be re-used after generation. It is recommended that your corpus of input files are related types. Consequently, if your project consists of many varied kinds of data, consider creating multiple different archives with different dictionaries.

If you have many files with fragments of data that is repeated across them rather than just within each individual file, a Dictionary will likely buy you decent savings.

A dictionary is always concatenated to the end of an archive after which a `lam_size_t` and a final `uint8_t` are then appended indicating the size of the dictionary and the flag (`1`) denoting its presence.

## Minutiae

* We assume that the required alignment of a `lam_size_t` is equal to its `sizeof()` - however, reads and writes are encoded as byte-level operations and left to the compiler to optimise if it can. For architectures that do not permit unaligned accesses, this will certainly result in byte-by-byte operations. In future, this can be hinted for optimisation more robustly via `std::assume_aligned` in C++20.
* We assume that Linux/Windows will align the start address of the archive to the size of a memory page (typically 4KB or more) and that consequently, a page is never smaller than `lam_size_t`.
* The `lam_size_t` for the dictionary (if present) may or may not be aligned depending on the size of the dictionary. The field is only read once to load the dictionary; loading the dictionary itself will take far longer than a single unaligned read.
* case-sensitivity of files is respected. Consequently, files with paths that differ only by case will not extract properly into a case-insensitive filesystem. Of course, the main use case is to decompress files directly from the archive into memory which makes the filesystem irrelevant.
* We verify at compile time that a `uint8_t` is an `unsigned char` - if your platform differs, you can remove the check and run the unit tests.
* A workaround is implemented for a deficiency in libstdc++'s std::reduce - namely, we use std::accumulate instead.

## Archive structure

See the comment in `include/DirectoryMetadata.h` for a description and overview of the internal layout.

