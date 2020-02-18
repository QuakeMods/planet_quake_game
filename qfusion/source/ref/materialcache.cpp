/*
Copyright (C) 1999 Stephen C. Taylor
Copyright (C) 2002-2007 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// r_shader.c

#include "local.h"
#include "../qcommon/hash.h"
#include "../qcommon/links.h"
#include "../qcommon/qcommon.h"
#include "../qcommon/singletonholder.h"
#include "materiallocal.h"

#include <algorithm>

static SingletonHolder<MaterialCache> materialCacheInstanceHolder;

void MaterialCache::init() {
	::materialCacheInstanceHolder.Init();
}

void MaterialCache::shutdown() {
	::materialCacheInstanceHolder.Shutdown();
}

MaterialCache *MaterialCache::instance() {
	return ::materialCacheInstanceHolder.Instance();
}

MaterialCache::MaterialCache() {
	for( const char *dir : { "<scripts", ">scripts", "scripts" } ) {
		// TODO: Must be checked if exists
		loadDirContents( dir );
	}

	freeMaterialIds.reserve( MAX_SHADERS );
	for( unsigned i = 0; i < MAX_SHADERS; ++i ) {
		freeMaterialIds.push_back( i );
	}
}

void MaterialCache::loadDirContents( const char *dir ) {
	// enumerate shaders
	const int numfiles = FS_GetFileList( dir, ".shader", NULL, 0, 0, 0 );

	char shaderPaths[1024];

	int k = 0;
	// now load them all
	for( int i = 0; i < numfiles; i += k ) {
		// TODO: There should be a sane iteration facility
		if( ( k = FS_GetFileList( dir, ".shader", shaderPaths, sizeof( shaderPaths ), i, numfiles ) ) == 0 ) {
			k = 1; // advance by one file
			continue;
		}

		// TODO: There should be a sane iteration facility
		const char *fileptr = shaderPaths;
		for( int j = 0; j < k; j++ ) {
			this->addFileContents( fileptr );
			fileptr += strlen( fileptr ) + 1;
			if( !*fileptr ) {
				break;
			}
		}
	}
}

MaterialCache::~MaterialCache() {
}

/*
* R_TouchShader
*/
void R_TouchShader( shader_t *s ) {
	if( s->registrationSequence == rsh.registrationSequence ) {
		return;
	}

	s->registrationSequence = rsh.registrationSequence;

	// touch all images this shader references
	for( unsigned i = 0; i < s->numpasses; i++ ) {
		shaderpass_t *pass = s->passes + i;

		for( unsigned j = 0; j < MAX_SHADER_IMAGES; j++ ) {
			image_t *image = pass->images[j];
			if( image ) {
				R_TouchImage( image, s->imagetags );
			} else if( !pass->program_type ) {
				// only programs can have gaps in images
				break;
			}
		}
	}

	if( s->flags & SHADER_SKY ) {
		// touch sky images for this shader
		for( image_t *image: s->skyboxImages ) {
			if( image ) {
				R_TouchImage( image, s->imagetags );
			}
		}
	}
}

void MaterialCache::freeUnusedMaterialsByType( const shaderType_e *types, unsigned int numTypes ) {
	if( !numTypes ) {
		for( shader_s *material = materialsHead; material; material = material->next[shader_t::kListLinks] ) {
			if( material->registrationSequence != rsh.registrationSequence ) {
				continue;
			}
			unlinkAndFree( material );
		}
		return;
	}

	for( shader_s *material = materialsHead; material; material = material->next[shader_t::kListLinks] ) {
		if( material->registrationSequence == rsh.registrationSequence ) {
			continue;
		}
		if( std::find( types, types + numTypes, material->type ) == types + numTypes ) {
			continue;
		}
		unlinkAndFree( material );
	}
}

void MaterialCache::unlinkAndFree( shader_t *s ) {
	materialById[s->id] = nullptr;
	freeMaterialIds.push_back( s->id );

	abort();
}

unsigned MaterialCache::getNextMaterialId() {
	if( freeMaterialIds.empty() ) {
		Com_Error( ERR_FATAL, "Out of free material ids\n" );
	}
	auto result = freeMaterialIds.back();
	freeMaterialIds.pop_back();
	return result;
}

wsw::HashedStringView MaterialCache::makeCleanName( const wsw::StringView &name ) {
	cleanNameBuffer.clear();
	cleanNameBuffer.reserve( name.length() + 1 );

	unsigned i = 0;
	for( char ch: name ) {
		if( !( ch == '/' | ch == '\\' ) ) {
			break;
		}
		i++;
	}

	unsigned lastDot = 0;
	unsigned lastSlash = 0;
	unsigned len = 0;

	uint32_t hash = 0;
	uint32_t hashBackup = 0;
	for(; i < name.length(); ++i ) {
		char ch = name[i];
		if( ch == '.' ) {
			lastDot = len;
			hashBackup = hash;
		}
		char cleanCh = ch != '\\' ? tolower( ch ) : '/';
		cleanNameBuffer.push_back( cleanCh );
		hash = NextHashStep( hash, cleanCh );
		if( cleanNameBuffer.back() == '/' ) {
			lastSlash = len;
		}
		len++;
	}

	if( !len ) {
		return wsw::HashedStringView();
	}

	if( lastDot < lastSlash ) {
		lastDot = 0;
	}
	if( lastDot ) {
		cleanNameBuffer.resize( lastDot );
		hash = hashBackup;
	}

	return wsw::HashedStringView( cleanNameBuffer.data(), cleanNameBuffer.length(), hash );
}

