
/* marker */
#define PM_SMD_C

/* dependencies */
#include "picointernal.h"

/* disable warnings */
#ifdef _WIN32
#pragma warning( disable:4100 )		/* unref param */
#endif

/* this holds temporary vertex data read by parser */
typedef struct SObjVertexData
{
	picoVec3_t v;           /* geometric vertices */
	picoVec2_t vt;          /* texture vertices */
	picoVec3_t vn;          /* vertex normals (optional) */
}
TObjVertexData;

/* _smd_canload:
 *  validates a source smd model file.
 */
static int _smd_canload( PM_PARAMS_CANLOAD ) {
	picoParser_t *p;

	// Only check file extension, since smd is a stop gap for us
	if( _pico_stristr( fileName, ".smd" ) != NULL ) {
		return PICO_PMV_OK;
	}

	return PICO_PMV_ERROR;
}

/* SizeObjVertexData:
 *   This pretty piece of 'alloc ahead' code dynamically
 *   allocates - and reallocates as soon as required -
 *   my vertex data array in even steps.
 */
#define SIZE_OBJ_STEP  4096

static TObjVertexData *SizeObjVertexData(
	TObjVertexData *vertexData, int reqEntries,
	int *entries, int *allocated ) {
	int newAllocated;

	/* sanity checks */
	if( reqEntries < 1 ) {
		return NULL;
	}
	if( entries == NULL || allocated == NULL ) {
		return NULL; /* must have */

	}
	/* no need to grow yet */
	if( vertexData && ( reqEntries < *allocated ) ) {
		*entries = reqEntries;
		return vertexData;
	}
	/* given vertex data ptr not allocated yet */
	if( vertexData == NULL ) {
		/* how many entries to allocate */
		newAllocated = ( reqEntries > SIZE_OBJ_STEP ) ?
			reqEntries : SIZE_OBJ_STEP;

		/* throw out an extended debug message */
#ifdef DEBUG_PM_OBJ_EX
		printf( "SizeObjVertexData: allocate (%d entries)\n",
			newAllocated );
#endif
		/* first time allocation */
		vertexData = (TObjVertexData *)
			_pico_alloc( sizeof( TObjVertexData ) * newAllocated );

		/* allocation failed */
		if( vertexData == NULL ) {
			return NULL;
		}

		/* allocation succeeded */
		*allocated = newAllocated;
		*entries = reqEntries;
		return vertexData;
	}
	/* given vertex data ptr needs to be resized */
	if( reqEntries == *allocated ) {
		newAllocated = ( *allocated + SIZE_OBJ_STEP );

		/* throw out an extended debug message */
#ifdef DEBUG_PM_OBJ_EX
		printf( "SizeObjVertexData: reallocate (%d entries)\n",
			newAllocated );
#endif
		/* try to reallocate */
		vertexData = (TObjVertexData *)
			_pico_realloc( (void *) &vertexData,
				sizeof( TObjVertexData ) * ( *allocated ),
				sizeof( TObjVertexData ) * ( newAllocated ) );

		/* reallocation failed */
		if( vertexData == NULL ) {
			return NULL;
		}

		/* reallocation succeeded */
		*allocated = newAllocated;
		*entries = reqEntries;
		return vertexData;
	}
	/* we're b0rked when we reach this */
	return NULL;
}

static void FreeObjVertexData( TObjVertexData *vertexData ) {
	if( vertexData != NULL ) {
		free( (TObjVertexData *) vertexData );
	}
}

/* _smd_load:
 *  loads a source smd model file.
 */
