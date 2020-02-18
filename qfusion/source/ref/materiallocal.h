#ifndef WSW_MATERIALLOCAL_H
#define WSW_MATERIALLOCAL_H

#include "../gameshared/q_shared.h"
#include "../qcommon/wswstdtypes.h"
#include "../qcommon/qcommon.h"

#include "vattribs.h"
#include "shader.h"
#include "glimp.h"
#include "../game/ai/static_vector.h"

#include <optional>

enum class PassKey {
	RgbGen,
	BlendFunc,
	DepthFunc,
	DepthWrite,
	AlphaFunc,
	TCMod,
	Map,
	AnimMap,
	CubeMap,
	ShadeCubeMap,
	ClampMap,
	AnimClampMap,
	Material,
	Distortion,
	CelShade,
	TCGen,
	AlphaGen,
	Detail,
	Grayscale,
	Skip,
};

enum class Deform {
	Wave = DEFORMV_WAVE,
	Bulge = DEFORMV_BULGE,
	Move = DEFORMV_MOVE,
	Autosprite = DEFORMV_AUTOSPRITE,
	Autosprite2 = DEFORMV_AUTOSPRITE2,
	Autoparticle = DEFORMV_AUTOPARTICLE,
};

enum class Func {
	Sin = SHADER_FUNC_SIN,
	Triangle = SHADER_FUNC_TRIANGLE,
	Square = SHADER_FUNC_SQUARE,
	Sawtooth = SHADER_FUNC_SAWTOOTH,
	InvSawtooth = SHADER_FUNC_INVERSESAWTOOTH,
	Noize = SHADER_FUNC_NOISE,
	DistanceRamp = SHADER_FUNC_RAMP,
};

enum class IntConditionVar {
	MaxTextureSize,
	MaxTextureCubemapSize,
	MaxTextureUnits,
};

enum class BoolConditionVar {
	TextureCubeMap,
	Glsl,
	DeluxeMaps,
	PortalMaps,
};

enum class LogicOp {
	And,
	Or
};

enum class CmpOp {
	LS,
	LE,
	GT,
	GE,
	NE,
	EQ
};

enum class CullMode {
	None,
	Back,
	Front
};

enum class SortMode {
	Portal,
	Sky,
	Opaque,
	Banner,
	Underwater,
	Additive,
	Nearest
};

enum class MaterialKey {
	Cull,
	SkyParams,
	SkyParams2,
	SkyParamsSides,
	FogParams,
	NoMipMaps,
	NoPicMip,
	NoCompress,
	NoFiltering,
	SmallestMipSize,
	PolygonOffset,
	StencilTest,
	Sort,
	DeformVertexes,
	Portal,
	EntityMergable,
	If,
	EndIf,
	OffsetMappingScale,
	GlossExponent,
	GlossIntensity,
	Template,
	Skip,
	SoftParticle,
	ForceWorldOutlines,
};

enum class RgbGen {
	Identity,
	Wave,
	ColorWave,
	Custom,
	CustomWave,
	Entity,
	EntityWave,
	OneMinusEntity,
	Vertex,
	OneMinusVertex,
	LightingDiffuse,
	ExactVertex,
	Const
};

enum class AlphaGen {
	Vertex = ALPHA_GEN_VERTEX,
	OneMinusVertex = ALPHA_GEN_ONE_MINUS_VERTEX,
	Entity = ALPHA_GEN_ENTITY,
	Wave = ALPHA_GEN_WAVE,
	Const = ALPHA_GEN_CONST,
};

enum class SrcBlend {
	Zero = GLSTATE_SRCBLEND_ZERO,
	One = GLSTATE_SRCBLEND_ONE,
	DstColor = GLSTATE_SRCBLEND_DST_COLOR,
	OneMinusDstColor = GLSTATE_SRCBLEND_ONE_MINUS_DST_COLOR,
	SrcAlpha = GLSTATE_SRCBLEND_SRC_ALPHA,
	DstAlpha = GLSTATE_SRCBLEND_DST_ALPHA,
	OneMinusSrcAlpha = GLSTATE_SRCBLEND_ONE_MINUS_SRC_ALPHA,
	OneMinusDstAlpha = GLSTATE_SRCBLEND_ONE_MINUS_DST_ALPHA
};