TokenStream *MaterialCache::getTokenStreamForShader( const wsw::HashedStringView &cleanName ) {
	Source *source = findSourceByName( cleanName );
	if( !source ) {
		return nullptr;
	}

	if( !primaryTokenStreamHolder.empty() ) {
		primaryTokenStreamHolder.pop_back();
	}

	const char *data = source->file->data;
	const TokenSpan *spans = source->file->spans + source->tokenSpansOffset;
	void *mem = primaryTokenStreamHolder.unsafe_grow_back();
	return new( mem )TokenStream( data, spans, source->numTokens );
}

/*
* R_PackShaderOrder
*
* Sort opaque shaders by this value for better caching of GL/program state.
*/
unsigned R_PackShaderOrder( const shader_t *shader ) {
	int order;
	int program_type;
	const shaderpass_t *pass;

	if( !shader->numpasses ) {
		return 0;
	}

	pass = &shader->passes[0];
	program_type = pass->program_type;

	if( program_type == GLSL_PROGRAM_TYPE_MATERIAL ) {
		// this is not a material shader in case all images are missing except for the defuse
		if( ( !pass->images[1] || pass->images[1]->missing || pass->images[1] == rsh.blankBumpTexture ) &&
			( !pass->images[2] || pass->images[2]->missing ) &&
			( !pass->images[3] || pass->images[3]->missing ) &&
			( !pass->images[4] || pass->images[4]->missing ) ) {
			program_type = GLSL_PROGRAM_TYPE_Q3A_SHADER;
		}
	}

	// sort by base program type
	order = program_type & 0x1F;

	// check presence of gloss for materials
	if( program_type == GLSL_PROGRAM_TYPE_MATERIAL && pass->images[2] != NULL && !pass->images[2]->missing ) {
		order |= 0x20;
	}

	return order;
}

void MaterialCache::touchMaterialsByName( const wsw::StringView &name ) {
	wsw::HashedStringView cleanView( makeCleanName( name ) );
	auto binIndex = cleanView.getHash() % kNumBins;
	for( shader_t *material = materialBins[binIndex]; material; material = material->next[shader_t::kBinLinks] ) {
		if( cleanView.equalsIgnoreCase( wsw::HashedStringView( material->name ) ) ) {
			R_TouchShader( material );
		}
	}
}

shader_t *MaterialCache::loadMaterial( const wsw::StringView &name, int type, bool forceDefault, image_t *defaultImage ) {
	wsw::HashedStringView cleanName( makeCleanName( name ) );
	const auto binIndex = cleanName.getHash() % kNumBins;

	for( shader_t *material = materialBins[binIndex]; material; material = material->next[shader_t::kBinLinks] ) {
		if( !cleanName.equalsIgnoreCase( material->name ) ) {
			continue;
		}
		// TODO: This should be a method
		if( material->type == type || ( type == SHADER_TYPE_2D && material->type == SHADER_TYPE_2D_RAW ) ) {
			R_TouchShader( material );
			return material;
		}
	}

	TokenStream *tokenStream = forceDefault ? nullptr : getTokenStreamForShader( cleanName );
	auto *material = loadMaterial( cleanName, name, type, tokenStream );
	if( !material ) {
		return nullptr;
	}

	material->registrationSequence = rsh.registrationSequence;
	::Link( material, &materialBins[binIndex], shader_t::kBinLinks );
	return material;
}

shader_t *MaterialCache::loadDefaultMaterial( const wsw::StringView &name, int type ) {
    return loadMaterial( name, type, true, rsh.noTexture );
}

/*
* R_RegisterPic
*/
shader_t *R_RegisterPic( const char *name ) {
	return MaterialCache::instance()->loadMaterial( wsw::StringView( name ), SHADER_TYPE_2D, false, rsh.noTexture );
}

/*
* R_RegisterRawPic_
*
* Registers default 2D shader with base image provided as raw data.
*/
shader_t *R_RegisterRawPic_( const char *name, int width, int height, uint8_t *data, int flags, int samples, bool bgra ) {
	int type = SHADER_TYPE_2D_RAW;
	flags |= IT_SPECIAL;
	if( bgra ) {
		flags |= IT_BGRA;
	}

	auto *s = MaterialCache::instance()->loadDefaultMaterial( wsw::StringView( name ), type );
	if( !s ) {
	    return nullptr;
	}

	// unlink and delete the old image from memory, unless it's the default one
	image_t *image = s->passes[0].images[0];
	if( !image || image == rsh.noTexture ) {
		// try to load new image
		image = R_LoadImage( name, &data, width, height, flags, 1, IMAGE_TAG_GENERIC, samples );
		s->passes[0].images[0] = image;
	} else {
		// replace current texture data
		R_ReplaceImage( image, &data, width, height, image->flags, 1, image->samples );
	}
	return s;
}

