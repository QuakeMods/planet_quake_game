#include "materiallocal.h"
#include "local.h"
#include "shader.h"

#include "../qcommon/hash.h"
#include "../game/ai/static_vector.h"

static bool isANumber( const wsw::StringView &view ) {
	for( char ch: view ) {
		if( (unsigned)( ch - '0' ) > 9 ) {
			return false;
		}
	}
	return true;
}

static inline bool isAPlaceholder( const wsw::StringView &view ) {
	return view.length() == 1 && view[0] == '-';
}

tcmod_t *MaterialParser::tryAddingPassTCMod() {
	auto *const pass = currPass();
	if( pass->numtcmods == MAX_SHADER_TCMODS ) {
		return nullptr;
	}
	if( tcMods.size() == tcMods.capacity() ) {
		return nullptr;
	}
	// Set the tcmod data pointer if it has not been set yet
	if( !pass->numtcmods ) {
		pass->tcmods = tcMods.end();
	}
	pass->numtcmods++;
	auto *newMod = tcMods.unsafe_grow_back();
	memset( newMod, 0, sizeof( tcmod_t ) );
	return newMod;
}

deformv_t *MaterialParser::tryAddingDeform() {
	if( deforms.size() == deforms.capacity() ) {
		return nullptr;
	}
	auto *deform = deforms.unsafe_grow_back();
	memset( deform, 0, sizeof( deformv_t ) );
	return deform;
}

MaterialParser::MaterialParser( MaterialCache *materialCache_,
								TokenStream *mainTokenStream_,
								const wsw::StringView &name_,
								const wsw::HashedStringView &cleanName_,
								shaderType_e type_ )
	: materialCache( materialCache_ )
	, defaultLexer( mainTokenStream_ )
	, lexer( &defaultLexer )
	, name( name_ )
	, cleanName( cleanName_ )
	, type( type_ ) {}

shader_t *MaterialParser::exec() {
	for(;;) {
		auto maybeToken = lexer->getNextToken();
		if( !maybeToken ) {
			break;
		}

		const wsw::StringView token = *maybeToken;
		// Check for a pass or a key
		// TODO: There should be a routine that allows a direct comparison with chars
		if( !token.equals( wsw::StringView( "{" ) ) ) {
			lexer->unGetToken();
			if( !parseKey() ) {
				Com_Printf( "Failed to parse a key\n" );
				return nullptr;
			}

			// Skip any trailing stuff at the end of the parsed key
			lexer->skipToEndOfLine();
			continue;
		}

		if( passes.size() == passes.capacity() ) {
			Com_Printf( S_COLOR_YELLOW "Too many passes\n" );
			return nullptr;
		}

		auto *newPass = passes.unsafe_grow_back();
		memset( newPass, 0, sizeof( shaderpass_t ) );

		if( !parsePass() ) {
			Com_Printf( S_COLOR_YELLOW "Failed to parse a pass\n" );
			return nullptr;
		}

		auto maybeNextToken = lexer->getNextToken();
		if( !maybeNextToken ) {
			Com_Printf( S_COLOR_YELLOW "Missing a closing brace of a pass\n" );
			return nullptr;
		}

		auto nextToken = *maybeNextToken;
		if( !nextToken.equals( wsw::StringView( "}" ) ) ) {
			Com_Printf( S_COLOR_YELLOW "Missing a closing brace of a pass\n" );
			return nullptr;
		}

		const auto blendMask = ( newPass->flags & GLSTATE_BLEND_MASK );

		if( newPass->rgbgen.type == RGB_GEN_UNKNOWN ) {
			newPass->rgbgen.type = RGB_GEN_IDENTITY;
		}

		if( newPass->alphagen.type == ALPHA_GEN_UNKNOWN ) {
			auto rgbGenType = newPass->rgbgen.type;
			if( rgbGenType == RGB_GEN_VERTEX || rgbGenType == RGB_GEN_EXACT_VERTEX ) {
				newPass->alphagen.type = ALPHA_GEN_VERTEX;
			} else {
				newPass->alphagen.type = ALPHA_GEN_IDENTITY;
			}
		}

		if( blendMask == ( GLSTATE_SRCBLEND_ONE | GLSTATE_DSTBLEND_ZERO ) ) {
			newPass->flags &= ~blendMask;
		}
	}

	return build();
}

bool MaterialParser::parsePass() {
	for(;;) {
		const auto maybeToken = lexer->getNextToken();
		if( !maybeToken ) {
			Com_Printf( S_COLOR_YELLOW "Failed to get a next token in a pass\n" );
			return false;
		}

		const wsw::StringView token = *maybeToken;
		lexer->unGetToken();

		// Protect from nested passes
		if( token.equals( wsw::StringView( "{" ) ) ) {
			return false;
		}

		// Return having met a closing brace so
		if( token.equals( wsw::StringView( "}" ) ) ) {
			return true;
		}

		if( !parsePassKey() ) {
			Com_Printf( "Failed to parse a key\n" );
			return false;
		}

		// Skip any trailing stuff at the end of the parsed key
		lexer->skipToEndOfLine();
	}
}

bool MaterialParser::parsePassKey() {
	std::optional<PassKey> maybeKey = lexer->getPassKey();
	if( !maybeKey ) {
		return true;
	}

	switch( *maybeKey ) {
		case PassKey::RgbGen:
			return parseRgbGen();
		case PassKey::BlendFunc:
			return parseBlendFunc();
		case PassKey::DepthFunc:
			return parseDepthFunc();
		case PassKey::DepthWrite:
			return parseDepthWrite();
		case PassKey::AlphaFunc:
			return parseAlphaFunc();
		case PassKey::TCMod:
			return parseTCMod();
		case PassKey::Map:
			return parseMap();
		case PassKey::AnimMap:
			return parseAnimMap();
		case PassKey::CubeMap:
			return parseCubeMap();
		case PassKey::ShadeCubeMap:
			return parseShadeCubeMap();
		case PassKey::ClampMap:
			return parseClampMap();
		case PassKey::AnimClampMap:
			return parseAnimClampMap();
		case PassKey::Material:
			return parseMaterial();
		case PassKey::Distortion:
			return parseDistortion();
		case PassKey::CelShade:
			return parseCelshade();
		case PassKey::TCGen:
			return parseTCGen();
		case PassKey::AlphaGen:
			return parseAlphaGen();
		case PassKey::Detail:
			return parseDetail();
		case PassKey::Grayscale:
			return parseGrayscale();
		case PassKey::Skip:
			return parseSkip();
	}
}

bool MaterialParser::parseKey() {
	std::optional<MaterialKey> maybeKey = lexer->getMaterialKey();
	if( !maybeKey ) {
		return true;
	}

	switch( *maybeKey ) {
		case MaterialKey::Cull:
			return parseCull();
		case MaterialKey::SkyParams:
			return parseSkyParms();
		case MaterialKey::SkyParams2:
			return parseSkyParms2();
		case MaterialKey::SkyParamsSides:
			return parseSkyParmsSides();
		case MaterialKey::FogParams:
			return parseFogParams();
		case MaterialKey::NoMipMaps:
			return parseNoMipmaps();
		case MaterialKey::NoPicMip:
			return parseNoPicmip();
		case MaterialKey::NoCompress:
			return parseNoCompress();
		case MaterialKey::NoFiltering:
			return parseNofiltering();
		case MaterialKey::SmallestMipSize:
			return parseSmallestMipSize();
		case MaterialKey::PolygonOffset:
			return parsePolygonOffset();
		case MaterialKey::StencilTest:
			return parseStencilTest();
		case MaterialKey::Sort:
			return parseSort();
		case MaterialKey::DeformVertexes:
			return parseDeformVertexes();
		case MaterialKey::Portal:
			return parsePortal();
		case MaterialKey::EntityMergable:
			return parseEntityMergable();
		case MaterialKey::If:
			return parseIf();
		case MaterialKey::EndIf:
			return true;
		case MaterialKey::OffsetMappingScale:
			return parseOffsetMappingScale();
		case MaterialKey::GlossExponent:
			return parseGlossExponent();
		case MaterialKey::GlossIntensity:
			return parseGlossIntensity();
		case MaterialKey::Template:
			return parseTemplate();
		case MaterialKey::Skip:
			return parseSkip();
		case MaterialKey::SoftParticle:
			return parseSoftParticle();
		case MaterialKey::ForceWorldOutlines:
			return parseForceWorldOutlines();
	}
}