enum class DstBlend {
	Zero = GLSTATE_DSTBLEND_ZERO,
	One = GLSTATE_DSTBLEND_ONE,
	SrcColor = GLSTATE_DSTBLEND_SRC_COLOR,
	OneMinusSrcColor = GLSTATE_DSTBLEND_ONE_MINUS_SRC_COLOR,
	SrcAlpha = GLSTATE_DSTBLEND_SRC_ALPHA,
	OneMinusSrcAlpha = GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA,
	DstAlpha = GLSTATE_DSTBLEND_DST_ALPHA,
	OneMinusDstAlpha = GLSTATE_DSTBLEND_ONE_MINUS_DST_ALPHA
};

enum class UnaryBlendFunc {
	Blend = GLSTATE_SRCBLEND_SRC_ALPHA | GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA,
	Filter = GLSTATE_SRCBLEND_DST_COLOR | GLSTATE_DSTBLEND_ZERO,
	Add = GLSTATE_SRCBLEND_ONE | GLSTATE_DSTBLEND_ONE
};

enum class AlphaFunc {
	GT0,
	LT128,
	GE128
};

enum class DepthFunc {
	GT,
	EQ
};

enum class TCMod {
	Rotate = TC_MOD_ROTATE,
	Scale = TC_MOD_SCALE,
	Scroll = TC_MOD_SCROLL,
	Stretch = TC_MOD_STRETCH,
	Transform = TC_MOD_TRANSFORM,
	Turb = TC_MOD_TURB
};

enum class TCGen {
	Base = TC_GEN_BASE,
	Lightmap = TC_GEN_LIGHTMAP,
	Environment = TC_GEN_ENVIRONMENT,
	Vector = TC_GEN_VECTOR,
	Reflection = TC_GEN_REFLECTION,
	Celshade = TC_GEN_REFLECTION_CELSHADE,
	Surround = TC_GEN_SURROUND
};

enum class SkySide {
	Right,
	Back,
	Left,
	Front,
	Up,
	Down
};

class TokenSplitter {
	const char *const data;
	size_t dataSize;
	ptrdiff_t offset { 0 };

	std::optional<unsigned> tryMatching1Or2CharsToken( const char *tokenStart ) const;

	[[nodiscard]]
	static bool mustCloseTokenAtChar( char ch, char nextCh );
public:
	TokenSplitter( const char *data_, size_t dataSize_ )
		: data( data_ ), dataSize( dataSize_ ) {}

	[[nodiscard]]
	bool isAtEof() const {
		if( (ptrdiff_t)offset >= dataSize ) {
			return true;
		}
		// Protect against bogus files
		return data[offset] == '\0';
	}

	std::optional<std::pair<uint32_t, uint32_t>> fetchNextTokenInLine();
};

struct TokenSpan {
	int32_t offset;
	uint32_t len;
	uint32_t line;
};

class TokenStream {
	// Initialize just to suppress a lint warning
	const char *data[2] { nullptr, nullptr };

	const TokenSpan *const tokenSpans;
	const int numTokens;

	int currToken { 0 };
	int currLine { 0 };

	wsw::StringView getView( int offset, unsigned len ) {
		// Use either a data or an alt data as a base ptr based on the offset sign
		const char *p = this->data[offset < 0] + std::abs( offset );
		assert( p );
		return wsw::StringView( p, len );
	}
public:
	TokenStream( const char *data_, const TokenSpan *tokenSpans_, const int numTokens_, const char *altData_ = nullptr )
		: tokenSpans( tokenSpans_ ), numTokens( numTokens_ ) {
		data[0] = data_;
		data[1] = altData_;
	}

	[[nodiscard]]
	bool isAtEof() const {
		return currToken >= numTokens;
	}

	[[nodiscard]]
	int getCurrTokenNum() { return currToken; }

	void setCurrTokenNum( int num ) {
		assert( num >= 0 && num <= numTokens );
		currToken = num;
	}