/*
* R_RegisterRawPic
*
* Registers default 2D shader with base image provided as raw color data.
*/
shader_t *R_RegisterRawPic( const char *name, int width, int height, uint8_t *data, int samples, bool bgra ) {
	return R_RegisterRawPic_( name, width, height, data, 0, samples, bgra );
}

/*
* R_RegisterRawAlphaMask
*
* Registers default alpha mask shader with base image provided as raw alpha values.
*/
shader_t *R_RegisterRawAlphaMask( const char *name, int width, int height, uint8_t *data ) {
	return R_RegisterRawPic_( name, width, height, data, IT_ALPHAMASK, 1, false );
}

/*
* R_RegisterLevelshot
*/
shader_t *R_RegisterLevelshot( const char *name, shader_t *defaultShader, bool *matchesDefault ) {
	shader_t *shader;

	image_t *defaultImage = defaultShader ? defaultShader->passes[0].images[0] : NULL;
	shader = MaterialCache::instance()->loadMaterial( wsw::StringView( name ), SHADER_TYPE_2D, true, defaultImage );

	if( matchesDefault ) {
		*matchesDefault = ( shader->passes[0].images[0] == defaultImage );
	}

	return shader;
}

/*
* R_RegisterShader
*/
shader_t *R_RegisterShader( const char *name, shaderType_e type ) {
	return MaterialCache::instance()->loadMaterial( wsw::StringView( name ), type, false, rsh.noTexture );
}

/*
* R_RegisterSkin
*/
shader_t *R_RegisterSkin( const char *name ) {
	return MaterialCache::instance()->loadMaterial( wsw::StringView( name ), SHADER_TYPE_DIFFUSE, false, rsh.noTexture );
}

/*
* R_RegisterVideo
*/
shader_t *R_RegisterVideo( const char *name ) {
	return MaterialCache::instance()->loadMaterial( wsw::StringView( name ), SHADER_TYPE_VIDEO, false, rsh.noTexture );
}

/*
* R_RegisterLinearPic
*/
shader_t *R_RegisterLinearPic( const char *name ) {
	return MaterialCache::instance()->loadMaterial( wsw::StringView( name ), SHADER_TYPE_2D_LINEAR, false, rsh.noTexture );
}


/*
* R_GetShaderDimensions
*
* Returns dimensions for shader's base (taken from the first pass) image
*/
void R_GetShaderDimensions( const shader_t *shader, int *width, int *height ) {
	image_t *baseImage;

	assert( shader );
	if( !shader || !shader->numpasses ) {
		return;
	}

	baseImage = shader->passes[0].images[0];
	if( !baseImage ) {
		//Com_DPrintf( S_COLOR_YELLOW "R_GetShaderDimensions: shader %s is missing base image\n", shader->name );
		return;
	}

	if( width ) {
		*width = baseImage->width;
	}
	if( height ) {
		*height = baseImage->height;
	}
}

/*
* R_ReplaceRawSubPic
*
* Adds a new subimage to the specified raw pic.
* Must not be used to overwrite previously written areas when doing batched drawing.
*/
void R_ReplaceRawSubPic( shader_t *shader, int x, int y, int width, int height, uint8_t *data ) {
	image_t *baseImage;

	assert( shader );
	if( !shader ) {
		return;
	}

	assert( shader->type == SHADER_TYPE_2D_RAW );
	if( shader->type != SHADER_TYPE_2D_RAW ) {
		return;
	}

	baseImage = shader->passes[0].images[0];

	assert( ( ( x + width ) <= baseImage->upload_width ) && ( ( y + height ) <= baseImage->upload_height ) );
	if( ( ( x + width ) > baseImage->upload_width ) || ( ( y + height ) > baseImage->upload_height ) ) {
		return;
	}

	R_ReplaceSubImage( baseImage, 0, x, y, &data, width, height );
}