bool MaterialParser::parseRgbGen() {
	std::optional<RgbGen> maybeRgbGen = lexer->getRgbGen();
	// TODO: We handle this at loop level, do we?
	if( !maybeRgbGen ) {
		return false;
	}

	auto *const pass = currPass();
	switch( *maybeRgbGen ) {
		case RgbGen::Identity:
			pass->rgbgen.type = RGB_GEN_IDENTITY;
			break;
		case RgbGen::Wave:
			pass->rgbgen.type = RGB_GEN_WAVE;
			VectorSet( pass->rgbgen.args, 1.0f, 1.0f, 1.0f );
			if( !parseFunc( &pass->rgbgen.func ) ) {
			    return false;
			}
			break;
		case RgbGen::ColorWave:
			pass->rgbgen.type = RGB_GEN_WAVE;
			lexer->getVector<3>( pass->rgbgen.args );
			if( !parseFunc( &pass->rgbgen.func ) ) {
			    return false;
			}
			break;
		case RgbGen::Custom:
			pass->rgbgen.type = RGB_GEN_CUSTOMWAVE;
			pass->rgbgen.args[0] = lexer->getFloat().value_or(0);
			if( pass->rgbgen.args[0] < 0 || pass->rgbgen.args[0] >= NUM_CUSTOMCOLORS ) {
				pass->rgbgen.args[0] = 0;
			}
			pass->rgbgen.func.type = SHADER_FUNC_NONE;
			break;
		case RgbGen::CustomWave:
			pass->rgbgen.type = RGB_GEN_CUSTOMWAVE;
			pass->rgbgen.args[0] = lexer->getFloat().value_or(0);
			if( pass->rgbgen.args[0] < 0 || pass->rgbgen.args[0] >= NUM_CUSTOMCOLORS ) {
				pass->rgbgen.args[0] = 0;
			}
			pass->rgbgen.func.type = SHADER_FUNC_NONE;
			return parseFunc( &pass->rgbgen.func );
		case RgbGen::Entity:
			pass->rgbgen.type = RGB_GEN_ENTITYWAVE;
			pass->rgbgen.func.type = SHADER_FUNC_NONE;
			break;
		case RgbGen::EntityWave:
			pass->rgbgen.type = RGB_GEN_ENTITYWAVE;
			pass->rgbgen.func.type = SHADER_FUNC_NONE;
			if( !lexer->getVector<3>( pass->rgbgen.args ) ) {
			    return false;
			}
			return parseFunc( &pass->rgbgen.func );
		case RgbGen::OneMinusEntity:
			pass->rgbgen.type = RGB_GEN_ONE_MINUS_ENTITY;
			break;
		case RgbGen::Vertex:
			pass->rgbgen.type = RGB_GEN_VERTEX;
			break;
		case RgbGen::OneMinusVertex:
			pass->rgbgen.type = RGB_GEN_ONE_MINUS_VERTEX;
			break;
		case RgbGen::LightingDiffuse:
			if( type < SHADER_TYPE_DIFFUSE ) {
				pass->rgbgen.type = RGB_GEN_VERTEX;
			} else if( type > SHADER_TYPE_DIFFUSE ) {
				pass->rgbgen.type = RGB_GEN_IDENTITY;
			} else {
				pass->rgbgen.type = RGB_GEN_LIGHTING_DIFFUSE;
			}
			break;
		case RgbGen::ExactVertex:
			pass->rgbgen.type = RGB_GEN_EXACT_VERTEX;
			break;
		case RgbGen::Const:
			pass->rgbgen.type = RGB_GEN_CONST;
			vec3_t color;
			if( !lexer->getVector<3>( color ) ) {
			    return false;
			}
			ColorNormalize( color, pass->rgbgen.args );
			break;
	}

	return true;
}

bool MaterialParser::parseBlendFunc() {
	auto *const pass = currPass();

	pass->flags &= ~GLSTATE_BLEND_MASK;
	if( std::optional<UnaryBlendFunc> maybeUnaryFunc = lexer->getUnaryBlendFunc() ) {
		pass->flags |= (int)( *maybeUnaryFunc );
		return true;
	}

	if( std::optional<SrcBlend> srcBlend = lexer->getSrcBlend() ) {
		if( std::optional<DstBlend> dstBlend = lexer->getDstBlend() ) {
			pass->flags |= (int)( *srcBlend ) | (int)( *dstBlend );
			return true;
		}
	}

	return false;
}

bool MaterialParser::parseDepthFunc() {
	std::optional<DepthFunc> maybeFunc = lexer->getDepthFunc();
	if( !maybeFunc ) {
		return false;
	}

	auto *const pass = currPass();
	pass->flags &= ~(GLSTATE_DEPTHFUNC_EQ|GLSTATE_DEPTHFUNC_GT);
	if( *maybeFunc == DepthFunc::EQ ) {
		pass->flags |= GLSTATE_DEPTHFUNC_EQ;
	} else if( *maybeFunc == DepthFunc::GT ) {
		pass->flags |= GLSTATE_DEPTHFUNC_GT;
	}

	return true;
}

bool MaterialParser::parseDepthWrite() {
	currPass()->flags |= GLSTATE_DEPTHWRITE;
	return true;
}

bool MaterialParser::parseAlphaFunc() {
	auto *pass = currPass();

	pass->flags &= ~( SHADERPASS_ALPHAFUNC | GLSTATE_ALPHATEST );

	std::optional<AlphaFunc> maybeFunc = lexer->getAlphaFunc();
	if( !maybeFunc ) {
		return false;
	}
	switch( *maybeFunc ) {
		// TODO...
		case AlphaFunc::GT0:
			pass->flags |= SHADERPASS_AFUNC_GT0;
			break;
		case AlphaFunc::LT128:
			pass->flags |= SHADERPASS_AFUNC_LT128;
			break;
		case AlphaFunc::GE128:
			pass->flags |= SHADERPASS_AFUNC_GE128;
			break;
	}

	if( pass->flags & SHADERPASS_ALPHAFUNC ) {
		pass->flags |= GLSTATE_ALPHATEST;
	}

	return true;
}

bool MaterialParser::parseTCMod() {
	std::optional<TCMod> maybeTCMod = lexer->getTCMod();
	if( !maybeTCMod ) {
		return false;
	}

	TCMod tcMod = *maybeTCMod;
	if( tcMod == TCMod::Rotate ) {
		auto maybeArg = lexer->getFloat();
		if( !maybeArg ) {
			return false;
		}
		auto *newMod = tryAddingPassTCMod();
		if( !newMod ) {
			return false;
		}
		newMod->args[0] = -( *maybeArg ) / 360.0f;
		return true;
	}

	auto *const newMod = tryAddingPassTCMod();
	if( !newMod ) {
		return false;
	}

	if( tcMod == TCMod::Scale || tcMod == TCMod::Scroll ) {
		return lexer->getVector<2>( newMod->args );
	}

	if( tcMod == TCMod::Stretch ) {
		shaderfunc_t func;
		if( !parseFunc( &func ) ) {
			return false;
		}

		newMod->args[0] = func.type;
		for( int i = 1; i < 5; i++ ) {
            newMod->args[i] = func.args[i - 1];
        }

		return true;
	}

	if( tcMod == TCMod::Turb ) {
		return lexer->getVector<4>( newMod->args );
	}

	assert( tcMod == TCMod::Transform );
	if( !lexer->getVector<6>( newMod->args ) ) {
	    return false;
	}

	newMod->args[4] = newMod->args[4] - floor( newMod->args[4] );
	newMod->args[5] = newMod->args[5] - floor( newMod->args[5] );
	return true;
}

