#include "API.h"

#include "export.h"
#include <string>

#include "HavokFunctions.h"
#include "NavGen.h"
#include "Util/fileio.h"
#include <nlohmann/json.hpp>
#include "globals.h"

#include "Util/JsonHelpers.h"

static std::string gamePath = "";

NAVMA_API bool Init()
{
    return Havok::init(gamePath);
}

NAVMA_API void SetNavmeshGenerationSettings(hkaiNavMeshGenerationSnapshot* snapshot)
{
    if (snapshot == nullptr)
        return;

    // Free existing string allocations to avoid memory leaks
    if (g_snapshot.settings.snapshotFilename)
    {
        free((void*)g_snapshot.settings.snapshotFilename);
    }
    if (g_snapshot.settings.simplificationSettings.snapshotFilename)
    {
        free((void*)g_snapshot.settings.simplificationSettings.snapshotFilename);
    }

    // Perform a shallow copy of all the primitive data
    g_snapshot = *snapshot;

    // Deep copy the strings (C# temporary pointers will be destroyed after this call)
    if (snapshot->settings.snapshotFilename)
    {
        g_snapshot.settings.snapshotFilename = _strdup(snapshot->settings.snapshotFilename);
    }
    
    if (snapshot->settings.simplificationSettings.snapshotFilename)
    {
        g_snapshot.settings.simplificationSettings.snapshotFilename = _strdup(snapshot->settings.simplificationSettings.snapshotFilename);
    }

    // Nullify unsupported dynamic structures to be safe
    g_snapshot.geometry.vftable = nullptr;
    g_snapshot.settings.vftable = nullptr;
    g_snapshot.settings.painterOverlapCallback = nullptr;

    // Zero out arrays
    g_snapshot.geometry.vertices = hkArrayGeneric();
    g_snapshot.geometry.triangles = hkArrayGeneric();
    g_snapshot.settings.carvers = hkArrayGeneric();
    g_snapshot.settings.painters = hkArrayGeneric();
    g_snapshot.settings.materialMap = hkArrayGeneric();
    g_snapshot.settings.overrideSettings = hkArrayGeneric();
    g_snapshot.settings.regionPruningSettings.regionSeedPoints = hkArrayGeneric();
    g_snapshot.settings.regionPruningSettings.regionConnections = hkArrayGeneric();
    g_snapshot.settings.simplificationSettings.extraVertexSettings.userVertices = hkArrayGeneric();
}

NAVMA_API bool SetGamePath(const char* path)
{
    gamePath = path;
    return true;
}

NAVMA_API bool GenerateNavMeshFromCollisionAPI(const char* path, const char* outFile,const char* compendiumPath)
{
    if (path == nullptr)
    {
        return false;
    }
    
    // if (!Havok::init(gamePath))
    // {
    //     PLOG_ERROR << "Failed to init Havok";
    //     return false;
    // }

    std::string colPath = path;
    std::string colOutPath = outFile;
    std::string compendium = compendiumPath ? compendiumPath : "";
	PLOG_INFO << "Generating navmesh from: " << colPath;
    if (!GenerateNavMeshFromCollision(colPath, colOutPath, compendium))
    {
        PLOG_INFO << "Navmesh failed to generate from" << colPath;
        return false;
    }
	PLOG_INFO << "Navmesh generated from" << colPath;

    return true;
}

static bool BatchGenerateNavMeshFromCollision(const std::string& folder, const std::string& outFolder, const char* compendiumPath) {
    if (folder.empty() || outFolder.empty()) {
        return false;
    }

    // if (!Havok::init(gamePath))
    // {
    //     PLOG_ERROR << "Failed to init Havok";
    //     return false;
    // }
    
    std::string compendium = compendiumPath ? compendiumPath : "";
    
    PLOG_INFO << "Generating navmesh from all files in " << folder;
    for (std::string file : fileIO::GetAllFilesInFolder(folder)) {
        if (ends_with(file, ".hkx")) {
            PLOG_INFO << "Generating navmesh from: " << file;
            std::filesystem::path pathIn { file };
            const std::string inFilename = "n" + pathIn.filename().string().substr(1);
            std::filesystem::path pathOut { outFolder };
            pathOut.replace_filename(inFilename);
             if (!GenerateNavMeshFromCollision(file, pathOut.string(), compendium)) {
                 PLOG_INFO << "Navmesh failed to generate from" << file;
                 return false;
             }
        }
    }

    PLOG_INFO << "Generated navmeshes from all files in " << folder;
    
    return true;
}