shader_t *MaterialCache::initMaterial( int type, const wsw::HashedStringView &cleanName, MemSpecBuilder memSpec ) {
	assert( memSpec.sizeSoFar() >= sizeof( shader_t ) );
	auto nameSpec = memSpec.add<char>( cleanName.size() + 1 );

	// TODO: Overload a global delete operator for material
	void *mem = malloc( memSpec.sizeSoFar() );
	if( !mem ) {
		throw std::bad_alloc();
	}

	// TODO: Should call constructors for all contained items
	std::memset( mem, 0, memSpec.sizeSoFar() );

	auto *s = new( mem )shader_t;
	// TODO... all this initialization belongs to a (not yet implemented) constructor
	s->type = (decltype( s->type ) )type;

	if( type >= SHADER_TYPE_BSP_MIN && type <= SHADER_TYPE_BSP_MAX ) {
		s->imagetags = IMAGE_TAG_WORLD;
	} else {
		s->imagetags = IMAGE_TAG_GENERIC;
	}

	// set defaults
	s->flags = SHADER_CULL_FRONT;
	s->vattribs = 0;
	s->glossIntensity = 0;
	s->glossExponent = 0;
	s->offsetmappingScale = 1;

	char *nameData = nameSpec.get( s );
	cleanName.copyTo( nameData, cleanName.size() + 1 );
	s->name = wsw::HashedStringView( nameData, cleanName.size(), cleanName.getHash(), wsw::StringView::ZeroTerminated );

	return s;
}

shader_t *MaterialCache::newDefaultVertexMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shader_t>();
	auto passSpec = memSpec.add<shaderpass_t>();

	shader_t *s = initMaterial( SHADER_TYPE_VERTEX, cleanName, memSpec );

	// vertex lighting
	s->flags = SHADER_DEPTHWRITE | SHADER_CULL_FRONT;
	s->vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT | VATTRIB_COLOR0_BIT;
	s->sort = SHADER_SORT_OPAQUE;
	s->numpasses = 1;
	auto *pass = s->passes = passSpec.get( s );

	pass->flags = GLSTATE_DEPTHWRITE;
	pass->tcgen = TC_GEN_BASE;
	pass->rgbgen.type = RGB_GEN_VERTEX;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->images[0] = findImage( name,  s ->flags, IT_SRGB );

	return s;
}

shader_t *MaterialCache::newDefaultDeluxeMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shader_t>();
	auto passSpec = memSpec.add<shaderpass_t>();
	shader_t *s = initMaterial( SHADER_TYPE_DELUXEMAP, cleanName, memSpec );

	// deluxemapping

    image_t *images[3] { nullptr, nullptr, nullptr };
	// TODO: Name or clean name?
	loadMaterial( images, name, 0, s->imagetags );

	s->flags = SHADER_DEPTHWRITE | SHADER_CULL_FRONT | SHADER_LIGHTMAP;
	s->vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT | VATTRIB_LMCOORDS0_BIT | VATTRIB_NORMAL_BIT | VATTRIB_SVECTOR_BIT;
	s->sort = SHADER_SORT_OPAQUE;
	s->numpasses = 1;
	s->passes = passSpec.get( s );

	auto *pass = &s->passes[0];
	pass->flags = GLSTATE_DEPTHWRITE;
	pass->tcgen = TC_GEN_BASE;
	pass->rgbgen.type = RGB_GEN_IDENTITY;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
	pass->images[0] = findImage( name, s->flags, IT_SRGB );
	pass->images[1] = images[0]; // normalmap
	pass->images[2] = images[1]; // glossmap
	pass->images[3] = images[2]; // decalmap

	return s;
}

shader_t *MaterialCache::newDefaultCoronaMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shader_t>();
	auto passSpec = memSpec.add<shaderpass_t>();

	shader_t *s = initMaterial( SHADER_TYPE_CORONA, cleanName, memSpec );

	s->vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT | VATTRIB_COLOR0_BIT;
	s->sort = SHADER_SORT_ADDITIVE;
	s->numpasses = 1;
	s->passes = passSpec.get( s );
	s->flags = SHADER_SOFT_PARTICLE;

	auto *pass = &s->passes[0];
	pass->flags = GLSTATE_SRCBLEND_ONE | GLSTATE_DSTBLEND_ONE;
	pass->rgbgen.type = RGB_GEN_VERTEX;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->tcgen = TC_GEN_BASE;
	pass->images[0] = findImage( wsw::StringView( "*corona" ), s->flags, IT_SPECIAL );

	return s;
}

shader_s *MaterialCache::newDefaultDiffuseMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shader_t>();
	auto passSpec = memSpec.add<shaderpass_t>();

	shader_s *s = initMaterial( SHADER_TYPE_DIFFUSE, cleanName, memSpec );

	image_t *materialImages[3];

	// load material images
	// TODO: Name or clean name?
	loadMaterial( materialImages, cleanName, 0, s->imagetags );

	s->flags = SHADER_DEPTHWRITE | SHADER_CULL_FRONT;
	s->vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT | VATTRIB_NORMAL_BIT;
	s->sort = SHADER_SORT_OPAQUE;
	s->numpasses = 1;
	s->passes = passSpec.get( s );

	auto *pass = &s->passes[0];
	pass->flags = GLSTATE_DEPTHWRITE;
	pass->rgbgen.type = RGB_GEN_IDENTITY;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->tcgen = TC_GEN_BASE;
	pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
	pass->images[0] = findImage( name, s->flags, IT_SRGB );
	pass->images[1] = materialImages[0]; // normalmap
	pass->images[2] = materialImages[1]; // glossmap
	pass->images[3] = materialImages[2]; // decalmap
	s->vattribs |= VATTRIB_SVECTOR_BIT | VATTRIB_NORMAL_BIT;

	return s;
}