bool MaterialParser::parseMap() {
	return parseMapExt( 0 );
}

bool MaterialParser::parseAnimMap() {
	return parseAnimMapExt( 0 );
}

static const wsw::StringView kLightmap( "$lightmap" );
static const wsw::StringView kPortalMap( "$portalmap" );
static const wsw::StringView kMirrorMap( "$mirrormap" );

bool MaterialParser::tryMatchingLightMap( const wsw::StringView &texNameToken ) {
	if( !kLightmap.equalsIgnoreCase( texNameToken ) ) {
		return false;
	}

	auto *pass = currPass();
	pass->tcgen = TC_GEN_LIGHTMAP;
	pass->flags = ( pass->flags & ~( SHADERPASS_PORTALMAP ) ) | SHADERPASS_LIGHTMAP;
	pass->anim_fps = 0;
	pass->images[0] = nullptr;
	return true;
}

bool MaterialParser::tryMatchingPortalMap( const wsw::StringView &texNameToken ) {
	if( !kPortalMap.equalsIgnoreCase( texNameToken ) && !kMirrorMap.equalsIgnoreCase( texNameToken ) ) {
		return false;
	}

	auto *pass = currPass();
	pass->tcgen = TC_GEN_PROJECTION;
	pass->flags = ( pass->flags & ~( SHADERPASS_LIGHTMAP ) ) | SHADERPASS_PORTALMAP;
	pass->anim_fps = 0;
	pass->images[0] = nullptr;
	if( ( flags & SHADER_PORTAL ) && ( sort == SHADER_SORT_PORTAL ) ) {
		sort = 0; // reset sorting so we can figure it out later. FIXME?
	}

	flags |= SHADER_PORTAL | ( r_portalmaps->integer ? SHADER_PORTAL_CAPTURE : 0 );
	return true;
}

bool MaterialParser::parseMapExt( int addFlags ) {
	const std::optional<wsw::StringView> maybeToken = lexer->getNextTokenInLine();
	if( !maybeToken ) {
		return false;
	}

	const auto token = *maybeToken;
	if( tryMatchingLightMap( token ) ) {
		return true;
	}
	if( tryMatchingPortalMap( token ) ) {
		return true;
	}

	auto *const pass = currPass();
	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP | SHADERPASS_PORTALMAP );
	pass->anim_fps = 0;
	pass->images[0] = findImage( token, getImageFlags() | addFlags | IT_SRGB );
	return true;
}

bool MaterialParser::parseAnimMapExt( int addFlags ) {
	const auto imageFlags = getImageFlags() | addFlags | IT_SRGB;

	auto *const pass = currPass();
	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP | SHADERPASS_PORTALMAP );
	// TODO: Disallow?
	pass->anim_fps = lexer->getFloatOr( 0.0f );
	pass->anim_numframes = 0;

	for(;; ) {
		if( auto maybeToken = lexer->getNextTokenInLine() ) {
			if( pass->anim_numframes < MAX_SHADER_IMAGES ) {
				pass->images[pass->anim_numframes++] = findImage( *maybeToken, imageFlags );
			}
		} else {
			break;
		}
	}

	// TODO: Disallow?
	if( pass->anim_numframes == 0 ) {
		pass->anim_fps = 0;
	}

	return true;
}

bool MaterialParser::parseCubeMapExt( int addFlags, int tcGen ) {
	auto *const pass = currPass();
	if( auto maybeToken = lexer->getNextTokenInLine() ) {
		const auto imageFlags = getImageFlags() | addFlags | IT_SRGB | IT_CUBEMAP;
		pass->images[0] = R_FindImage( *maybeToken, wsw::StringView(), imageFlags, *minMipSize, imageTags );
	}

	pass->anim_fps = 0;
	pass->flags &= ~( SHADERPASS_LIGHTMAP | SHADERPASS_PORTALMAP );

	if( pass->images[0] ) {
		pass->tcgen = tcGen;
		return true;
	}

	// TODO Use the newly added facilities
	//Com_DPrintf( S_COLOR_YELLOW "Shader %s has a stage with no image: %s\n", shader->name, token );
	pass->images[0] = rsh.noTexture;
	pass->tcgen = TC_GEN_BASE;

	return true;
}

bool MaterialParser::parseCubeMap() {
	return parseCubeMapExt( IT_CLAMP, TC_GEN_REFLECTION );
}

bool MaterialParser::parseShadeCubeMap() {
	return parseCubeMapExt( IT_CLAMP, TC_GEN_REFLECTION_CELSHADE );
}

bool MaterialParser::parseSurroundMap() {
	return parseCubeMapExt( IT_CLAMP, TC_GEN_SURROUND );
}

bool MaterialParser::parseClampMap() {
	return parseAnimMapExt( IT_CLAMP );
}

bool MaterialParser::parseAnimClampMap() {
	return parseAnimMapExt( IT_CLAMP );
}

bool MaterialParser::parseMaterial() {
	auto *const pass = currPass();

	const auto imageFlags = getImageFlags();
	auto maybeFirstToken = lexer->getNextTokenInLine();
	const auto &firstToken = maybeFirstToken ? *maybeFirstToken : this->name;

	pass->images[0] = findImage( firstToken, imageFlags | IT_SRGB );
	if( !pass->images[0] ) {
		//Com_DPrintf( S_COLOR_YELLOW "WARNING: failed to load base/diffuse image for material %s in shader %s.\n", token, shader->name );
		return false;
	}

	pass->images[1] = pass->images[2] = pass->images[3] = nullptr;

	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP | SHADERPASS_PORTALMAP );
	if( pass->rgbgen.type == RGB_GEN_UNKNOWN ) {
		pass->rgbgen.type = RGB_GEN_IDENTITY;
	}

	// I assume materials are only applied to lightmapped surfaces

	for(;;) {
		const auto maybeToken = lexer->getNextTokenInLine();
		if( !maybeToken ) {
			break;
		}

		auto token = *maybeToken;
		if( isANumber( token ) ) {
			continue;
		}

		if( !pass->images[1] ) {
			pass->images[1] = findImage( token, imageFlags | IT_NORMALMAP );
			pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
		} else if( !pass->images[2] ) {
			if( !isAPlaceholder( token ) && r_lighting_specular->integer ) {
				pass->images[2] = findImage( token, imageFlags );
			} else {
				// set gloss to rsh.blackTexture so we know we have already parsed the gloss image
				pass->images[2] = rsh.blackTexture;
			}
		} else {
			// parse decal images
			for( int i = 3; i < 5; i++ ) {
				if( pass->images[i] ) {
					continue;
				}
				if( !isAPlaceholder( token ) ) {
					pass->images[i] = findImage( token, imageFlags | IT_SRGB );
				} else {
					pass->images[i] = rsh.whiteTexture;
				}
			}
		}
	}

	// black texture => no gloss, so don't waste time in the GLSL program
	if( pass->images[2] == rsh.blackTexture ) {
		pass->images[2] = nullptr;
	}

	for( int i = 3; i < 5; i++ ) {
		if( pass->images[i] == rsh.whiteTexture ) {
			pass->images[i] = nullptr;
		}
	}

	if( pass->images[1] ) {
		return true;
	}

	// load default images
	pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;

	pass->images[1] = pass->images[2] = pass->images[3] = nullptr;

	// set defaults
	const auto mipSize = minMipSize.value_or( 1 );

	// load normalmap image
	pass->images[1] = R_FindImage( name, kNormSuffix, ( imageFlags | IT_NORMALMAP ), mipSize, imageTags );

	// load glossmap image
	if( r_lighting_specular->integer ) {
		pass->images[2] = R_FindImage( name, kGlossSuffix, flags, mipSize, imageTags );
	}

	if( !( pass->images[2] = R_FindImage( name, kDecalSuffix, flags, mipSize, imageTags ) ) ) {
		pass->images[2] = R_FindImage( name, kAddSuffix, flags, mipSize, imageTags );
	}

	return true;
}