static picoModel_t *_smd_load( PM_PARAMS_LOAD )
{
	TObjVertexData *vertexData = NULL;
	picoModel_t    *model;
	picoSurface_t  *curSurface = NULL;
	picoParser_t   *p;
	int allocated;
	int entries;
	int numVerts = 0;
	int numNormals = 0;
	int numUVs = 0;
	int curVertex = 0;
	int curFace = 0;

	int allocatingTris = 0;

	printf( "LOADING SMD: %s", fileName );

	/* helper */
#define _obj_error_return( m ) \
	{ \
		_pico_printf( PICO_ERROR, "%s in SMD, line %d.", m, p->curLine ); \
		_pico_free_parser( p );	\
		FreeObjVertexData( vertexData ); \
		PicoFreeModel( model );	\
		return NULL; \
	}

	// Allocate new parser
	p = _pico_new_parser( (const picoByte_t *) buffer, bufSize );
	if( p == NULL ) {
		return NULL;
	}

	// Create a new pico model
	model = PicoNewModel();
	if( model == NULL ) {
		_pico_free_parser( p );
		return NULL;
	}

	// Setup model
	PicoSetModelFrameNum( model, frameNum );
	PicoSetModelName( model, fileName );
	PicoSetModelFileName( model, fileName );

	// Parse SMDs line by line
	while( 1 )
	{
		// Get the first token on the line
		if( _pico_parse_first( p ) == NULL )
		{
			break;
		}

		// Skip empty lines
		if( p->token == NULL || !strlen( p->token ) )
		{
			continue;
		}

		// Skip comment lines
		if( !_pico_stricmp( p->token, "//" ) )
		{
			_pico_parse_skip_rest( p );
			continue;
		}

		// Find when we enter a new section
		if( !_pico_stricmp( p->token, "nodes" ) )
		{
			// @todo
			_pico_parse_skip_rest( p );
			continue;
		}
		if( !_pico_stricmp( p->token, "skeleton" ) )
		{
			// @todo
			_pico_parse_skip_rest( p );
			continue;
		}
		if( !_pico_stricmp( p->token, "triangles" ) )
		{
			allocatingTris = 1;
			_pico_parse_skip_rest( p );
			continue;
		}

		// Find when the section ends
		if( !_pico_stricmp( p->token, "end" ) )
		{
			allocatingTris = 0;
			_pico_parse_skip_rest( p );
			continue;
		}

		if( allocatingTris )
		{
			// @todo: figure out if theres a better way to check if the token is a number
			int tokenVal = p->token[0] - '0';
			if( tokenVal >= 0 && tokenVal < 10 )
			{
				TObjVertexData *data;

				// Ignore the parent idx
				// @nothing

				// Parse the vertex
				picoVec3_t v;
				vertexData = SizeObjVertexData( vertexData, numVerts + 1, &entries, &allocated );
				if( vertexData == NULL )
				{
					_obj_error_return( "Realloc of vertex data failed (1)" );
				}
				data = &vertexData[numVerts++];
				if( !_pico_parse_vec( p, v ) )
				{
					_obj_error_return( "Vertex parse error" );
				}
				_pico_copy_vec( v, data->v );

				// Parse the vertex normal
				picoVec3_t n;
				vertexData = SizeObjVertexData( vertexData, numNormals + 1, &entries, &allocated );
				if( vertexData == NULL )
				{
					_obj_error_return( "Realloc of vertex data failed (3)" );
				}
				data = &vertexData[numNormals++];
				if( !_pico_parse_vec( p, n ) )
				{
					_obj_error_return( "Vertex normal parse error" );
				}
				_pico_copy_vec( n, data->vn );

				// Parse the vertex UV
				picoVec2_t coord;
				vertexData = SizeObjVertexData( vertexData, numUVs + 1, &entries, &allocated );
				if( vertexData == NULL )
				{
					_obj_error_return( "Realloc of vertex data failed (2)" );
				}
				data = &vertexData[numUVs++];
				if( !_pico_parse_vec2( p, coord ) ) {
					_obj_error_return( "UV coord parse error" );
				}
				_pico_copy_vec2( coord, data->vt );

				// @note: ignore the rest of the line, we don't need the bone links or weights
				_pico_parse_skip_rest( p );
				continue;
			}
			else
			{
				// Found a material name, so we're either starting the triangles or have just parsed a triangle
				if( curSurface == NULL )
				{
					curSurface = PicoNewSurface( model );
					if( curSurface == NULL )
					{
						_obj_error_return( "Error allocating surface" );
					}
					curFace = 0;
					curVertex = 0;
					PicoSetSurfaceType( curSurface, PICO_TRIANGLES );
					PicoSetSurfaceName( curSurface, p->token );
				}
				else
				{
					int max = 3;

					/* assign all surface information */
					for( int i = 0; i < max; i++ )
					{
						TObjVertexData *data;
						data = &vertexData[numVerts - (max - i)];

						PicoSetSurfaceXYZ( curSurface, ( curVertex + i ), data->v );
						PicoSetSurfaceST( curSurface, 0, ( curVertex + i ), data->vt );
						PicoSetSurfaceNormal( curSurface, ( curVertex + i ), data->vn );
					}

					/* add our triangle (A B C) */
					PicoSetSurfaceIndex( curSurface, ( curFace * 3 + 2 ), (picoIndex_t) ( curVertex + 0 ) );
					PicoSetSurfaceIndex( curSurface, ( curFace * 3 + 1 ), (picoIndex_t) ( curVertex + 1 ) );
					PicoSetSurfaceIndex( curSurface, ( curFace * 3 + 0 ), (picoIndex_t) ( curVertex + 2 ) );
					curFace++;

					/* increase vertex count */
					curVertex += max;
				}
				continue;
			}

		}

		// @todo: handle materials
		/*
		if( !_pico_stricmp( p->token, "usemtl" ) ) {
			char *materialName;
			materialName = _pico_parse( p, 0 );
			if( materialName && strlen( materialName ) ) {
				picoShader_t *shader;
				shader = PicoFindShader( model, materialName, 0 );
				if( !shader ) {
					shader = PicoNewShader( model );
					if( shader ) {
						PicoSetShaderMapName( shader, materialName );
						PicoSetShaderName( shader, materialName );
					}
				}
				if( shader && curSurface ) {
					PicoSetSurfaceShader( curSurface, shader );
				}
			}
		}
		*/

		// Skip unparsed rest of line and continue
		_pico_parse_skip_rest( p );
	}

	// Free memory used by temporary vertexdata
	FreeObjVertexData( vertexData );

	// Return allocated pico model
	return model;
}

// pico file format module definition
const picoModule_t picoModuleSMD =
{
	"0.1", // module version string
	"Source SMD", // module display name 
	"James Wilkinson", // author's name
	"2019 James Wilkinson", // module copyright
	{
		"smd", NULL, NULL, NULL // default extensions to use
	},
	_smd_canload, // validation routine
	_smd_load, // load routine
	NULL, // save validation routine
	NULL // save routine
};