shader_t *MaterialCache::newDefault2DLikeMaterial( int type, const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shader_t>();
	auto passSpec = memSpec.add<shaderpass_t>();

	shader_t *s = initMaterial( type, cleanName, memSpec );

	s->flags = 0;
	s->vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT | VATTRIB_COLOR0_BIT;
	s->sort = SHADER_SORT_ADDITIVE;
	s->numpasses = 1;
	s->passes = passSpec.get( s );

	auto *pass = &s->passes[0];
	pass->flags = GLSTATE_SRCBLEND_SRC_ALPHA | GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	pass->rgbgen.type = RGB_GEN_VERTEX;
	pass->alphagen.type = ALPHA_GEN_VERTEX;
	pass->tcgen = TC_GEN_BASE;
	if( type == SHADER_TYPE_2D_LINEAR ) {
		pass->images[0] = findImage( name, s->flags, IT_SPECIAL | IT_SYNC );
	} else if( type != SHADER_TYPE_2D_RAW ) {
		pass->images[0] = findImage( name, s->flags, IT_SPECIAL | IT_SYNC | IT_SRGB );
	}

	return s;
}

shader_s *MaterialCache::newOpaqueEnvMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shader_t>();
	auto passSpec = memSpec.add<shaderpass_t>();

	auto *s = initMaterial( SHADER_TYPE_OPAQUE_ENV, cleanName, memSpec );

	s->vattribs = VATTRIB_POSITION_BIT;
	s->sort = SHADER_SORT_OPAQUE;
	s->flags = SHADER_CULL_FRONT | SHADER_DEPTHWRITE;
	s->numpasses = 1;
	s->passes = passSpec.get( s );

	auto *pass = &s->passes[0];
	pass->flags = GLSTATE_DEPTHWRITE;
	pass->rgbgen.type = RGB_GEN_ENVIRONMENT;
	VectorClear( pass->rgbgen.args );
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->tcgen = TC_GEN_NONE;
	assert( rsh.whiteTexture );
	pass->images[0] = rsh.whiteTexture;

	return s;
}

shader_t *MaterialCache::newSkyBoxMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shader_t>();
	auto passSpec = memSpec.add<shaderpass_t>();

	shader_t *s = initMaterial( SHADER_TYPE_SKYBOX, cleanName, memSpec );

	s->vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT;
	s->sort = SHADER_SORT_SKY;
	s->flags = SHADER_CULL_FRONT | SHADER_SKY;
	s->numpasses = 1;
	s->passes = passSpec.get( s );

	auto *pass = &s->passes[0];
	pass->rgbgen.type = RGB_GEN_IDENTITY;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->tcgen = TC_GEN_BASE;
	pass->flags = SHADERPASS_SKYBOXSIDE;
	// the actual image will be picked at rendering time based on skyside number
	pass->images[0] = rsh.whiteTexture;

	return s;
}

shader_t *MaterialCache::newFogMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	MemSpecBuilder memSpec;
	memSpec.add<shaderpass_t>();

	auto *s = initMaterial( SHADER_TYPE_FOG, cleanName, memSpec );
	s->vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT;
	s->sort = SHADER_SORT_FOG;
	s->flags = SHADER_CULL_FRONT;
	s->numpasses = 0;
	return s;
}

shader_s *MaterialCache::loadMaterial( const wsw::HashedStringView &cleanName, const wsw::StringView &name, int type, TokenStream *stream ) {
	shader_s *result = nullptr;
	if( stream ) {
		MaterialParser parser( this, stream, name, cleanName, (shaderType_e)type );
		result = parser.exec();
	}

	if( !result ) {
		result = newDefaultMaterial( type, cleanName, name );
	}

	assert( result );
	result->id = getNextMaterialId();
	materialById[result->id] = result;
	return result;
}

shader_s *MaterialCache::newDefaultMaterial( int type, const wsw::HashedStringView &cleanName, const wsw::StringView &name ) {
	switch( type ) {
		case SHADER_TYPE_VERTEX:
			return newDefaultVertexMaterial( cleanName, name );
		case SHADER_TYPE_DELUXEMAP:
			return newDefaultDeluxeMaterial( cleanName, name );
		case SHADER_TYPE_CORONA:
			return newDefaultCoronaMaterial( cleanName, name );
		case SHADER_TYPE_DIFFUSE:
			return newDefaultDiffuseMaterial( cleanName, name );
		case SHADER_TYPE_2D:
		case SHADER_TYPE_2D_RAW:
		case SHADER_TYPE_VIDEO:
		case SHADER_TYPE_2D_LINEAR:
			return newDefault2DLikeMaterial( type, cleanName, name );
		case SHADER_TYPE_OPAQUE_ENV:
			return newOpaqueEnvMaterial( cleanName, name );
		case SHADER_TYPE_SKYBOX:
			return newSkyBoxMaterial( cleanName, name );
		case SHADER_TYPE_FOG:
			return newFogMaterial( cleanName, name );
	}

	return nullptr;
}