bool MaterialParser::parseDistortion() {
	if( !r_portalmaps->integer ) {
	    // TODO:....
		//Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a distortion stage, while GLSL is not supported\n", shader->name );
		lexer->skipToEndOfLine();
		return true;
	}

	auto *const pass = currPass();

	const auto imageFlags = getImageFlags();
	pass->flags &= ~( SHADERPASS_LIGHTMAP | SHADERPASS_PORTALMAP );
	pass->images[0] = pass->images[1] = nullptr;

	for(;;) {
		auto maybeToken = lexer->getNextTokenInLine();
		if( !maybeToken ) {
			break;
		}

		auto token = *maybeToken;
		if( isANumber( token ) ) {
			continue;
		}

		if( !pass->images[0] ) {
			pass->images[0] = findImage( token, imageFlags );
			pass->program_type = GLSL_PROGRAM_TYPE_DISTORTION;
		} else {
			pass->images[1] = findImage( token, imageFlags );
			// TODO: Interrupt at this, skip/print warning?
		}
	}

	if( pass->rgbgen.type == RGB_GEN_UNKNOWN ) {
		pass->rgbgen.type = RGB_GEN_CONST;
		VectorClear( pass->rgbgen.args );
	}

	if( sort == SHADER_SORT_PORTAL ) {
		sort = 0; // reset sorting so we can figure it out later. FIXME?
	}

	flags |= SHADER_PORTAL | SHADER_PORTAL_CAPTURE | SHADER_PORTAL_CAPTURE2;
	return true;
}

bool MaterialParser::parseCelshade() {
	auto *const pass = currPass();

	const auto imageFlags = getImageFlags() | IT_SRGB;
	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP | SHADERPASS_PORTALMAP );
	if( pass->rgbgen.type == RGB_GEN_UNKNOWN ) {
		pass->rgbgen.type = RGB_GEN_IDENTITY;
	}

	pass->anim_fps = 0;
	memset( pass->images, 0, sizeof( pass->images ) );

	// at least two valid images are required: 'base' and 'celshade'
	for( int i = 0; i < 2; i++ ) {
		auto maybeToken = lexer->getNextTokenInLine();
		if( !maybeToken ) {
			return false;
		}
		auto token = *maybeToken;
		if( !isAPlaceholder( token ) ) {
			pass->images[i] = findImage( token, imageFlags | ( i ? IT_CLAMP | IT_CUBEMAP : 0 ) );
		}
	}

	pass->program_type = GLSL_PROGRAM_TYPE_CELSHADE;

	// parse optional images: [diffuse] [decal] [entitydecal] [stripes] [celllight]
	for( int i = 0; i < 5; i++ ) {
		auto maybeToken = lexer->getNextTokenInLine();
		if( !maybeToken ) {
			break;
		}
		auto token = *maybeToken;
		if( !isAPlaceholder( token ) ) {
			pass->images[i + 2] = findImage( token, imageFlags | ( i == 4 ? IT_CLAMP | IT_CUBEMAP : 0 ) );
		}
	}

	return true;
}

bool MaterialParser::parseTCGen() {
	auto maybeTCGen = lexer->getTCGen();
	if( !maybeTCGen ) {
	    return false;
	}

	TCGen tcGen = *maybeTCGen;

	auto *pass = currPass();
	pass->tcgen = (unsigned)tcGen;
	if( tcGen == TCGen::Vector ) {
		if( !lexer->getVector<4>( &pass->tcgenVec[0] ) ) {
			return false;
		}
		if( !lexer->getVector<4>( &pass->tcgenVec[4] ) ) {
			return false;
		}
	}

	return true;
}

bool MaterialParser::parseAlphaGen() {
	std::optional<AlphaGen> maybeAlphaGen = lexer->getAlphaGen();
	if( !maybeAlphaGen ) {
		return parseAlphaGenPortal();
	}

	auto *const pass = currPass();
	auto alphaGen = *maybeAlphaGen;
	pass->alphagen.type = (unsigned)alphaGen;

	if( alphaGen == AlphaGen::Const ) {
		pass->alphagen.args[0] = std::abs( lexer->getFloat().value_or(0) );
		return true;
	}

	if( alphaGen != AlphaGen::Wave ) {
		return true;
	}

	if( !parseFunc( &pass->alphagen.func ) ) {
		return false;
	}

	// treat custom distanceramp as portal
	if( pass->alphagen.func.type == SHADER_FUNC_RAMP && pass->alphagen.func.args[1] == 1 ) {
		portalDistance = std::max( std::abs( pass->alphagen.func.args[3] ), portalDistance );
	}

	return true;
}

static wsw::StringView kPortal = wsw::StringView( "portal" );

bool MaterialParser::parseAlphaGenPortal() {
	auto maybeToken = lexer->getNextToken();
	if( !maybeToken ) {
		return false;
	}

	if( !kPortal.equalsIgnoreCase( *maybeToken ) ) {
		return false;
	}

	auto *const pass = currPass();

	float dist = lexer->getFloatOr( 256.0f );
	pass->alphagen.type = ALPHA_GEN_WAVE;
	pass->alphagen.func.type = SHADER_FUNC_RAMP;

	Vector4Set( pass->alphagen.func.args, 0, 1, 0, dist );
	portalDistance = std::max( dist, portalDistance );
	return true;
}

bool MaterialParser::parseDetail() {
	flags |= SHADERPASS_DETAIL;
	return true;
}

bool MaterialParser::parseGrayscale() {
	flags |= SHADERPASS_GREYSCALE;
	return true;
}

bool MaterialParser::parseSkip() {
	lexer->skipToEndOfLine();
	return true;
}

bool MaterialParser::parseCull() {
	if( auto maybeCullMode = lexer->getCullMode() ) {
		flags &= ~( SHADER_CULL_FRONT | SHADER_CULL_BACK );
		auto cullMode = *maybeCullMode;
		if( cullMode == CullMode::Front ) {
			flags |= SHADER_CULL_FRONT;
		} else if( cullMode == CullMode::Back ) {
			flags |= SHADER_CULL_BACK;
		}
		return true;
	}
	return false;
}

bool MaterialParser::parseSkyParms() {
	// TODO: Implement...
	return true;
}

bool MaterialParser::parseSkyParms2() {
	// TODO: Implement
	return true;
}

bool MaterialParser::parseSkyParmsSides() {
	// TODO: Implement
	return true;
}

bool MaterialParser::parseFogParams() {
	vec3_t color, fcolor;
	if( !lexer->getVector<3>( color ) ) {
		return false;
	}

	ColorNormalize( color, fcolor );

	fog_color[0] = ( int )( fcolor[0] * 255.0f );
	fog_color[1] = ( int )( fcolor[1] * 255.0f );
	fog_color[2] = ( int )( fcolor[2] * 255.0f );
	fog_color[3] = 255;

	// TODO: Print warnings on getting default values?
	fog_dist = lexer->getFloatOr( -1.0f );
	if( fog_dist <= 0.1f ) {
		fog_dist = 128.0f;
	}

	// TODO: Print warnings on getting default values?
	fog_clearDist = lexer->getFloatOr( -1.0f );
	if( fog_clearDist > fog_dist - 128 ) {
		fog_clearDist = fog_dist - 128;
	}
	if( fog_clearDist <= 0.0f ) {
		fog_clearDist = 0;
	}

	return true;
}

bool MaterialParser::parseNoMipmaps() {
	noMipMaps = noPicMip = true;
	minMipSize = std::optional( 1 );
	return true;
}

bool MaterialParser::parseNoPicmip() {
	noPicMip = true;
	return true;
}

bool MaterialParser::parseNoCompress() {
	noCompress = true;
	return true;
}

bool MaterialParser::parseNofiltering() {
	noFiltering = true;
	flags |= SHADER_NO_TEX_FILTERING;
	return true;
}

bool MaterialParser::parseSmallestMipSize() {
	if( auto maybeMipSize = lexer->getInt() ) {
		minMipSize = std::optional( std::max( 1, *maybeMipSize ) );
		return true;
	}
	return false;
}

