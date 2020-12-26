#ifndef LIBASSETMAP_IFILE
#define LIBASSETMAP_IFILE

#include <cstddef>
#include <vector>

namespace AssetMap {
	class IFile {
	public:
		virtual const std::vector<std::byte>& Data() const noexcept = 0;
		virtual ~IFile() noexcept																		= default;
	};
} // namespace AssetMap
#endif