const wsw::String *MaterialCache::readFileContents( const char *filename ) {
	wsw::String &pathName = pathNameBuffer;
	pathName.clear();
	pathName.append( "scripts/" );
	pathName.append( filename );

	if ( r_showShaderCache && r_showShaderCache->integer ) {
		Com_Printf( "...loading '%s'\n", pathName.data() );
	}

	int fhandle = 0;
	// look for it in the filesystem or pack files
	// TODO: Use sane RAII wrappers
	int contentLength = FS_FOpenFile( pathName.data(), &fhandle, FS_READ );
	if( contentLength < 0 ) {
		return nullptr;
	}

	fileContentsBuffer.resize( contentLength + 1 );

	if( FS_Read( fileContentsBuffer.data(), contentLength, fhandle ) != contentLength ) {
		FS_FCloseFile( fhandle );
		return nullptr;
	}

	FS_FCloseFile( fhandle );

	// Put the terminating zero, this is not mandatory as tokens aren't supposed
	// to be zero terminated but allows printing contents using C-style facilities
	fileContentsBuffer[contentLength] = '\0';
	return &fileContentsBuffer;
}

MaterialCache::FileCache *MaterialCache::createFileCache( const char *filename ) {
	const wsw::String *fileContents = readFileContents( filename );
	if( !fileContents ) {
		return nullptr;
	}

	TokenSplitter splitter( fileContents->data(), fileContents->size() );

	fileTokenSpans.clear();

	uint32_t lineNum = 0;
	size_t numKeptChars = 0;
	while( !splitter.isAtEof() ) {
		while( auto maybeToken = splitter.fetchNextTokenInLine() ) {
			const auto &[off, len] = *maybeToken;
			fileTokenSpans.emplace_back( TokenSpan {(int)off, len, lineNum } );
			numKeptChars += len;
		}
		lineNum++;
	}

	MemSpecBuilder memSpec;
	auto headerSpec = memSpec.add<FileCache>();
	auto spansSpec = memSpec.add<TokenSpan>( fileTokenSpans.size() );
	auto contentsSpec = memSpec.add<char>( numKeptChars );

	auto *mem = (uint8_t *)::malloc( memSpec.sizeSoFar() );
	if( !mem ) {
		return nullptr;
	}

	auto *result = new( headerSpec.get( mem ) )FileCache();
	result->spans = spansSpec.get( mem );
	result->data = contentsSpec.get( mem );
	assert( !result->dataSize && !result->numSpans );

	// Copy spans and compactified data
	char *data = contentsSpec.get( mem );
	for( const auto &parsedSpan: fileTokenSpans ) {
		auto *copiedSpan = &result->spans[result->numSpans++];
		*copiedSpan = parsedSpan;
		copiedSpan->offset = result->dataSize;
		std::memcpy( data + copiedSpan->offset, fileContents->data() + parsedSpan.offset, parsedSpan.len );
		result->dataSize += parsedSpan.len;
		assert( parsedSpan.len == copiedSpan->len && parsedSpan.line == copiedSpan->line );
	}

	assert( result->numSpans == fileTokenSpans.size() );
	assert( result->dataSize == numKeptChars );

	return result;
}

void MaterialCache::addFileContents( const char *filename ) {
	if( FileCache *fileCache = createFileCache( filename ) ) {
		if( tryAddingFileCacheContents( fileCache ) ) {
			assert( !fileCache->next );
			fileCache->next = fileCacheHead;
			fileCacheHead = fileCache;
		} else {
			fileCache->~FileCache();
			free( fileCache );
		}
	}
}

