using System.Runtime.InteropServices;
using System.Text.Json.Serialization;

namespace ERNavmeshGenCS
{
    // --- Foundation Structs ---

    [StructLayout(LayoutKind.Sequential)]
    public struct hkArrayGeneric
    {
        [JsonIgnore] public IntPtr data;
        [JsonIgnore] public uint size;
        [JsonIgnore] public uint capacityAndFlags;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkVector4
    {
        public float x;
        public float y;
        public float z;
        public float w;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkAabb
    {
        public hkVector4 min;
        public hkVector4 max;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkPropertyBag
    {
        [JsonIgnore] public IntPtr bag;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkReferencedObject_Data
    {
        [JsonIgnore] public hkPropertyBag propertyBag;
        [JsonIgnore] public ushort memSizeAndFlags;
        [JsonIgnore] public ushort refCount;
        
        [JsonIgnore] private uint _padding; 
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkGeometry
    {
        [JsonIgnore] public IntPtr vftable;
        [JsonIgnore] public hkReferencedObject_Data super_hkReferencedObject;
        [JsonIgnore] public hkArrayGeneric vertices;
        [JsonIgnore] public hkArrayGeneric triangles;
    }

    // --- Enums ---

    public enum TriangleWinding : int { WINDING_CCW = 0, WINDING_CW = 1 }
    public enum EdgeMatchingMetric : int {NONE = 0, ORDER_BY_OVERLAP = 1, ORDER_BY_DISTANCE = 2 }
    public enum ConstructionFlagsBits : int {NONE = 0, MATERIAL_WALKABLE = 1, MATERIAL_CUTTING = 2, MATERIAL_WALKABLE_AND_CUTTING = 3 }
    public enum CharacterWidthUsage : int { NONE = 0, BLOCK_EDGES = 1, SHRINK_NAV_MESH = 2 }
    public enum WalkableTriangleSettings : int { ONLY_FIX_WALKABLE = 0, PREFER_WALKABLE = 1, PREFER_UNWALKABLE = 2 }
    public enum VertexSelectionMethod : int { PROPORTIONAL_TO_AREA = 0, PROPORTIONAL_TO_VERTICES = 1 }

    // --- Nested Settings Structs ---

    [StructLayout(LayoutKind.Sequential)]
    public struct RegionPruningSettings
    {
        public float minRegionArea;
        public float minDistanceToSeedPoints;
        public float borderPreservationTolerance;
        
        [MarshalAs(UnmanagedType.I1)] public bool preserveVerticalBorderRegions;
        [MarshalAs(UnmanagedType.I1)] public bool pruneBeforeTriangulation;
        
        [JsonIgnore] private ushort _padding1; 
        
        [JsonIgnore] public hkArrayGeneric regionSeedPoints;
        [JsonIgnore] public hkArrayGeneric regionConnections;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct WallClimbingSettings
    {
        [MarshalAs(UnmanagedType.I1)] public bool enableWallClimbing;
        [MarshalAs(UnmanagedType.I1)] public bool excludeWalkableFaces;
        
        [JsonIgnore] private ushort _padding1; 
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiOverlappingTriangles_Settings
    {
        public float coplanarityTolerance;
        public float raycastLengthMultiplier;
        public WalkableTriangleSettings walkableTriangleSettings;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiNavMeshEdgeMatchingParameters
    {
        public float maxStepHeight;
        public float maxSeparation;
        public float maxOverhang;
        public float behindFaceTolerance;
        public float cosPlanarAlignmentAngle;
        public float cosVerticalAlignmentAngle;
        public float minEdgeOverlap;
        public float edgeTraversibilityHorizontalEpsilon;
        public float edgeTraversibilityVerticalEpsilon;
        public float cosClimbingFaceNormalAlignmentAngle;
        public float cosClimbingEdgeAlignmentAngle;
        public float minAngleBetweenFaces;
        public float edgeParallelTolerance;
        
        [MarshalAs(UnmanagedType.I1)] public bool useSafeEdgeTraversibilityHorizontalEpsilon;
        
        [JsonIgnore] private byte _padding1;
        [JsonIgnore] private ushort _padding2; 
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiNavMeshSimplificationUtils_ExtraVertexSettings
    {
        public VertexSelectionMethod vertexSelectionMethod;
        public float vertexFraction;
        public float areaFraction;
        public float minPartitionArea;
        public int numSmoothingIterations;
        public float iterationDamping;
        
        [MarshalAs(UnmanagedType.I1)] public bool addVerticesOnBoundaryEdges;
        [MarshalAs(UnmanagedType.I1)] public bool addVerticesOnPartitionBorders;
        
        [JsonIgnore] private ushort _padding1; 
        
        public float boundaryEdgeSplitLength;
        public float partitionBordersSplitLength;
        public float userVertexOnBoundaryTolerance;
        
        [JsonIgnore] private uint _padding2; 
        
        [JsonIgnore] public hkArrayGeneric userVertices;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiNavMeshSimplificationUtils_Settings
    {
        public float maxBorderSimplifyArea;
        public float maxConcaveBorderSimplifyArea;
        public float minCorridorWidth;
        public float maxCorridorWidth;
        public float holeReplacementArea;
        public float aabbReplacementAreaFraction;
        public float maxLoopShrinkFraction;
        public float maxBorderHeightError;
        public float maxBorderDistanceError;
        public int maxPartitionSize;
        
        [MarshalAs(UnmanagedType.I1)] public bool useHeightPartitioning;
        
        [JsonIgnore] private byte _padding1;
        [JsonIgnore] private ushort _padding2;
        
        public float maxPartitionHeightError;
        
        [MarshalAs(UnmanagedType.I1)] public bool useConservativeHeightPartitioning;
        
        [JsonIgnore] private byte _padding3;
        [JsonIgnore] private ushort _padding4;
        
        public float hertelMehlhornHeightError;
        public float cosPlanarityThreshold;
        public float nonconvexityThreshold;
        public float boundaryEdgeFilterThreshold;
        public float maxSharedVertexHorizontalError;
        public float maxSharedVertexVerticalError;
        public float maxBoundaryVertexHorizontalError;
        public float maxBoundaryVertexVerticalError;
        
        [MarshalAs(UnmanagedType.I1)] public bool mergeLongestEdgesFirst;
        
        [JsonIgnore] private byte _padding5;
        [JsonIgnore] private ushort _padding6;
        
        public hkaiNavMeshSimplificationUtils_ExtraVertexSettings extraVertexSettings;
        
        [MarshalAs(UnmanagedType.I1)] public bool saveInputSnapshot;
        
        [JsonIgnore] private byte _padding7;
        [JsonIgnore] private ushort _padding8;
        [JsonIgnore] private uint _padding9;
        
        [MarshalAs(UnmanagedType.LPStr)] public string snapshotFilename; 
    }

    // --- Main Settings Struct ---

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiNavMeshGenerationUtilsSettings
    {
        [JsonIgnore] public IntPtr vftable;
        [JsonIgnore] public hkReferencedObject_Data super_hkReferencedObject;
        public float characterHeight;
        
        [JsonIgnore] private uint _vectorAlignPadding; 

        public hkVector4 up;
        public float quantizationGridSize;
        public float maxWalkableSlope;
        public TriangleWinding triangleWinding;
        public float degenerateAreaThreshold;
        public float degenerateWidthThreshold;
        public float convexThreshold;
        public int maxNumEdgesPerFace;
        public hkaiNavMeshEdgeMatchingParameters edgeMatchingParams;
        public EdgeMatchingMetric edgeMatchingMetric;
        public int edgeConnectionIterations;
        
        [MarshalAs(UnmanagedType.I1)] public bool smallBoundaryEdgeGroupRemoval;
        
        [JsonIgnore] private byte _padding1;
        [JsonIgnore] private ushort _padding2;
        
        public RegionPruningSettings regionPruningSettings;
        public WallClimbingSettings wallClimbingSettings;
        
        [JsonIgnore] private uint _paddingAabbAlignment; 
        
        public hkAabb boundsAabb;
        
        [JsonIgnore] public hkArrayGeneric carvers;
        [JsonIgnore] public hkArrayGeneric painters;
        [JsonIgnore] public IntPtr painterOverlapCallback; 
        
        public ConstructionFlagsBits defaultConstructionProperties;
        
        [JsonIgnore] private uint _padding3;
        [JsonIgnore] public hkArrayGeneric materialMap;
        
        [MarshalAs(UnmanagedType.I1)] public bool fixupOverlappingTriangles;
        
        [JsonIgnore] private byte _padding4;
        [JsonIgnore] private ushort _padding5;
        
        public hkaiOverlappingTriangles_Settings overlappingTrianglesSettings;
        
        [MarshalAs(UnmanagedType.I1)] public bool swapOverlappingAndQuantization;
        [MarshalAs(UnmanagedType.I1)] public bool weldInputVertices;
        
        [JsonIgnore] private ushort _padding6;
        
        public float weldThreshold;
        public float minCharacterWidth;
        public CharacterWidthUsage characterWidthUsage;
        public float maxCharacterWidth;
        
        [MarshalAs(UnmanagedType.I1)] public bool precalculateClearanceSeedingData;
        [MarshalAs(UnmanagedType.I1)] public bool enableSimplification;
        
        [JsonIgnore] private ushort _padding7;
        
        public hkaiNavMeshSimplificationUtils_Settings simplificationSettings;
        public int carvedMaterialDeprecated;
        public int carvedCuttingMaterialDeprecated;
        
        [MarshalAs(UnmanagedType.I1)] public bool checkEdgeGeometryConsistency;
        [MarshalAs(UnmanagedType.I1)] public bool saveInputSnapshot;
        
        [JsonIgnore] private ushort _padding8;
        [JsonIgnore] private uint _padding9; 
        
        [MarshalAs(UnmanagedType.LPStr)] public string snapshotFilename; 
        
        [JsonIgnore] public hkArrayGeneric overrideSettings;
    }

    // --- The Final Snapshot Struct ---

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiNavMeshGenerationSnapshot
    {
        public hkGeometry geometry;
        public hkaiNavMeshGenerationUtilsSettings settings;
        
        public hkaiNavMeshGenerationSnapshot()
        {
            HavokNavmeshNative.GetDefaultNavMeshGenerationSettings(out hkaiNavMeshGenerationUtilsSettings defaultSnapshot);

            settings = defaultSnapshot;
        }
    }
}
