#include "ZSTDComp.h"

#include <fstream>
#include <numeric>
#include <type_traits>

#include "dictBuilder/zdict.h"
#include "zstd.h"

// Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95833
// Also defined in AssetMapCLI.cpp
#ifdef __GLIBCXX__
#	define stdReduce std::accumulate
#else
#	define stdReduce std::reduce
#endif

using namespace AssetMap;

namespace fs = std::filesystem;

template <class Ctx>
constexpr ZCtx<Ctx>::ZCtx(ZCtx&& rhs) noexcept : ctx{rhs.ctx} {
	rhs.ctx = nullptr;
}

template <class Ctx>
constexpr ZCtx<Ctx>& ZCtx<Ctx>::operator=(ZCtx&& rhs) noexcept {
	if (ctx != nullptr)
		Dispose();
	ctx			= rhs.ctx;
	rhs.ctx = nullptr;
	return *this;
}

template <class Ctx>
constexpr ZCtx<Ctx>& ZCtx<Ctx>::operator=(Ctx* ptr) noexcept {
	if (ctx != nullptr)
		Dispose();
	ctx = ptr;
	return *this;
}

template <bool... flags>
constexpr bool OneOf() {
	return 1 == ((flags ? 1 : 0) + ...);
}

template <class Ctx>
constexpr void ZCtx<Ctx>::Create() {
	constexpr auto isCctx = std::is_same_v<Ctx, ZSTD_CCtx>,
								 isDctx = std::is_same_v<Ctx, ZSTD_DCtx>;
	static_assert(OneOf<isCctx, isDctx>());
	if constexpr (isCctx)
		ctx = ZSTD_createCCtx();
	else if constexpr (isDctx)
		ctx = ZSTD_createDCtx();
}

template <class Ctx>
void ZCtx<Ctx>::Dispose() {
	constexpr auto isCctx = std::is_same_v<Ctx, ZSTD_CCtx>,
								 isDctx = std::is_same_v<Ctx, ZSTD_DCtx>;
	static_assert(OneOf<isCctx, isDctx>());
	if constexpr (std::is_same_v<Ctx, ZSTD_CCtx>)
		ZSTD_freeCCtx(ctx);
	else if constexpr (std::is_same_v<Ctx, ZSTD_DCtx>)
		ZSTD_freeDCtx(ctx);
	ctx = nullptr;
}

template <class Ctx>
constexpr ZCtx<Ctx>::operator bool() const noexcept {
	return ctx != nullptr;
}

template <class Ctx>
ZCtx<Ctx>::operator Ctx*() const noexcept {
	return ctx;
}

template <class Ctx>
ZCtx<Ctx>::~ZCtx() {
	if (ctx != nullptr)
		Dispose();
}

ZSTD::ZSTD(float dictRatio) noexcept : dictRatio{dictRatio} {}

ZSTD::ZSTD(both_t, float dictRatio) : ZSTD{dictRatio} {
	cCtx.Create();
	dCtx.Create();
}

ZSTD::ZSTD(compress_t, float dictRatio) : ZSTD{dictRatio} {
	cCtx.Create();
}

ZSTD::ZSTD(decompress_t) {
	dCtx.Create();
}

ZSTD::ZSTD(ZSTD&& rhs) noexcept :
		cCtx{std::move(rhs.cCtx)},
		dCtx{std::move(rhs.dCtx)},
		dictionary{rhs.dictionary},
		dictLen{rhs.dictLen},
		generatedDictionary{std::move(rhs.generatedDictionary)} {
	rhs.dictionary = nullptr;
	rhs.dictLen		 = 0;
}

ZSTD& ZSTD::operator=(ZSTD&& rhs) noexcept {
	cCtx								= std::move(rhs.cCtx);
	dCtx								= std::move(rhs.dCtx);
	dictionary					= rhs.dictionary;
	dictLen							= rhs.dictLen;
	generatedDictionary = std::move(generatedDictionary);
	rhs.dictionary			= nullptr;
	rhs.dictLen					= 0;
	return *this;
}

size_t ZSTD::Compress(const uint8_t* src,
											size_t srcLen,
											uint8_t* dst,
											size_t dstLen) {
	return ZSTD_compress2(cCtx, dst, dstLen, src, srcLen);
}

size_t ZSTD::Decompress(const uint8_t* src,
												size_t srcLen,
												uint8_t* dst,
												size_t dstLen) {
	return ZSTD_decompressDCtx(dCtx, dst, dstLen, src, srcLen);
}

size_t ZSTD::CalcCompressSize(size_t len) const noexcept {
	return ZSTD_compressBound(len);
}

size_t ZSTD::CalcDecompressSize(const uint8_t* src, size_t len) const noexcept {
	return ZSTD_getFrameContentSize(src, len);
}