bool MaterialCache::tryAddingFileCacheContents( const FileCache *fileCache ) {
	fileMaterialNames.clear();
	fileSourceSpans.clear();

	unsigned tokenNum = 0;
	TokenStream stream( fileCache->data, fileCache->spans, fileCache->numSpans );
	for(;;) {
		auto maybeNameToken = stream.getNextToken();
		if( !maybeNameToken ) {
			break;
		}

		tokenNum++;

		auto maybeNextToken = stream.getNextToken();
		if( !maybeNextToken ) {
			return false;
		}

		tokenNum++;
		const unsigned shaderSpanStart = tokenNum;

		auto nextToken = *maybeNextToken;
		if( nextToken.length() != 1 || nextToken[0] != '{' ) {
			// TODO: Include the line and the token in report
			// TODO: Use new logging facilities
			Com_Printf( S_COLOR_YELLOW "Expected an opening brace after the name\n" );
			return false;
		}

		// TODO: Count how many tokens are in the shader
		for( int depth = 1; depth; ) {
			if( auto maybeBlockToken = stream.getNextToken() ) {
				tokenNum++;
				auto blockToken = *maybeBlockToken;
				char ch = blockToken.maybeFront().value_or( '\0' );
				depth += ( ch == '{' ) ? +1 : 0;
				depth += ( ch == '}' ) ? -1 : 0;
			} else {
				Com_Printf( S_COLOR_YELLOW "Missing closing brace(s) at the end of file\n" );
				// TODO: Include the line and the token in report
				// TODO: Use new logging facilities
				return false;
			}
		}

		fileMaterialNames.emplace_back( *maybeNameToken );
		assert( tokenNum > shaderSpanStart );
		// Exclude the closing brace from the range
		fileSourceSpans.emplace_back( std::make_pair( shaderSpanStart, tokenNum - shaderSpanStart - 1 ) );
	}

	auto *mem = (uint8_t *)::malloc( sizeof( Source ) * fileMaterialNames.size() );
	if( !mem ) {
		return false;
	}

	auto *const firstInSameMemChunk = (Source *)mem;

	assert( fileMaterialNames.size() == fileSourceSpans.size() );
	for( size_t i = 0; i < fileMaterialNames.size(); ++i ) {
		auto *const source = new( mem )Source;
		mem += sizeof( Source );

		auto [from, len] = fileSourceSpans[i];
		source->tokenSpansOffset = from;
		source->numTokens = len;
		source->file = fileCache;
		source->firstInSameMemChunk = firstInSameMemChunk;
		source->name = wsw::HashedStringView( fileMaterialNames[i] );
		source->nextInList = sourcesHead;
		sourcesHead = source;

		auto binIndex = source->name.getHash() % kNumBins;
		source->nextInList = sourceBins[binIndex];
		sourceBins[binIndex] = source;
	}

	return true;
}

std::optional<MaterialCache::Source::Placeholders> MaterialCache::Source::preparePlaceholders() {
	std::vector<PlaceholderSpan> buffer;

	TokenSpan *tokenSpans = file->spans + this->tokenSpansOffset;
	const char *data = file->data;

	for( unsigned i = 0; i < numTokens; ++i ) {
		const auto &tokenSpan = tokenSpans[i];
		findPlaceholdersInToken( wsw::StringView( data + tokenSpan.offset, tokenSpan.len ), i, buffer );
	}

	return buffer.empty() ? std::nullopt : std::optional( buffer );
}

void MaterialCache::Source::findPlaceholdersInToken( const wsw::StringView &token, int tokenNum,
													 std::vector<PlaceholderSpan> &buffer ) {
	size_t index = 0;

	for(;; ) {
		auto *p = std::find( token.data() + index, token.data() + token.size(), '$' );
		if( p == token.data() + token.size() ) {
			return;
		}
		index = p - token.data() + 1;
		auto start = index;
		int num = 0;
		bool overflow = false;
		for(; index < token.length() && isdigit( token[index] ); index++ ) {
			if( !overflow ) {
				num = num * 10 + ( token[index] - '0' );
				if( num > std::numeric_limits<uint8_t>::max()) {
					overflow = true;
				}
			}
		}
		// If it was just a single dollar character
		if( !num ) {
			continue;
		}
		if( overflow ) {
			// TODO: Show a warning
		}
		auto len = index - start;
		PlaceholderSpan span = { (uint32_t)tokenNum, (uint16_t)start, (uint8_t)len, (uint8_t)num };
		buffer.emplace_back( span );
	}
}

MaterialCache::Source *MaterialCache::findSourceByName( const wsw::HashedStringView &name ) {
	auto binIndex = name.getHash() % kNumBins;
	for( Source *source = sourceBins[binIndex]; source; source = source->nextInBin ) {
		if( source->name.equalsIgnoreCase( name ) ) {
			return source;
		}
	}
	return nullptr;
}

MaterialLexer *MaterialCache::expandTemplate( const wsw::StringView &name, const wsw::StringView *args, size_t numArgs ) {
	Source *source = findSourceByName( name );
	if( !source ) {
		return nullptr;
	}

	expansionBuffer.clear();
	templateTokenSpans.clear();
	source->expandTemplate( args, numArgs, expansionBuffer, templateTokenSpans );

	if( !templateLexerHolder.empty() ) {
		templateLexerHolder.pop_back();
		templateTokenStreamHolder.pop_back();
	}

	void *streamMem = templateTokenStreamHolder.unsafe_grow_back();
	new( streamMem )TokenStream( source->file->data, templateTokenSpans.data(),templateTokenSpans.size(), expansionBuffer.data() );

	void *lexerMem = templateLexerHolder.unsafe_grow_back();
	new( lexerMem )MaterialLexer( templateTokenStreamHolder.begin() );

	return templateLexerHolder.begin();
}