bool MaterialParser::parsePolygonOffset() {
	flags |= SHADER_POLYGONOFFSET;
	return true;
}

bool MaterialParser::parseStencilTest() {
	flags |= SHADER_STENCILTEST;
	return true;
}

bool MaterialParser::parseEntityMergable() {
	flags |= SHADER_ENTITY_MERGABLE;
	return true;
}

bool MaterialParser::parseSort() {
	if( auto maybeSortMode = lexer->getSortMode() ) {
		sort = (int)( *maybeSortMode );
		return true;
	}

	if( auto maybeNum = lexer->getInt() ) {
		sort = *maybeNum;
		// TODO: Check the lower bound and print a warning
		if( sort > SHADER_SORT_NEAREST ) {
			sort = SHADER_SORT_NEAREST;
		}
		return true;
	}
	return false;
}

bool MaterialParser::parseDeformVertexes() {
	std::optional<Deform> maybeDeform = lexer->getDeform();
	if( !maybeDeform ) {
		return false;
	}

	const auto deformType = *maybeDeform;
	if( deformType == Deform::Wave ) {
		return parseDeformWave();
	}

	if( deformType == Deform::Bulge ) {
		return parseDeformBulge();
	}

	if( deformType == Deform::Move ) {
		return parseDeformMove();
	}

	assert( deformType == Deform::Autosprite || deformType == Deform::Autosprite2 || deformType == Deform::Autoparticle );

	if( auto *deform = tryAddingDeform() ) {
		deform->type = DEFORMV_AUTOSPRITE;
		flags |= SHADER_AUTOSPRITE;
		return true;
	}

	return false;
}

bool MaterialParser::parseDeformWave() {
	auto *deform = tryAddingDeform();
	if( !deform ) {
		return false;
	}
	auto maybeArg = lexer->getFloat();
	if( !maybeArg ) {
		return false;
	}
	if( !parseFunc( &deform->func ) ) {
		return false;
	}
	deform->args[0] = *maybeArg;
	auto &fn = deform->func;
	if( !addToDeformSignature( deform->args[0], fn.type, fn.args[0], fn.args[1], fn.args[2], fn.args[3] ) ) {
		return false;
	}
	deform->args[0] = deform->args[0] ? 1.0f / deform->args[0] : 100.0f;
	return true;
}

bool MaterialParser::parseDeformMove() {
	auto *deform = tryAddingDeform();
	if( !deform ) {
		return false;
	}
	if( !lexer->getVector<3>( deform->args ) ) {
		return false;
	}
	if( !parseFunc( &deform->func ) ) {
		return false;
	}
	auto &fn = deform->func;
	if( !addToDeformSignature( deform->args[0], deform->args[1], deform->args[2] ) ) {
		return false;
	}
	if( !addToDeformSignature( fn.type, fn.args[0], fn.args[1], fn.args[2], fn.args[3] ) ) {
		return false;
	}
	return true;
}

bool MaterialParser::parseDeformBulge() {
	if( auto *deform = tryAddingDeform() ) {
		if ( lexer->getVector<4>( deform->args ) ) {
			return addToDeformSignature( deform->args[0], deform->args[1], deform->args[2], deform->args[3] );
		}
	}
	return false;
}

bool MaterialParser::parsePortal() {
	flags |= SHADER_PORTAL;
	sort = SHADER_SORT_PORTAL;
	return true;
}

bool MaterialParser::parseOffsetMappingScale() {
	offsetMappingScale = lexer->getFloatOr(0.0f );
	return true;
}

bool MaterialParser::parseGlossExponent() {
	glossExponent = lexer->getFloatOr(0.0f);
	return true;
}

bool MaterialParser::parseGlossIntensity() {
	glossIntensity = lexer->getFloatOr(0.0f);
	return true;
}

bool MaterialParser::parseTemplate() {
	if( lexer != &defaultLexer ) {
		// TODO:
		Com_Printf( "Recursive template" );
		return false;
	}

	// TODO: Disallow multiple template expansions as well?

	auto maybeNameToken = lexer->getNextToken();
	if( !maybeNameToken ) {
		return false;
	}

	// TODO: check immediately for template existance before parsing args?

	StaticVector<wsw::StringView, 16> args;
	for(;; ) {
		if( auto maybeToken = lexer->getNextTokenInLine() ) {
			if( args.size() == args.capacity() ) {
				return false;
			}
			args.push_back( *maybeToken );
		} else {
			break;
		}
	}

	lexer = materialCache->expandTemplate( *maybeNameToken, args.begin(), args.size() );
	bool result = false;
	if( lexer ) {
		// TODO... really?
		result = exec();
	}
	lexer = &defaultLexer;
	return result;
}

bool MaterialParser::parseSoftParticle() {
	flags |= SHADER_SOFT_PARTICLE;
	return true;
}

bool MaterialParser::parseForceWorldOutlines() {
	flags |= SHADER_FORCE_OUTLINE_WORLD;
	return true;
}

bool MaterialParser::parseFunc( shaderfunc_t *func ) {
	if( auto maybeFunc = lexer->getFunc() ) {
		lexer->getVectorOrFill<4>( func->args, 0.0f );
		return true;
	}
	return false;
}

class Evaluator {
public:
	enum { Capacity = 32 };
private:
	enum class Tag: uint8_t {
		Value,
		UnaryNot,
		LogicOp,
		CmpOp,
		LParen,
		RParen
	};

	struct alignas( 8 )TapeEntry {
		uint8_t data[7];
		Tag tag;
	};


	TapeEntry tape[Capacity];
	int numEntries { 0 };
	int tapeCursor { 0 };
	bool hadError { false };

#pragma pack( push, 2 )
	struct Value {
		union {
			int32_t i;
			bool b;
		} u;

		bool isBool;
		bool isInputValue { false };

		explicit Value( bool b ) : isBool( true ) {
			u.b = b;
		}
		explicit Value( int32_t i ) : isBool( false ) {
			u.i = i;
		}
		operator bool() const {
			return isBool ? u.b : u.i;
		}
		operator int() const {
			return isBool ? u.b : u.i;
		}
	};

	static_assert( alignof( Value ) <= 8 );
	static_assert( sizeof( Value ) == 6 );
#pragma pack( pop )

	TapeEntry *makeEntry( Tag tag ) {
		assert( numEntries < Capacity );
		auto *e = &tape[numEntries++];
		e->tag = tag;
		return e;
	}

	std::optional<Tag> nextTokenTag() {
		return ( tapeCursor < numEntries ) ? std::optional( tape[tapeCursor++].tag ) : std::nullopt;
	}

	void ungetToken() {
		assert( tapeCursor >= 0 );
		tapeCursor = tapeCursor ? tapeCursor - 1 : 0;
	}

	[[nodiscard]]
	const TapeEntry &lastEntry() const {
		assert( tapeCursor - 1 < numEntries );
		return tape[tapeCursor - 1];
	}

	template <typename T>
	[[nodiscard]]
	T lastEntryAs() const {
		return *( ( const T *)lastEntry().data );
	}

	[[nodiscard]]
	Tag lastTag() const {
		assert( tapeCursor - 1 < numEntries );
		return tape[tapeCursor - 1].tag;
	}

	std::optional<Value> lastValue() {
		return ( lastTag() == Tag::Value ) ? std::optional( lastEntryAs<Value>() ) : std::nullopt;
	}

	[[nodiscard]]
	std::optional<LogicOp> lastLogicOp() {
		return ( lastTag() == Tag::LogicOp ) ? std::optional( lastEntryAs<LogicOp>() ) : std::nullopt;
	}

	[[nodiscard]]
	std::optional<CmpOp> lastCmpOp() {
		return ( lastTag() == Tag::CmpOp ) ? std::optional( lastEntryAs<CmpOp>() ) : std::nullopt;
	}

#ifndef _MSC_VER
	std::nullopt_t withError( const char *fmt, ... ) __attribute__( ( format( printf, 2, 3 ) ) );
	void warn( const char *fmt, ... ) __attribute__( ( format( printf, 2, 3 ) ) );
#else
	std::nullopt_t withError( _Printf_format_string_ const char *fmt, ... );
	void warn( _Printf_format_string_ const char *fmt, ... );
#endif

