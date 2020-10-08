#include <stdio.h>

#include "libsm64.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "include/PR/os_cont.h"
#include "engine/math_util.h"
#include "include/sm64.h"
#include "shim.h"
#include "game/mario.h"
#include "game/object_stuff.h"
#include "engine/surface_load.h"
#include "engine/surface_collision.h"
#include "engine/graph_node.h"
#include "engine/geo_layout.h"
#include "assets/mario_anim_data.h"
#include "game/rendering_graph_node.h"
#include "model/geo.inc.h"
#include "gfx_adapter.h"

static struct AllocOnlyPool *s_mario_geo_pool;
static struct GraphNode *s_mario_graph_node;

static void update_button( bool on, u16 button )
{
    gController.buttonPressed &= ~button;

    if( on )
    {
        if(( gController.buttonDown & button ) == 0 )
            gController.buttonPressed |= button;

        gController.buttonDown |= button;
    }
    else 
    {
        gController.buttonDown &= ~button;
    }
}

static struct Camera *hack_build_camera( void )
{
    struct Camera *result = malloc( sizeof( struct Camera ));
    memset( result, 0, sizeof( struct Camera ));
    return result;
}

static struct Area *hack_build_area( void )
{
    struct Area *result = malloc( sizeof( struct Area ));

    result->index = 0;
    result->flags = 1;
    result->terrainType = TERRAIN_GRASS;
    result->unk04 = NULL;
    result->terrainData = NULL;
    result->surfaceRooms = NULL;
    result->macroObjects = NULL;
    result->warpNodes = NULL;
    result->paintingWarpNodes = NULL;
    result->instantWarps = NULL;
    result->objectSpawnInfos = NULL;
    result->camera = hack_build_camera();
    result->unused28 = NULL;
    result->whirlpools[0] = NULL;
    result->whirlpools[1] = NULL;
    result->dialog[0] = 0;
    result->dialog[1] = 0;
    result->musicParam = 0;
    result->musicParam2 = 0;

    return result;
}

void sm64_global_init( SM64DebugPrintFunctionPtr debugPrintFunction )
{
    gDebugPrint = debugPrintFunction;

    gMarioObject = hack_allocate_mario();
    gCurrentArea = hack_build_area();
    gCurrentObject = gMarioObject;

    s_mario_geo_pool = alloc_only_pool_init();
    s_mario_graph_node = process_geo_layout( s_mario_geo_pool, mario_geo_ptr );

    D_80339D10.animDmaTable = (void*)(&gMarioAnims);
    D_80339D10.currentAnimAddr = NULL;
    D_80339D10.targetAnim = malloc( 0x4000 );

    DEBUG_LOG( "Mario animations loaded from address %lu", (uint64_t)D_80339D10.animDmaTable );
}

void sm64_load_surfaces( uint16_t terrainType, const struct SM64Surface *surfaceArray, size_t numSurfaces )
{
	surface_load_for_libsm64( surfaceArray, numSurfaces );
    gCurrentArea->terrainType = terrainType;
}

void sm64_mario_reset( int16_t marioX, int16_t marioY, int16_t marioZ )
{
    gMarioSpawnInfoVal.startPos[0] = marioX;
    gMarioSpawnInfoVal.startPos[1] = marioY;
    gMarioSpawnInfoVal.startPos[2] = marioZ;

    gMarioSpawnInfoVal.startAngle[0] = 0;
    gMarioSpawnInfoVal.startAngle[1] = 0;
    gMarioSpawnInfoVal.startAngle[2] = 0;

    gMarioSpawnInfoVal.areaIndex = 0;
    gMarioSpawnInfoVal.activeAreaIndex = 0;
    gMarioSpawnInfoVal.behaviorArg = 0;
    gMarioSpawnInfoVal.behaviorScript = NULL;
    gMarioSpawnInfoVal.unk18 = NULL;
    gMarioSpawnInfoVal.next = NULL;

    init_mario_from_save_file();
    init_mario();
    set_mario_action( gMarioState, ACT_SPAWN_SPIN_AIRBORNE, 0);
    find_floor( marioX, marioY, marioZ, &gMarioState->floor );
}

void sm64_mario_tick( const struct SM64MarioInputs *inputs, struct SM64MarioState *outState,  struct SM64MarioGeometryBuffers *outBuffers )
{
    update_button( inputs->buttonA, A_BUTTON );
    update_button( inputs->buttonB, B_BUTTON );
    update_button( inputs->buttonZ, Z_TRIG );

    gMarioState->area->camera->yaw = atan2s( inputs->camLookZ, inputs->camLookX );

    gController.stickX = -64.0f * inputs->stickX;
    gController.stickY = 64.0f * inputs->stickY;
    gController.stickMag = sqrtf( gController.stickX*gController.stickX + gController.stickY*gController.stickY );

    bhv_mario_update();

    gfx_adapter_bind_output_buffers( outBuffers );

    geo_process_root_hack_single_node( s_mario_graph_node );

    gAreaUpdateCounter++;

	outState->health = gMarioState->health;
	vec3f_copy( outState->position, gMarioState->pos );
	vec3f_copy( outState->velocity, gMarioState->vel );
	outState->faceAngle = (float)gMarioState->faceAngle[1] / 32768.0f * 3.14159f;
}

void sm64_global_terminate( void )
{
    // TODO free
}