NAVMA_API bool BatchGenerateNavMeshFromCollisionAPI(const char* folder, const char* outFolder, const char* compendiumPath)
{
    if (folder == nullptr) {
        return false;
    }
    
    if (outFolder == nullptr || strlen(outFolder) == 0)
    {
        outFolder = folder;
    }
    
    std::string folderPath = folder;
    std::string outFolderPath = outFolder;
    return BatchGenerateNavMeshFromCollision(folderPath, outFolderPath, compendiumPath);
}

NAVMA_API void GetDefaultNavMeshGenerationSettings(hkaiNavMeshGenerationUtilsSettings* outSettings)
{
    if (outSettings == nullptr)
        return;

    // 1. Construct a local object on the C++ stack.
    // This safely initializes the vftable and sets all hkArray capacityAndFlags to 0x80000000.
    hkaiNavMeshGenerationUtilsSettings localSettings;

    // 2. Pass the valid, fully-initialized C++ object to Havok
    Havok::getDefaultNavMeshGenerationSettings(localSettings);

    // 3. Bit-copy the populated data directly into the C# memory block
    *outSettings = localSettings;

    // 4. Safely nullify the vftable before returning to C# 
    // (Passing a C++ vftable into managed memory can cause issues if the C# GC moves the struct)
    outSettings->vftable = nullptr;
}