bool ZSTD::CreateDictionary(fs::directory_entry samplesDir) {
	std::vector<size_t> sizes;
	std::vector<char> inputBuf;
	{
		fs::recursive_directory_iterator dir{samplesDir};
		uintmax_t fileCount = 0;
		inputBuf.reserve(stdReduce(fs::begin(dir),
															 fs::end(dir),
															 uintmax_t{0},
															 [&fileCount](auto sum, auto& file) {
																 return file.is_regular_file()
																						? (++fileCount,
																							 (sum + file.file_size()))
																						: sum;
															 }));
		sizes.reserve(fileCount);
	}
	for (auto& file : fs::recursive_directory_iterator{samplesDir}) {
		if (file.is_regular_file()) {
			auto offset = inputBuf.size();
			inputBuf.resize(inputBuf.size() + sizes.emplace_back(file.file_size()));
			std::ifstream in{file.path(), std::ios::binary};
			in.read(inputBuf.data() + offset, file.file_size());
		}
	}

	std::vector<uint8_t> dictBuf((inputBuf.size() * dictRatio) +
															 ZDICT_CONTENTSIZE_MIN);
	auto dictSize = ZDICT_trainFromBuffer(dictBuf.data(),
																				dictBuf.size(),
																				inputBuf.data(),
																				sizes.data(),
																				sizes.size());
	if (!ZDICT_isError(dictSize)) {
		ZDICT_params_t params{};
		params.compressionLevel = compressionLevel;
		dictSize								= ZDICT_finalizeDictionary(dictBuf.data(),
																				 dictBuf.size(),
																				 dictBuf.data(),
																				 dictSize,
																				 inputBuf.data(),
																				 sizes.data(),
																				 sizes.size(),
																				 params);
		dictBuf.resize(dictSize);
		dictBuf.shrink_to_fit();
		generatedDictionary = std::move(dictBuf);
		UseDictionary(generatedDictionary.data(), generatedDictionary.size());
		return true;
	}
	return false;
}

void ZSTD::UseDictionary(const uint8_t* dict, size_t len) noexcept {
	dictionary = dict;
	dictLen		 = len;
	if (cCtx)
		ZSTD_CCtx_loadDictionary_byReference(cCtx, dictionary, dictLen);
	if (dCtx)
		ZSTD_DCtx_loadDictionary_byReference(dCtx, dictionary, dictLen);
}

void ZSTD::UseDictionary(std::filesystem::directory_entry ent) {
	generatedDictionary.resize(ent.file_size());
	std::ifstream{ent.path()}.read(
			reinterpret_cast<char*>(generatedDictionary.data()),
			generatedDictionary.size());
	UseDictionary(generatedDictionary.data(), generatedDictionary.size());
}

void ZSTD::SetCompressLevel(int level) noexcept {
	ZSTD_CCtx_setParameter(cCtx, ZSTD_c_compressionLevel, level);
}

void ZSTD::SetStrategyLevel(int level) noexcept {
	ZSTD_CCtx_setParameter(cCtx, ZSTD_c_strategy, level);
}

int ZSTD::MinCompressLevel() noexcept {
	return ZSTD_minCLevel();
}

int ZSTD::MaxCompressLevel() noexcept {
	return ZSTD_maxCLevel();
}

int ZSTD::DefaultCompressLevel() noexcept {
	return ZSTD_CLEVEL_DEFAULT;
}

constexpr auto strategyMin = ZSTD_STRATEGY_MIN;
constexpr auto strategyMax = ZSTD_STRATEGY_MAX;

int ZSTD::MinStrategyLevel() noexcept {
	return strategyMin;
}

int ZSTD::MaxStrategyLevel() noexcept {
	return strategyMax;
}

template <int a, int b>
constexpr void CheckStrategyLevels() {
	static_assert(
			a == b,
			"You appear to have updated ZSTD. Please update ZSTD::StrategyInfo() "
			"New strategies have been added; read them from the ZSTD header.");
}

const char* ZSTD::StrategyInfo() noexcept {
	CheckStrategyLevels<1, strategyMin>();
	CheckStrategyLevels<9, strategyMax>();
	return "0 (Use whatever ZSTD decides is default)\n"
				 "1 (fast)\n"
				 "2 (dfast)\n"
				 "3 (greedy)\n"
				 "4 (lazy)\n"
				 "5 (lazy2)\n"
				 "6 (btlazy2)\n"
				 "7 (btopt)\n"
				 "8 (btultra)\n"
				 "9 (btultra2)\n";
}

std::pair<const uint8_t*, size_t> ZSTD::Dictionary() const noexcept {
	return {dictionary, dictLen};
}

ZSTD::~ZSTD() noexcept = default;