	void warnBooleanToIntegerConversion( const Value &value, const char *desc, const char *context );
	void warnIntegerToBooleanConversion( const Value &value, const char *desc, const char *context );

	[[nodiscard]]
	std::optional<Value> evalUnaryExpr();
	[[nodiscard]]
	std::optional<Value> evalCmpExpr();
	[[nodiscard]]
	std::optional<Value> evalLogicExpr();

	[[nodiscard]]
	std::optional<Value> evalExpr() { return evalLogicExpr(); }
public:

	void addInt( int value ) {
		auto *v = new( makeEntry( Tag::Value ) )Value( value );
		v->isInputValue = true;
	}

	void addBool( bool value ) {
		auto *v = new( makeEntry( Tag::Value ) )Value( value );
		v->isInputValue = true;
	}

	void addLogicOp( LogicOp op ) {
		new( makeEntry( Tag::LogicOp ) )LogicOp( op );
	}

	void addCmpOp( CmpOp op ) {
		new( makeEntry( Tag::CmpOp ) )CmpOp( op );
	}

	void addUnaryNot() { makeEntry( Tag::UnaryNot ); }

	void addLeftParen() { makeEntry( Tag::LParen ); }
	void addRightParen() { makeEntry( Tag::RParen ); }

	[[nodiscard]]
	std::optional<bool> exec();
};

std::nullopt_t Evaluator::withError( const char *fmt, ... ) {
	if( hadError ) {
		return std::nullopt;
	}

	char buffer[1024];

	va_list va;
	va_start( va, fmt );
	Q_vsnprintfz( buffer, sizeof( buffer ), fmt, va );
	va_end( va );

	Com_Printf( S_COLOR_RED "%s\n", buffer );

	hadError = true;
	return std::nullopt;
}

void Evaluator::warn( const char *fmt, ... ) {
	if( hadError ) {
		return;
	}

	char buffer[1024];

	va_list va;
	va_start( va, fmt );
	Q_vsnprintfz( buffer, sizeof( buffer ), fmt, va );
	va_end( va );

	Com_Printf( S_COLOR_YELLOW "%s\n", buffer );
}

void Evaluator::warnBooleanToIntegerConversion( const Value &value, const char *desc, const char *context ) {
	if( value.isInputValue ) {
		warn( "Converting %s value %s to an integer %s", desc, value ? "true" : "false", context );
	} else {
		warn( "Converting %s expression to an integer %s", desc, context );
	}
}

void Evaluator::warnIntegerToBooleanConversion( const Value &value, const char *desc, const char *context ) {
	if( value.isInputValue ) {
		warn( "Converting %s value %d to a boolean %s", desc, (int)value, context );
	} else {
		warn( "Converting %s expression to a boolean %s", desc, context );
	}
}

std::optional<Evaluator::Value> Evaluator::evalUnaryExpr() {
	std::optional<Tag> maybeTag = nextTokenTag();
	if( !maybeTag ) {
		return std::nullopt;
	}

	Tag tag = *maybeTag;
	if( tag == Tag::Value ) {
		return lastValue();
	}

	if( tag == Tag::UnaryNot ) {
		if( auto maybeValue = evalUnaryExpr() ) {
			Value value = *maybeValue;
			if( !value.isBool ) {
				warnIntegerToBooleanConversion( value, "an integer", "under a NOT operator");
			}
			return Value( !(bool)value );
		}
		return withError( "Expected an unary expression after an unary NOT operator" );
	}

	if( tag == Tag::LParen ) {
		if( auto maybeValue = evalExpr() ) {
			if( auto maybeParenTag = nextTokenTag() ) {
				if( *maybeParenTag == Tag::RParen ) {
					return maybeValue;
				}
				return withError( "Expected a right paren after an expression" );
			}
		}
		return withError( "Expected an expression in parentheses" );
	}

	return withError( "Expected a value, an unary NOT operator or a left paren" );
}

std::optional<Evaluator::Value> Evaluator::evalCmpExpr() {
	std::optional<Value> maybeLeft = evalUnaryExpr();
	if( !maybeLeft ) {
		return std::nullopt;
	}

	std::optional<Tag> maybeTag = nextTokenTag();
	if( !maybeTag ) {
		return maybeLeft;
	}

	if( *maybeTag != Tag::CmpOp ) {
		ungetToken();
		return maybeLeft;
	}

	const auto op = *lastCmpOp();

	std::optional<Value> maybeRight = evalUnaryExpr();
	if( !maybeRight ) {
		return withError( "Failed to evaluate a right-hand expression" );
	}

	Value left = *maybeLeft;
	if( left.isBool ) {
		warnBooleanToIntegerConversion( left, "a left-hand boolean", "for comparison" );
	}

	Value right = *maybeRight;
	if( right.isBool ) {
		warnBooleanToIntegerConversion( right, "a right-hand boolean", "for comparison" );
	}

	switch( op ) {
		case CmpOp::LS:
			return std::optional( Value( (int)left < (int)right ) );
		case CmpOp::LE:
			return std::optional( Value( (int)left <= (int)right ) );
		case CmpOp::NE:
			return std::optional( Value( (int)left != (int)right ) );
		case CmpOp::EQ:
			return std::optional( Value( (int)left == (int)right ) );
		case CmpOp::GE:
			return std::optional( Value( (int)left >= (int)right ) );
		case CmpOp::GT:
			return std::optional( Value( (int)left > (int)right ) );
	}
}

std::optional<Evaluator::Value> Evaluator::evalLogicExpr() {
	std::optional<Value> maybeLeft = evalCmpExpr();
	if( !maybeLeft ) {
		return std::nullopt;
	}

	std::optional<Tag> maybeTag = nextTokenTag();
	if( !maybeTag ) {
		return maybeLeft;
	}

	if( *maybeTag != Tag::LogicOp ) {
		ungetToken();
		return maybeLeft;
	}

	const auto op = *lastLogicOp();

	std::optional<Value> maybeRight = evalCmpExpr();
	if( !maybeRight ) {
		return withError( "Missing a right-hand operand of a logical operation" );
	}

	Value left = *maybeLeft;
	if( !left.isBool ) {
		warnIntegerToBooleanConversion( left, "a left-hand integer", "for a logical operation" );
	}

	Value right = *maybeRight;
	if( !right.isBool ) {
		warnIntegerToBooleanConversion( right, "a right-hand integer", "for a logical operation" );
	}

	switch( op ) {
		case LogicOp::And:
			return std::optional( Value( (bool)left && (bool)right ) );
		case LogicOp::Or:
			return std::optional( Value( (bool)left || (bool)right ) );
	}
}

std::optional<bool> Evaluator::exec() {
	std::optional<Value> maybeValue = evalExpr();
	if( hadError ) {
		return std::nullopt;
	}

	if( tapeCursor != numEntries ) {
		return withError( "Syntax error. Are there missing operators or mismatched parentheses?" );
	}

	if( !maybeValue ) {
		return std::nullopt;
	}

	Value value = *maybeValue;
	if( !value.isBool ) {
		warnIntegerToBooleanConversion( value, "an integer", "as an IF condition");
	}
	return (bool)value;
}

bool MaterialParser::parseIf() {
	if( auto maybeCondition = parseCondition() ) {
		if( *maybeCondition ) {
			skipConditionBlock();
		}
		return true;
	}
	return false;
}

static const wsw::StringView kIfLiteral( "if" );
static const wsw::StringView kEndIfLiteral( "endif" );

void MaterialParser::skipConditionBlock() {
	for( int depth = 1; depth > 0; ) {
		std::optional<wsw::StringView> maybeToken = lexer->getNextToken();
		if( !maybeToken ) {
			break;
		}
		if( kIfLiteral.equalsIgnoreCase( *maybeToken ) ) {
			depth++;
		} else if( kEndIfLiteral.equalsIgnoreCase( *maybeToken ) ) {
			depth--;
		}
	}
}