// The main Deserialization Function
NAVMA_API void LoadSnapshotFromJson(const char* jsonString)
{
    using json = nlohmann::json;
    if (jsonString == nullptr) return;

    try 
    {
        json root = json::parse(jsonString);
        
        if (!root.contains("settings")) return;
        const nlohmann::json& jSettings = root["settings"];

        // --- 1. Map Primitives ---
        JsonHelpers::Read(jSettings, "characterHeight", g_snapshot.settings.characterHeight);
        JsonHelpers::Read(jSettings, "quantizationGridSize", g_snapshot.settings.quantizationGridSize);
        JsonHelpers::Read(jSettings, "maxWalkableSlope", g_snapshot.settings.maxWalkableSlope);
        JsonHelpers::Read(jSettings, "degenerateAreaThreshold", g_snapshot.settings.degenerateAreaThreshold);
        JsonHelpers::Read(jSettings, "degenerateWidthThreshold", g_snapshot.settings.degenerateWidthThreshold);
        JsonHelpers::Read(jSettings, "convexThreshold", g_snapshot.settings.convexThreshold);
        JsonHelpers::Read(jSettings, "maxNumEdgesPerFace", g_snapshot.settings.maxNumEdgesPerFace);
        JsonHelpers::Read(jSettings, "edgeConnectionIterations", g_snapshot.settings.edgeConnectionIterations);
        JsonHelpers::Read(jSettings, "smallBoundaryEdgeGroupRemoval", g_snapshot.settings.smallBoundaryEdgeGroupRemoval);
        JsonHelpers::Read(jSettings, "fixupOverlappingTriangles", g_snapshot.settings.fixupOverlappingTriangles);
        JsonHelpers::Read(jSettings, "swapOverlappingAndQuantization", g_snapshot.settings.swapOverlappingAndQuantization);
        JsonHelpers::Read(jSettings, "weldInputVertices", g_snapshot.settings.weldInputVertices);
        JsonHelpers::Read(jSettings, "weldThreshold", g_snapshot.settings.weldThreshold);
        JsonHelpers::Read(jSettings, "minCharacterWidth", g_snapshot.settings.minCharacterWidth);
        JsonHelpers::Read(jSettings, "maxCharacterWidth", g_snapshot.settings.maxCharacterWidth);
        JsonHelpers::Read(jSettings, "precalculateClearanceSeedingData", g_snapshot.settings.precalculateClearanceSeedingData);
        JsonHelpers::Read(jSettings, "enableSimplification", g_snapshot.settings.enableSimplification);
        JsonHelpers::Read(jSettings, "carvedMaterialDeprecated", g_snapshot.settings.carvedMaterialDeprecated);
        JsonHelpers::Read(jSettings, "carvedCuttingMaterialDeprecated", g_snapshot.settings.carvedCuttingMaterialDeprecated);
        JsonHelpers::Read(jSettings, "checkEdgeGeometryConsistency", g_snapshot.settings.checkEdgeGeometryConsistency);
        JsonHelpers::Read(jSettings, "saveInputSnapshot", g_snapshot.settings.saveInputSnapshot);

        // --- 2. Map Enums ---
        using Settings = hkaiNavMeshGenerationUtilsSettings;
        JsonHelpers::ReadEnum(jSettings, "triangleWinding", g_snapshot.settings.triangleWinding);
        JsonHelpers::ReadEnum(jSettings, "edgeMatchingMetric", g_snapshot.settings.edgeMatchingMetric);
        JsonHelpers::ReadEnum(jSettings, "defaultConstructionProperties", g_snapshot.settings.defaultConstructionProperties);
        JsonHelpers::ReadEnum(jSettings, "characterWidthUsage", g_snapshot.settings.characterWidthUsage);

        // --- 3. Map Strings ---
        JsonHelpers::ReadString(jSettings, "snapshotFilename", g_snapshot.settings.snapshotFilename);

        // --- 4. Map Nested Objects ---
        
        // hkVector4 up
        if (jSettings.contains("up")) {
            JsonHelpers::Read(jSettings["up"], "x", g_snapshot.settings.up.x);
            JsonHelpers::Read(jSettings["up"], "y", g_snapshot.settings.up.y);
            JsonHelpers::Read(jSettings["up"], "z", g_snapshot.settings.up.z);
            JsonHelpers::Read(jSettings["up"], "w", g_snapshot.settings.up.w);
        }

        // hkAabb boundsAabb
        if (jSettings.contains("boundsAabb")) {
            const auto& jAabb = jSettings["boundsAabb"];
            if (jAabb.contains("min")) {
                JsonHelpers::Read(jAabb["min"], "x", g_snapshot.settings.boundsAabb.min.x);
                JsonHelpers::Read(jAabb["min"], "y", g_snapshot.settings.boundsAabb.min.y);
                JsonHelpers::Read(jAabb["min"], "z", g_snapshot.settings.boundsAabb.min.z);
                JsonHelpers::Read(jAabb["min"], "w", g_snapshot.settings.boundsAabb.min.w);
            }
            if (jAabb.contains("max")) {
                JsonHelpers::Read(jAabb["max"], "x", g_snapshot.settings.boundsAabb.max.x);
                JsonHelpers::Read(jAabb["max"], "y", g_snapshot.settings.boundsAabb.max.y);
                JsonHelpers::Read(jAabb["max"], "z", g_snapshot.settings.boundsAabb.max.z);
                JsonHelpers::Read(jAabb["max"], "w", g_snapshot.settings.boundsAabb.max.w);
            }
        }

        // RegionPruningSettings
        if (jSettings.contains("regionPruningSettings")) {
            const auto& jPrune = jSettings["regionPruningSettings"];
            JsonHelpers::Read(jPrune, "minRegionArea", g_snapshot.settings.regionPruningSettings.minRegionArea);
            JsonHelpers::Read(jPrune, "minDistanceToSeedPoints", g_snapshot.settings.regionPruningSettings.minDistanceToSeedPoints);
            JsonHelpers::Read(jPrune, "borderPreservationTolerance", g_snapshot.settings.regionPruningSettings.borderPreservationTolerance);
            JsonHelpers::Read(jPrune, "preserveVerticalBorderRegions", g_snapshot.settings.regionPruningSettings.preserveVerticalBorderRegions);
            JsonHelpers::Read(jPrune, "pruneBeforeTriangulation", g_snapshot.settings.regionPruningSettings.pruneBeforeTriangulation);
            // JsonHelpers::ClearArray(g_snapshot.settings.regionPruningSettings.regionSeedPoints);
            // JsonHelpers::ClearArray(g_snapshot.settings.regionPruningSettings.regionConnections);
        }

        // WallClimbingSettings
        if (jSettings.contains("wallClimbingSettings")) {
            const auto& jWall = jSettings["wallClimbingSettings"];
            JsonHelpers::Read(jWall, "enableWallClimbing", g_snapshot.settings.wallClimbingSettings.enableWallClimbing);
            JsonHelpers::Read(jWall, "excludeWalkableFaces", g_snapshot.settings.wallClimbingSettings.excludeWalkableFaces);
        }

        // hkaiNavMeshEdgeMatchingParameters
        if (jSettings.contains("edgeMatchingParams")) {
            const auto& jEdge = jSettings["edgeMatchingParams"];
            JsonHelpers::Read(jEdge, "maxStepHeight", g_snapshot.settings.edgeMatchingParams.maxStepHeight);
            JsonHelpers::Read(jEdge, "maxSeparation", g_snapshot.settings.edgeMatchingParams.maxSeparation);
            JsonHelpers::Read(jEdge, "maxOverhang", g_snapshot.settings.edgeMatchingParams.maxOverhang);
            // ... Add the remaining edge mapping fields here if you use them heavily ...
            JsonHelpers::Read(jEdge, "useSafeEdgeTraversibilityHorizontalEpsilon", g_snapshot.settings.edgeMatchingParams.useSafeEdgeTraversibilityHorizontalEpsilon);
        }

        // hkaiOverlappingTriangles::Settings
        if (jSettings.contains("overlappingTrianglesSettings")) {
            const auto& jOver = jSettings["overlappingTrianglesSettings"];
            JsonHelpers::Read(jOver, "coplanarityTolerance", g_snapshot.settings.overlappingTrianglesSettings.coplanarityTolerance);
            JsonHelpers::Read(jOver, "raycastLengthMultiplier", g_snapshot.settings.overlappingTrianglesSettings.raycastLengthMultiplier);
            JsonHelpers::ReadEnum(jOver, "walkableTriangleSettings", g_snapshot.settings.overlappingTrianglesSettings.walkableTriangleSettings);
        }

        // hkaiNavMeshSimplificationUtils::Settings
        if (jSettings.contains("simplificationSettings")) {
            const auto& jSimp = jSettings["simplificationSettings"];
            JsonHelpers::Read(jSimp, "maxBorderSimplifyArea", g_snapshot.settings.simplificationSettings.maxBorderSimplifyArea);
            JsonHelpers::Read(jSimp, "useHeightPartitioning", g_snapshot.settings.simplificationSettings.useHeightPartitioning);
            JsonHelpers::Read(jSimp, "mergeLongestEdgesFirst", g_snapshot.settings.simplificationSettings.mergeLongestEdgesFirst);
            JsonHelpers::Read(jSimp, "saveInputSnapshot", g_snapshot.settings.simplificationSettings.saveInputSnapshot);
            
            // Nested string inside SimplificationUtils
            JsonHelpers::ReadString(jSimp, "snapshotFilename", g_snapshot.settings.simplificationSettings.snapshotFilename);

            // Nested ExtraVertexSettings
            if (jSimp.contains("extraVertexSettings")) {
                const auto& jExtra = jSimp["extraVertexSettings"];
                JsonHelpers::ReadEnum(jExtra, "vertexSelectionMethod", g_snapshot.settings.simplificationSettings.extraVertexSettings.vertexSelectionMethod);
                JsonHelpers::Read(jExtra, "vertexFraction", g_snapshot.settings.simplificationSettings.extraVertexSettings.vertexFraction);
                JsonHelpers::Read(jExtra, "addVerticesOnBoundaryEdges", g_snapshot.settings.simplificationSettings.extraVertexSettings.addVerticesOnBoundaryEdges);
                // JsonHelpers::ClearArray(g_snapshot.settings.simplificationSettings.extraVertexSettings.userVertices);
            }
        }

        // Nullify all unused dynamic items & Arrays ---
        // g_snapshot.geometry.vftable = nullptr;
        // g_snapshot.settings.vftable = nullptr;
        // g_snapshot.settings.painterOverlapCallback = nullptr;
        //
        // JsonHelpers::ClearArray(g_snapshot.geometry.vertices);
        // JsonHelpers::ClearArray(g_snapshot.geometry.triangles);
        // JsonHelpers::ClearArray(g_snapshot.settings.carvers);
        // JsonHelpers::ClearArray(g_snapshot.settings.painters);
        // JsonHelpers::ClearArray(g_snapshot.settings.materialMap);
        // JsonHelpers::ClearArray(g_snapshot.settings.overrideSettings);

    }
    catch (const json::exception& e)
    {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    }
}

NAVMA_API bool Close()
{
    HavokFunctions::denit();
    return true;
}