	[[nodiscard]]
	std::optional<wsw::StringView> getNextTokenInLine() {
		if( currToken >= numTokens ) {
			return std::nullopt;
		}
		const auto &[off, len, line] = tokenSpans[currToken];
		if( line != currLine ) {
			return std::nullopt;
		}
		currToken++;
		return std::optional( getView( off, len ) );
	}

	[[nodiscard]]
	std::optional<wsw::StringView> getNextToken() {
		if( currToken >= numTokens ) {
			return std::nullopt;
		}
		const auto &[off, len, line] = tokenSpans[currToken++];
		currLine = line;
		return std::optional( getView( off, len ) );
	}

	bool unGetToken() {
		assert( currToken <= numTokens );
		if( currToken == 0 ) {
			return false;
		}
		currToken = currToken - 1;
		currLine = tokenSpans[currToken].line;
		return true;
	}
};

class MaterialLexer {
	TokenStream *stream { nullptr };

	template <typename T>
	std::optional<T> getNumber() {
		if( auto token = getNextToken() ) {
			// Just to avoid a lint warning
			char tmp = '\0';
			const char *endPtr = &tmp;
			if( auto number = Q_tonum<T>( ( *token ).data(), &endPtr ) ) {
				if( std::isspace( *endPtr ) ) {
					return *number;
				}
			}
			unGetToken();
		}
		return std::nullopt;
	}

	template <typename T>
	T getNumberOr( T defaultValue ) {
		if( auto token = getNextToken() ) {
			// Just to avoid a lint warning
			char tmp = '\0';
			const char *endPtr = &tmp;
			if( auto number = Q_tonum<T>( ( *token ).data(), &endPtr ) ) {
				if( std::isspace( *endPtr ) ) {
					return *number;
				}
			}
		}
		return defaultValue;
	}

	bool parseVector( float *dest, size_t numElems );
	void parseVectorOrFill( float *dest, size_t numElems, float defaultValue );

	std::optional<bool> getSingleCharToken( char ch ) {
		if( auto maybeToken = getNextToken() ) {
			auto token = *maybeToken;
			return std::optional( token.length() == 1 && token[0] == ch );
		}
		return std::nullopt;
	}
public:
	explicit MaterialLexer( TokenStream *tokenStream_ ) : stream( tokenStream_ ) {}

	[[nodiscard]]
	bool isAtEof() const {
		return stream->isAtEof();
	}

	std::optional<wsw::StringView> getNextToken() {
		return stream->getNextToken();
	}

	std::optional<wsw::StringView> getNextTokenInLine() {
		return stream->getNextTokenInLine();
	}

	bool unGetToken() {
		return stream->unGetToken();
	}

	std::optional<bool> getLeftBrace() { return getSingleCharToken( '{' ); }
	std::optional<bool> getRightBrace() { return getSingleCharToken( '}' ); }
	std::optional<bool> getLeftParen() { return getSingleCharToken('(' ); }
	std::optional<bool> getRightParen() { return getSingleCharToken( ')' ); }

	std::optional<PassKey> getPassKey();
	std::optional<Deform> getDeform();
	std::optional<Func> getFunc();
	std::optional<IntConditionVar> getIntConditionVar();
	std::optional<BoolConditionVar> getBoolConditionVar();
	std::optional<LogicOp> getLogicOp();
	std::optional<CmpOp> getCmpOp();
	std::optional<CullMode> getCullMode();
	std::optional<SortMode> getSortMode();
	std::optional<MaterialKey> getMaterialKey();
	std::optional<RgbGen> getRgbGen();
	std::optional<AlphaGen> getAlphaGen();
	std::optional<SrcBlend> getSrcBlend();
	std::optional<DstBlend> getDstBlend();
	std::optional<UnaryBlendFunc> getUnaryBlendFunc();
	std::optional<AlphaFunc> getAlphaFunc();
	std::optional<DepthFunc> getDepthFunc();
	std::optional<TCMod> getTCMod();
	std::optional<TCGen> getTCGen();
	std::optional<SkySide> getSkySide();

	bool skipToEndOfLine();

	std::optional<float> getFloat() { return getNumber<float>(); }
	std::optional<int> getInt() { return getNumber<int>(); }