std::optional<bool> MaterialParser::parseCondition() {
	Evaluator evaluator;

	int numTokens = 0;
	for(;; ) {
		auto maybeToken = lexer->getNextTokenInLine();
		if( !maybeToken ) {
			if( numTokens == 0 ) {
				return std::nullopt;
			}
			break;
		}

		if( numTokens == Evaluator::Capacity ) {
			return std::nullopt;
		}

		numTokens++;

		lexer->unGetToken();
		if( auto maybeLogicOp = lexer->getLogicOp() ) {
			evaluator.addLogicOp( *maybeLogicOp );
		} else if( auto maybeCmpOp = lexer->getCmpOp() ) {
			evaluator.addCmpOp( *maybeCmpOp );
		} else if( auto maybeIntVar = lexer->getIntConditionVar() ) {
			evaluator.addInt( getIntConditionVarValue( *maybeIntVar ) );
		} else if( auto maybeBoolVar = lexer->getBoolConditionVar() ) {
			evaluator.addBool( getBoolConditionVarValue( *maybeBoolVar ) );
		} if( auto maybeInt = lexer->getInt() ) {
			evaluator.addInt( *maybeInt );
		} if( auto maybeBool = lexer->getBool() ) {
			evaluator.addBool( *maybeBool );
		}

		auto token = *maybeToken;
		if( token.length() != 1 ) {
			return std::nullopt;
		}

		const char ch = token[0];
		if( ch == '(' ) {
			evaluator.addLeftParen();
		} else if( ch == ')' ) {
			evaluator.addRightParen();
		} else if( ch == '!' ) {
			evaluator.addUnaryNot();
		} else {
			return std::nullopt;
		}
	}

	return evaluator.exec();
}

int MaterialParser::getIntConditionVarValue( IntConditionVar var ) {
	switch( var ) {
		case IntConditionVar::MaxTextureSize:
			return glConfig.maxTextureSize;
		case IntConditionVar::MaxTextureCubemapSize:
			return glConfig.maxTextureCubemapSize;
		case IntConditionVar::MaxTextureUnits:
			return glConfig.maxTextureUnits;
	}
}

bool MaterialParser::getBoolConditionVarValue( BoolConditionVar var ) {
	switch( var ) {
		case BoolConditionVar::TextureCubeMap:
			return true;
		case BoolConditionVar::Glsl:
			return true;
		case BoolConditionVar::DeluxeMaps:
			return mapConfig.deluxeMaps;
		case BoolConditionVar::PortalMaps:
			return r_portalmaps->integer;
	}
}

int MaterialParser::buildVertexAttribs() {
	int attribs = VATTRIB_POSITION_BIT;

	if( flags & SHADER_SKY ) {
		attribs |= VATTRIB_TEXCOORDS_BIT;
	}

	for( const auto &deform: deforms ) {
		attribs |= getDeformVertexAttribs( deform );
	}

	for( const auto &pass: passes ) {
		attribs |= getPassVertexAttribs( pass );
	}

	return attribs;
}

int MaterialParser::getDeformVertexAttribs( const deformv_t &deform ) {
	switch( deform.type ) {
		case DEFORMV_BULGE:
			return VATTRIB_NORMAL_BIT | VATTRIB_TEXCOORDS_BIT;
		case DEFORMV_WAVE:
			return VATTRIB_NORMAL_BIT;
		case DEFORMV_AUTOSPRITE:
		case DEFORMV_AUTOPARTICLE:
			return VATTRIB_AUTOSPRITE_BIT;
		case DEFORMV_AUTOSPRITE2:
			return VATTRIB_AUTOSPRITE_BIT | VATTRIB_AUTOSPRITE2_BIT;
		default:
			return 0;
	}
}

int MaterialParser::getPassVertexAttribs( const shaderpass_t &pass ) {
	int res = 0;

	if( pass.program_type == GLSL_PROGRAM_TYPE_MATERIAL ) {
		res |= VATTRIB_NORMAL_BIT | VATTRIB_SVECTOR_BIT | VATTRIB_LMCOORDS0_BIT;
	} else if( pass.program_type == GLSL_PROGRAM_TYPE_DISTORTION ) {
		res |= VATTRIB_NORMAL_BIT | VATTRIB_SVECTOR_BIT;
	} else if( pass.program_type == GLSL_PROGRAM_TYPE_CELSHADE ) {
		res |= VATTRIB_TEXCOORDS_BIT | VATTRIB_NORMAL_BIT;
	}

	res |= getRgbGenVertexAttribs( pass, pass.rgbgen );
	res |= getAlphaGenVertexAttribs( pass.alphagen );
	res |= getTCGenVertexAttribs( pass.tcgen );
	return res;
}

int MaterialParser::getRgbGenVertexAttribs( const shaderpass_t &pass, const colorgen_t &gen ) {
	// Looks way too ugly being in the switch block
	if( gen.type == RGB_GEN_LIGHTING_DIFFUSE ) {
		int res = VATTRIB_NORMAL_BIT;
		if( pass.program_type == GLSL_PROGRAM_TYPE_MATERIAL ) {
			res = ( res | VATTRIB_COLOR0_BIT ) & ~VATTRIB_LMCOORDS0_BIT;
		}
		return res;
	}

	switch( gen.type ) {
		case RGB_GEN_VERTEX:
		case RGB_GEN_ONE_MINUS_VERTEX:
		case RGB_GEN_EXACT_VERTEX:
			return VATTRIB_COLOR0_BIT;
		case RGB_GEN_WAVE:
			return gen.func.type == SHADER_FUNC_RAMP ? VATTRIB_NORMAL_BIT : 0;
		default:
			return 0;
	}
}

int MaterialParser::getAlphaGenVertexAttribs( const colorgen_t &gen ) {
	switch( gen.type ) {
		case ALPHA_GEN_VERTEX:
		case ALPHA_GEN_ONE_MINUS_VERTEX:
			return VATTRIB_COLOR0_BIT;
		case ALPHA_GEN_WAVE:
			return gen.func.type == SHADER_FUNC_RAMP ? VATTRIB_NORMAL_BIT : 0;
		default:
			return 0;
	}
}

int MaterialParser::getTCGenVertexAttribs( unsigned gen ) {
	switch( gen ) {
		case TC_GEN_LIGHTMAP:
			return VATTRIB_LMCOORDS0_BIT;
		case TC_GEN_ENVIRONMENT:
		case TC_GEN_REFLECTION:
		case TC_GEN_REFLECTION_CELSHADE:
			return VATTRIB_NORMAL_BIT;
		default:
			return VATTRIB_TEXCOORDS_BIT;
	}
}

