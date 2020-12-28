#ifndef LIBASSETMAP_DIRECTORYMETADATA_H
#define LIBASSETMAP_DIRECTORYMETADATA_H

#include "ICompress.h"
#include "IHasher.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <vector>

// clang-format off
//! \file DirectoryMetadata.h
//!	\brief Contains the base logic for computing necessary sizes required to
//!	       create an archive.
/*! \verbatim
An archive is laid out according to the compile-time integer size.
By default this is a uint32_t though it can be changed via CMake.

Target number of buckets is defined by IHasher.
All offsets are specified relative to the beginning of the file. This makes
it easier to inspect values in a hex editor and was chosen since the bucket
table is at the start which would buy very little additional space if it
were made relative to the current pointer. If you need more space, use
64-bit values.

Each file entry is padded to the sizeof() the chosen integer width. This
is not actually required but is designed to avoid the overhead on unaligned
accesses in cases where the compiler knows the CPU would happily read
unaligned data.

All integer values are stored little-endian.

If a dictionary has been used, it is concatenated to the end and a value
indicating its size is appended after it.

A single byte is placed at the end of the file indicating whether a
dictionary is present or not. If the value is anything other than 0 or 1, an
exception will be thrown as this is assumed to indicate a future version.

Assuming a 32-bit size with 2 buckets across 3 files, which all happened to
be compressed to a few bytes each, this would be the layout:

B = bucket
I = Item
+------+----------------------------------+---+---+---+---+---+---+---------------------------+-------------------------+---+---+---+------------------+-----+---------+---------+
|      |                 0                | 1 | 2 | 3 | 4 | 5 | 6 |             7             |            8            | 9 | A | B |         C        |  D  |    E    |    F    |
+------+----------------------------------+---+---+---+---+---+---+---------------------------+-------------------------+---+---+---+------------------+-----+---------+---------+
| 0x00 |              [bucket_count] = 2              |          [B0][offset] = 0xC           |         [B1][offset] = 0x24         |             [B0][I0][size] = 5             |
+------+----------------------------------------------+-----------+---------------------------+-------------------------------------+--------------------------------------------+
| 0x10 |                [B0][I0][name] = "f1.txt\0"               |                       [I0][data] = {bytes}                      |            [B0][Iend][size] = 0            |
+------+----------------------------------+-----------+-----------+---------------------------+-------------------------------------+--------------------------------------------+
| 0x20 |      [B0][Iend][name] = "\0"     | [padding] |           [B1][I0][size] = 5          |                           [B1][I0][name] = "file2.S\0"                           |
+------+----------------------------------+-----------+---+-----------------------------------+-------------------------------------+--------------------------------------------+
| 0x30 |             [B1][I0][data] = {bytes}             |             [padding]             |          [B1][I1][size] = 7         |          [B1][I1][name] = "abc\0"          |
+------+--------------------------------------------------+-------+---------------------------+-------------------------------------+------------------------+-------------------+
| 0x40 |               [B1][I1][data] = {bytes}                   | [B1][I1][padding] = {any} |          [B1][I2][size] = 3         | [B1][I2][name] = "x\0" | [B1][I2][data]... |
+------+----------------------------------+-----------+-----------+---------------------------+-------------------------+-----------+------------------+-----+-------------------+
| 0x50 | ...[B1][I2][data] = {bytes}      | [padding] |          [B1][Iend][size] = 0         | [B1][Iend][name] = "\0" | [padding] |   [hasDict] = 0  |   {EOF} unaddressable.  |
+------+----------------------------------+-----------+---------------------------------------+-------------------------+-----------+------------------+-------------------------+
\endverbatim
*/
// clang-format on

namespace AssetMap {
	class DirectoryMetadata {
		std::vector<std::vector<std::filesystem::directory_entry>> buckets;
		size_t totalCompressBound		 = 0;
		size_t totalNumFiles				 = 0;
		size_t totalFileNameSize		 = 0;
		size_t totalAlignmentPadding = 0;
		size_t dictionarySize				 = 0;

	public:
		//! \brief Construct an instance of DirectoryMetadata.
		//!
		//! The constructor will iterate through the directory passed and compute
		//! all necessary data required to archive it. Iteration is recursive and
		//! anything that is not a regular file will be ignored.
		//! \param hasher Any implementation satisfying IHasher.
		//! \param comp   Any implementation satisfying ICompress.
		//! \param ent 		An entry that references a valid, readable directory.
		explicit DirectoryMetadata(const IHasher& hasher,
															 ICompress& comp,
															 const std::filesystem::directory_entry& ent);

		//! \brief Obtain the worst-case required space to compress the directory
		//! 			 passed to the constructor for the given hash and compression algo
		//! \return The size required to compress the entire dir contents.
		[[nodiscard]] size_t TotalRequiredSpace() const noexcept;

		//! \brief Obtain the offset for the beginning of data entries.
		//! \return The equivalent of [file start] + [size of all bucket data]
		[[nodiscard]] ptrdiff_t DataStart() const noexcept;

		//! \brief Obtain a list of buckets and their entries.
		//!
		//! This function provides a vector of vector of directory_entries.
		//! The indexable position of an outer vector element is its bucket ID.
		//! Every directory entry is guaranteed to be a regular file. Or at least,
		//! it was at the time of inspection - if you (or something else) change the
		//! on-filesystem data, things may break.
		//! \return \c vector\<vector\<fs\::directory_entry\>\>
		[[nodiscard]] const decltype(buckets)& Buckets() const noexcept;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_DIRECTORYMETADATA_H