	float getFloatOr( float defaultValue ) { return getNumberOr<float>( defaultValue ); }
	int getIntOr( int defaultValue ) { return getNumberOr<int>( defaultValue ); }

	template <size_t N>
	bool getVector( float *dest ) {
		static_assert( N && N <= 8 );
		if constexpr( N == 1 ) {
			if( auto number = getFloat() ) {
				*dest = *number;
				return true;
			}
			return false;
		}
		// Make sure it rolls back offset properly on failure like everything else does
		const auto oldTokenNum = stream->getCurrTokenNum();
		if( parseVector( dest, N ) ) {
			return true;
		}
		stream->setCurrTokenNum( oldTokenNum );
		return false;
	}

	template <size_t N>
	void getVectorOrFill( float *dest, float defaultValue ) {
		static_assert( N && N < 8 );
		if constexpr( N == 1 ) {
			*dest = getFloatOr( defaultValue );
		} else {
			parseVectorOrFill( dest, N, defaultValue );
		}
	}

	std::optional<bool> getBool();
};

#include <vector>

class MemSpecBuilder {
	size_t size { 0 };
public:
	template <typename T>
	class Spec {
		friend class MemSpecBuilder;
		size_t offset;
	public:
		T *get( void *base ) const {
			auto *p = (uint8_t *)base;
			p += offset;
			assert( !( (uintptr_t)p % alignof( T ) ) );
			return (T *)p;
		}
	};

	template <typename T>
	Spec<T> add() {
		return add<T>( 1 );
	}

	template <typename T>
	Spec<T> add( size_t numElems ) {
		if( size % alignof( T ) ) {
			size += alignof( T ) - size % alignof( T );
		}
		Spec<T> result;
		result.offset = size;
		size += sizeof( T ) * numElems;
		return result;
	}

	size_t sizeSoFar() { return size; }
};

struct PlaceholderSpan {
	uint32_t tokenNum;
	uint16_t offset;
	uint8_t len;
	uint8_t argNum;
};

class MaterialCache {
	friend class MaterialParser;

	struct FileCache {
		FileCache *next { nullptr };
		const char *data { nullptr };
		size_t dataSize { 0 };
		TokenSpan *spans { nullptr };
		unsigned numSpans { 0 };
	};

	FileCache *fileCacheHead { nullptr };

	struct Source {
		using Placeholders = std::vector<PlaceholderSpan>;

		std::optional<Placeholders> maybePlaceholders;

		Source *nextInList { nullptr };
		Source *nextInBin { nullptr };

		wsw::HashedStringView name;

		const Source *firstInSameMemChunk { nullptr };
		const FileCache *file { nullptr };
		unsigned tokenSpansOffset { ~0u };
		unsigned numTokens { ~0u };

		std::optional<Placeholders> preparePlaceholders();

		void findPlaceholdersInToken( const wsw::StringView &token, int tokenNum, std::vector<PlaceholderSpan> &spans );

		bool expandTemplate( const wsw::StringView *args, size_t numArgs,
							 wsw::String &expansionBuffer,
			                 std::vector<TokenSpan> &resultingTokens );
	};

	enum { kNumBins = 307 };

	Source *sourcesHead { nullptr };
	Source *sourceBins[kNumBins] { nullptr };

	shader_t *materialsHead { nullptr };
	shader_t *materialBins[kNumBins] { nullptr };

	shader_t *materialById[MAX_SHADERS] { nullptr };

	wsw::String pathNameBuffer;
	wsw::String cleanNameBuffer;
	wsw::String expansionBuffer;
	wsw::String fileContentsBuffer;

	std::vector<TokenSpan> fileTokenSpans;
	std::vector<TokenSpan> templateTokenSpans;

	std::vector<wsw::StringView> fileMaterialNames;
	std::vector<std::pair<unsigned, unsigned>> fileSourceSpans;

	std::vector<uint16_t> freeMaterialIds;

	StaticVector<TokenStream, 1> templateTokenStreamHolder;
	StaticVector<MaterialLexer, 1> templateLexerHolder;
	StaticVector<TokenStream, 1> primaryTokenStreamHolder;

