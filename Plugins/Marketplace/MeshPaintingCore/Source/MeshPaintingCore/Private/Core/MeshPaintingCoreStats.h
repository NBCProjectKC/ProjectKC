// Copyright Shared Orbit 2026. All Rights Reserved.

#pragma once

#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("MeshPaintingCore"), STATGROUP_MeshPaintingCore, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Apply Paint"), STAT_MeshPaintingCore_ApplyPaint, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Update Brush Preview"), STAT_MeshPaintingCore_UpdateBrushPreview, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Trace Paint Under Cursor"), STAT_MeshPaintingCore_TracePaintUnderCursor, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Paint At Hit With Settings"), STAT_MeshPaintingCore_PaintAtHitWithSettings, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Resolve Paint Hit"), STAT_MeshPaintingCore_ResolvePaintHit, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Find Paint Hit UV"), STAT_MeshPaintingCore_FindPaintHitUV, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Find Skeletal Mesh Face UV"), STAT_MeshPaintingCore_FindSkeletalMeshFaceUV, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Collect Static Mesh UV Triangles"), STAT_MeshPaintingCore_CollectStaticMeshUVTriangles, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Collect Skeletal Mesh UV Triangles"), STAT_MeshPaintingCore_CollectSkeletalMeshUVTriangles, STATGROUP_MeshPaintingCore, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Draw Brush To Render Target"), STAT_MeshPaintingCore_DrawBrushToRenderTarget, STATGROUP_MeshPaintingCore, );

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Paint Calls"), STAT_MeshPaintingCore_PaintCalls, STATGROUP_MeshPaintingCore, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Successful Paint Calls"), STAT_MeshPaintingCore_SuccessfulPaintCalls, STATGROUP_MeshPaintingCore, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Render Target Draws"), STAT_MeshPaintingCore_RenderTargetDraws, STATGROUP_MeshPaintingCore, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("UV Island Triangles Scanned"), STAT_MeshPaintingCore_UVIslandTrianglesScanned, STATGROUP_MeshPaintingCore, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("GPU Brush Triangles Drawn"), STAT_MeshPaintingCore_GPUBrushTrianglesDrawn, STATGROUP_MeshPaintingCore, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Skeletal UV Fallback Triangles Scanned"), STAT_MeshPaintingCore_SkeletalUVFallbackTrianglesScanned, STATGROUP_MeshPaintingCore, );