bool MaterialCache::Source::expandTemplate( const wsw::StringView *args, size_t numArgs, wsw::String &expansionBuffer, std::vector<TokenSpan> &resultingTokens ) {
	if( !maybePlaceholders ) {
		preparePlaceholders();
		if( !maybePlaceholders ) {
			return false;
		}
	}

	const Placeholders &placeholders = *this->maybePlaceholders;

	const char *const tokensData = file->data;
	const TokenSpan *const tokenSpans = file->spans + this->tokenSpansOffset;

	int resultSize = 0;
	for( const auto &__restrict placeholder: placeholders ) {
		if( placeholder.argNum >= numArgs ) {
			return false;
		}
		resultSize -= placeholder.len;
		resultSize += args[placeholder.argNum].length();
	}

	expansionBuffer.reserve( resultSize );

	size_t lastTokenNum = 0;
	size_t offsetInLastToken = 0;
	// for each token that needs a substitution
	for( const auto &__restrict placeholder: placeholders ) {
		if( lastTokenNum != placeholder.tokenNum ) {
			// Copy token characters after last span
			// Create token bounds for copied characters
			// Add the newly created bounds to stream
			lastTokenNum = placeholder.tokenNum;
			offsetInLastToken = 0;
		}
		// Copy token characters before span
		// Create token bounds and add to stream
		for( int j = offsetInLastToken; j < placeholder.offset; ++j ) {
		}

		// Copy argument data and add to span
	}

	// Copy token characters after last span again and add token bounds
	// Copy all tokens after last bounds

	// TODO:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	return false;
}

class BuiltinTexMatcher {
	// We have to compose tokens based on a supplied name. Do that without touching heap.
	struct Chunk {
		char data[16];
	};

	StaticVector<Chunk, 12> stringChunks;

	using TexNumberViews = StaticVector<std::pair<wsw::StringView, BuiltinTexNumber>, 6>;

	TexNumberViews longTexNumbers;
	TexNumberViews shortTexNumbers;

	wsw::StringView makeView( const char *prefix, const char *name, const char *suffix ) {
		char *data = stringChunks.unsafe_grow_back()->data;
		Q_snprintfz( data, sizeof( Chunk::data ), "%s%s%s", prefix, name, suffix );
		return wsw::StringView( data );
	}

	void add( const char *name, BuiltinTexNumber texNumber ) noexcept {
		assert( std::strlen( name ) < sizeof( Chunk::data ) );

		longTexNumbers.emplace_back( std::make_pair( makeView( "$", name, "image" ), texNumber ) );
		shortTexNumbers.emplace_back( std::make_pair( makeView( "*", name, "" ), texNumber ) );
	}

	static std::optional<BuiltinTexNumber> matchInList( const wsw::StringView &image, const TexNumberViews &views ) {
		for( const auto &[name, texNum] : views ) {
			if( name.equalsIgnoreCase( image ) ) {
				return std::optional( texNum );
			}
		}
		return std::nullopt;
	}
public:
	BuiltinTexMatcher() noexcept {
		add( "white", BuiltinTexNumber::White );
		add( "black", BuiltinTexNumber::Black );
		add( "grey", BuiltinTexNumber::Grey );
		add( "blankbump", BuiltinTexNumber::BlankBump );
		add( "particle", BuiltinTexNumber::Particle );
		add( "corona", BuiltinTexNumber::Corona );
	}

	std::optional<BuiltinTexNumber> match( const wsw::StringView &image ) {
		// Try matching long tokens (they're more likely to be met in wsw assets)
		if( auto num = matchInList( image, longTexNumbers ) ) {
			return num;
		}
		return matchInList( image, shortTexNumbers );
	}
};

static BuiltinTexMatcher builtinTexMatcher;

static const wsw::StringView kLightmapPrefix( "*lm" );

image_t *MaterialCache::findImage( const wsw::StringView &name, int flags, int imageTags, int minMipSize ) {
	assert( minMipSize );

	if( auto maybeBuiltinTexNum = builtinTexMatcher.match( name ) ) {
		// TODO... add a builtin tex getter by num
	}

	if( kLightmapPrefix.equalsIgnoreCase( name.take( 3 ) ) ) {
	    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a stage with explicit lightmap image\n", shader->name );
		return rsh.whiteTexture;
	}

	auto image = R_FindImage( name, wsw::StringView(), flags, minMipSize, imageTags );
	if( !image ) {
	    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//Com_Printf( S_COLOR_YELLOW "WARNING: shader %s has a stage with no image: %s\n", shader->name, name.data() );
		return rsh.noTexture;
	}

	return image;
}

void MaterialCache::loadMaterial( image_t **images, const wsw::StringView &fullName, int addFlags, int imagetags, int minMipSize ) {
    // set defaults
    images[0] = images[1] = images[2] = nullptr;

    // load normalmap image
    images[0] = R_FindImage( fullName, kNormSuffix, ( addFlags | IT_NORMALMAP ), minMipSize, imagetags );

    // load glossmap image
    if( r_lighting_specular->integer ) {
        images[1] = R_FindImage( fullName, kGlossSuffix, addFlags, minMipSize, imagetags );
    }

    images[2] = R_FindImage( fullName, kDecalSuffix, addFlags, minMipSize, imagetags );
    if( !images[2] ) {
        images[2] = R_FindImage( fullName, kAddSuffix, addFlags, minMipSize, imagetags );
    }
}