	FileCache *createFileCache( const char *filename );
	const wsw::String *readFileContents( const char *filename );

	Source *findSourceByName( const wsw::StringView &name ) {
		return findSourceByName( wsw::HashedStringView( name ) );
	}

	Source *findSourceByName( const wsw::HashedStringView &name );

	image_s *findImage( const wsw::StringView &name, int flags, int imageTags, int minMipSize = 1 );
	void loadMaterial( image_s **images, const wsw::StringView &fullName, int flags, int imageTags, int minMipSize = 1 );

	void loadDirContents( const char *dir );

	void addFileContents( const char *filename );
	bool tryAddingFileCacheContents( const FileCache *fileCache );

	void unlinkAndFree( shader_t *s );

	unsigned getNextMaterialId();

	wsw::HashedStringView makeCleanName( const wsw::StringView &name );

	TokenStream *getTokenStreamForShader( const wsw::HashedStringView &cleanName );

	shader_t *loadMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name, int type, TokenStream *tokenStream );

	// This must go once sane material classes get implemented
	shader_t *initMaterial( int type, const wsw::HashedStringView &cleanName, MemSpecBuilder memSpec );

	shader_t *newDefaultMaterial( int type, const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newDefaultVertexMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newDefaultDeluxeMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newDefaultCoronaMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newDefaultDiffuseMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newDefault2DLikeMaterial( int type, const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newOpaqueEnvMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newSkyBoxMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name );
	shader_t *newFogMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name );
public:
	MaterialCache();
	~MaterialCache();

	static void init();
	static void shutdown();
	static MaterialCache *instance();

	void freeUnusedMaterialsByType( const shaderType_e *types, unsigned numTypes );

	void freeUnusedMaterials() {
		freeUnusedMaterialsByType( nullptr, 0 );
	}

	void touchMaterialsByName( const wsw::StringView &name );

	shader_t *getMaterialById( int id ) {
		return materialById[id];
	}

	MaterialLexer *expandTemplate( const wsw::StringView &name, const wsw::StringView *args, size_t numArgs );

	shader_t *loadMaterial( const wsw::StringView &name, int type, bool forceDefault, image_s *defaultImage );

	shader_t *loadDefaultMaterial( const wsw::StringView &name, int type );
};

struct shader_s;
struct shaderpass_s;
struct shaderfunc_s;
struct image_s;

class MaterialParser {
	MaterialCache *const materialCache;
	MaterialLexer defaultLexer;
	MaterialLexer *lexer;

	const wsw::StringView name;
	const wsw::HashedStringView cleanName;

	StaticVector<int, 256> deformSig;
	StaticVector<shaderpass_t, MAX_SHADER_PASSES> passes;
	StaticVector<deformv_t, MAX_SHADER_DEFORMVS> deforms;
	StaticVector<tcmod_t, MAX_SHADER_PASSES * MAX_SHADER_TCMODS> tcMods;

	int sort { 0 };
	int flags { 0 };
	shaderType_e type { (shaderType_e)0 };

	std::optional<int> minMipSize;

	uint8_t fog_color[4] { 0, 0, 0, 0 };
	float fog_dist { 0.0f };
	float fog_clearDist { 0.0f };

	float glossIntensity { 0.0f };
	float glossExponent { 0.0f };
	float offsetMappingScale { 0.0f };

	float portalDistance { 0.0f };

	int imageTags { 0 };

	bool noPicMip { false };
	bool noMipMaps { false };
	bool noCompress { false };
	bool noFiltering { false };

	bool hasLightmapPass { false };

	shaderpass_t *currPass() {
		assert( !passes.empty() );
		return &passes.back();
	}

	tcmod_t *tryAddingPassTCMod();
	deformv_t *tryAddingDeform();

	bool parsePass();
	bool parsePassKey();
	bool parseKey();

	bool parseRgbGen();
	bool parseBlendFunc();
	bool parseDepthFunc();
	bool parseDepthWrite();
	bool parseAlphaFunc();
	bool parseTCMod();
	bool parseMap();
	bool parseAnimMap();
	bool parseCubeMap();
	bool parseShadeCubeMap();
	bool parseSurroundMap();
	bool parseClampMap();
	bool parseAnimClampMap();
	bool parseMaterial();
	bool parseDistortion();
	bool parseCelshade();
	bool parseTCGen();
	bool parseAlphaGen();
	bool parseDetail();
	bool parseGrayscale();
	bool parseSkip();