void MaterialParser::fixLightmapsForVertexLight() {
	if( !hasLightmapPass ) {
		return;
	}

	if( type != SHADER_TYPE_DELUXEMAP && type != SHADER_TYPE_VERTEX ) {
		return;
	}

	// fix up rgbgen's and blendmodes for lightmapped shaders and vertex lighting
	for( auto &pass: passes ) {
		const auto blendMask = pass.flags & GLSTATE_BLEND_MASK;

		// TODO: Simplify
		if( !( !blendMask || blendMask == ( GLSTATE_SRCBLEND_DST_COLOR | GLSTATE_DSTBLEND_ZERO ) || passes.size() == 1 ) ) {
			continue;
		}

		if( pass.rgbgen.type == RGB_GEN_IDENTITY ) {
			pass.rgbgen.type = RGB_GEN_VERTEX;
		}
		//if( pass->alphagen.type == ALPHA_GEN_IDENTITY )
		//	pass->alphagen.type = ALPHA_GEN_VERTEX;

		if( !( pass.flags & SHADERPASS_ALPHAFUNC ) ) {
			if( blendMask != ( GLSTATE_SRCBLEND_SRC_ALPHA | GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
				pass.flags &= ~blendMask;
			}
		}
		break;
	}
}

void MaterialParser::fixFlagsAndSortingOrder() {
	if( passes.empty() && !sort ) {
		if( flags & SHADER_PORTAL ) {
			sort = SHADER_SORT_PORTAL;
		} else {
			sort = SHADER_SORT_ADDITIVE;
		}
	}

	if( flags & SHADER_SKY ) {
		sort = SHADER_SORT_SKY;
	}
	if( ( flags & SHADER_POLYGONOFFSET ) && !sort ) {
		sort = SHADER_SORT_DECAL;
	}
	if( flags & SHADER_AUTOSPRITE ) {
		flags &= ~( SHADER_CULL_FRONT | SHADER_CULL_BACK );
	}

	const shaderpass_t *opaquePass = nullptr;
	for( auto &pass: passes ) {
		const auto blendmask = pass.flags & GLSTATE_BLEND_MASK;

		if( !opaquePass && !blendmask ) {
			opaquePass = &pass;
		}

		if( !blendmask ) {
			pass.flags |= GLSTATE_DEPTHWRITE;
		}
		if( ( pass.flags & SHADERPASS_LIGHTMAP || pass.program_type == GLSL_PROGRAM_TYPE_MATERIAL ) ) {
			if( type > SHADER_TYPE_DELUXEMAP ) {
				flags |= SHADER_LIGHTMAP;
			}
		}
		if( pass.flags & GLSTATE_DEPTHWRITE ) {
			if( flags & SHADER_SKY ) {
				pass.flags &= ~GLSTATE_DEPTHWRITE;
			} else {
				flags |= SHADER_DEPTHWRITE;
			}
		}

		// disable r_drawflat for shaders with customizable color passes
		if( pass.rgbgen.type == RGB_GEN_CONST || pass.rgbgen.type == RGB_GEN_CUSTOMWAVE ||
			pass.rgbgen.type == RGB_GEN_ENTITYWAVE || pass.rgbgen.type == RGB_GEN_ONE_MINUS_ENTITY ) {
			flags |= SHADER_NODRAWFLAT;
		}
	}

	if( !sort ) {
		if( opaquePass ) {
			if( opaquePass->flags & SHADERPASS_ALPHAFUNC ) {
				sort = SHADER_SORT_ALPHATEST;
			}
		} else {
			// all passes have blendfuncs
			sort = ( flags & SHADER_DEPTHWRITE ) ? SHADER_SORT_ALPHATEST : SHADER_SORT_ADDITIVE;
		}
	}

	if( !sort ) {
		sort = SHADER_SORT_OPAQUE;
	}

	// disable r_drawflat for transparent shaders
	if( sort >= SHADER_SORT_UNDERWATER ) {
		flags |= SHADER_NODRAWFLAT;
	}
}

shader_t *MaterialParser::build() {
	if( r_lighting_vertexlight->integer ) {
		fixLightmapsForVertexLight();
	}

	fixFlagsAndSortingOrder();

	const auto attribs = buildVertexAttribs();

	using TcGenSpec = MemSpecBuilder::Spec<float>;
	using TcModSpec = MemSpecBuilder::Spec<tcmod_t>;
	StaticVector<std::optional<TcGenSpec>, 16> tcGenSpecs;
	StaticVector<std::optional<TcModSpec>, 16> tcModSpecs;

	MemSpecBuilder memSpec;
	const auto shaderSpec = memSpec.add<shader_t>();
	const auto passesSpec = memSpec.add<shaderpass_t>( passes.size() );

	for( const auto &pass: passes ) {
		if( auto numElems = pass.getNumTCGenElems() ) {
			tcGenSpecs.push_back( memSpec.add<float>( numElems ) );
		} else {
			tcGenSpecs.push_back( std::nullopt );
		}

		if( pass.numtcmods ) {
			tcModSpecs.push_back( memSpec.add<tcmod_t>( pass.numtcmods ) );
		} else {
			tcModSpecs.push_back( std::nullopt );
		}
	}

	std::optional<MemSpecBuilder::Spec<deformv_t>> deformDataSpec = std::nullopt;
	if( auto size = deforms.size() ) {
		deformDataSpec = memSpec.add<deformv_t>( size );
	}

	const auto nameSpec = memSpec.add<char>( cleanName.size() + 1 );

	std::optional<MemSpecBuilder::Spec<int>> deformSigSpec = std::nullopt;
	if( auto size = deformSig.size() ) {
		deformSigSpec = memSpec.add<int>( size );
	}

	void *const baseMem = ::malloc( memSpec.sizeSoFar() );
	if( !baseMem ) {
		return nullptr;
	}

	memset( baseMem, 0, memSpec.sizeSoFar() );
	auto *const s = new( shaderSpec.get( baseMem ) )shader_t;

	char *const savedName = nameSpec.get( baseMem );
	std::memcpy( savedName, cleanName.data(), cleanName.size() );
	savedName[cleanName.size()] = '\0';
	s->name = wsw::HashedStringView( savedName, cleanName.size(), cleanName.getHash(), wsw::StringView::ZeroTerminated );

	s->type = this->type;
	s->flags = this->flags;
	s->sort = this->sort;
	s->vattribs = attribs;

	s->passes = passesSpec.get( baseMem );
	s->numpasses = passes.size();

	// Perform a shallow copy first
	std::copy( passes.begin(), passes.end(), s->passes );

	for( size_t i = 0; i < passes.size(); ++i ) {
		auto *const savedPass = &s->passes[i];
		const auto *parsedPass = &passes[i];
		if( auto tcGenSpec = tcGenSpecs[i] ) {
			savedPass->tcgenVec = ( *tcGenSpec ).get( baseMem );
			size_t numTCGenElems = parsedPass->getNumTCGenElems();
			assert( numTCGenElems );
			std::copy( parsedPass->tcgenVec, parsedPass->tcgenVec + numTCGenElems, savedPass->tcgenVec );
		}
		if( auto tcModSpec = tcModSpecs[i] ) {
			savedPass->tcmods = ( *tcModSpec ).get( baseMem );
			std::copy( savedPass->tcmods, savedPass->tcmods + savedPass->numtcmods, parsedPass->tcmods );
		}
	}

	s->numdeforms = deforms.size();
	if( deformDataSpec ) {
		s->deforms = ( *deformDataSpec ).get( baseMem );
		std::copy( deforms.begin(), deforms.end(), s->deforms );
	}

	if( deformSigSpec ) {
		int *sigData = ( *deformSigSpec ).get( baseMem );
		size_t rawDataLen = deformSig.size() * sizeof( int );
		std::copy( deformSig.begin(), deformSig.end(), sigData );
		s->deformSig.hash = COM_SuperFastHash( (const uint8_t *)sigData, rawDataLen, rawDataLen );
		s->deformSig.len = (uint32_t)deformSig.size();
		s->deformSig.data = sigData;
	}

	std::copy( std::begin( fog_color ), std::end( fog_color ), s->fog_color );
	s->fog_dist = fog_dist;
	s->fog_clearDist = fog_clearDist;

	s->glossIntensity = glossIntensity;
	s->glossExponent = glossExponent;
	s->offsetmappingScale = offsetMappingScale;

	s->portalDistance = portalDistance;

	// TODO: Copy sky params

	return s;
}

int MaterialParser::getImageFlags() {
	int flags = 0;

	flags |= ( ( flags & SHADER_SKY ) ? IT_SKY : 0 );
	flags |= ( noMipMaps ? IT_NOMIPMAP : 0 );
	flags |= ( noPicMip ? IT_NOPICMIP : 0 );
	flags |= ( noCompress ? IT_NOCOMPRESS : 0 );
	flags |= ( noFiltering ? IT_NOFILTERING : 0 );
	if( type == SHADER_TYPE_2D || type == SHADER_TYPE_2D_RAW || type == SHADER_TYPE_VIDEO ) {
		flags |= IT_SYNC;
	}
	//if( r_shaderHasAutosprite )
	//	flags |= IT_CLAMP;

	return flags;
}