	bool parseAlphaGenPortal();

	bool parseMapExt( int addFlags );
	bool tryMatchingPortalMap( const wsw::StringView &texNameToken );
	bool tryMatchingLightMap( const wsw::StringView &texNameToken );

	bool parseAnimMapExt( int addFlags );
	bool parseCubeMapExt( int addFlags, int tcGen );

	bool parseCull();
	bool parseSkyParms();
	bool parseSkyParms2();
	bool parseSkyParmsSides();
	bool parseFogParams();
	bool parseNoMipmaps();
	bool parseNoPicmip();
	bool parseNoCompress();
	bool parseNofiltering();
	bool parseSmallestMipSize();
	bool parsePolygonOffset();
	bool parseStencilTest();
	bool parseEntityMergable();
	bool parseSort();
	bool parseDeformVertexes();
	bool parsePortal();
	bool parseIf();
	bool parseOffsetMappingScale();
	bool parseGlossExponent();
	bool parseGlossIntensity();
	bool parseTemplate();
	bool parseSoftParticle();
	bool parseForceWorldOutlines();

	std::optional<bool> parseCondition();
	void skipConditionBlock();
	static int getIntConditionVarValue( IntConditionVar var );
	static bool getBoolConditionVarValue( BoolConditionVar var );

	bool parseDeformWave();
	bool parseDeformBulge();
	bool parseDeformMove();

	[[nodiscard]]
	bool parseFunc( shaderfunc_s *func );

	template <typename... Args>
	[[nodiscard]]
	bool addToDeformSignature( Args ... args ) {
		return _addToDeformSignature( args... );
	}

	template <typename... Args>
	[[nodiscard]]
	bool _addToDeformSignature( int arg, Args... rest ) {
		return tryAddingToSignature( arg ) && _addToDeformSignature( rest... );
	}

	template <typename... Args>
	[[nodiscard]]
	bool _addToDeformSignature( unsigned arg, Args... rest ) {
		return tryAddingToSignature( (int)arg ) && _addToDeformSignature( rest... );
	}

	template <typename... Args>
	[[nodiscard]]
	bool _addToDeformSignature( float arg, Args... rest ) {
		union { float f; int32_t i; } u;
		u.f = arg;
		return tryAddingToSignature( u.i ) && _addToDeformSignature( rest... );
	}

	[[nodiscard]]
	bool tryAddingToSignature( int value ) {
		if( deformSig.size() != deformSig.capacity() ) {
			deformSig.push_back( value );
			return true;
		}
		return false;
	}

	[[nodiscard]]
	static bool _addToDeformSignature() { return true; }

	int getImageFlags();

	image_s *findImage( const wsw::StringView &name_, int flags_ ) {
		return materialCache->findImage( name_, flags_, imageTags, minMipSize.value_or( 1 ) );
	}

	void fixLightmapsForVertexLight();
	void fixFlagsAndSortingOrder();

	shader_t *build();

	int buildVertexAttribs();
	static int getDeformVertexAttribs( const deformv_t &deform );
	static int getPassVertexAttribs( const shaderpass_t &pass );
	static int getRgbGenVertexAttribs( const shaderpass_t &pass, const colorgen_t &gen );
	static int getAlphaGenVertexAttribs( const colorgen_t &gen );
	static int getTCGenVertexAttribs( unsigned gen );
public:
	MaterialParser( MaterialCache *materialCache_,
					TokenStream *mainTokenStream_,
					const wsw::StringView &name_,
					const wsw::HashedStringView &cleanName_,
					shaderType_e type_ );

    shader_t *exec();
};

const wsw::StringView kNormSuffix( "_norm" );
const wsw::StringView kGlossSuffix( "_gloss" );
const wsw::StringView kDecalSuffix( "_decal" );
const wsw::StringView kAddSuffix( "_add" );

#endif
