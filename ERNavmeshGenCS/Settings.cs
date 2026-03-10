using System.Runtime.InteropServices;
using System.Text.Json.Serialization;

namespace ERNavmeshGenCS
{
    // --- Foundation Structs ---

    [StructLayout(LayoutKind.Sequential)]
    public struct hkArrayGeneric
    {
        public IntPtr data;
        public uint size;
        public uint capacityAndFlags;
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
        public IntPtr bag;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkReferencedObject_Data
    {
        public hkPropertyBag propertyBag;
        public ushort memSizeAndFlags;
        public ushort refCount;
        private uint _padding; // Pads the struct to 16 bytes for 8-byte alignment
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
    public enum EdgeMatchingMetric : int { ORDER_BY_OVERLAP = 1, ORDER_BY_DISTANCE = 2 }
    public enum ConstructionFlagsBits : int { MATERIAL_WALKABLE = 1, MATERIAL_CUTTING = 2, MATERIAL_WALKABLE_AND_CUTTING = 3 }
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
        private ushort _padding1; // Maintain alignment
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
        private byte _padding1;
        private ushort _padding2; 
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
        private ushort _padding1; 
        public float boundaryEdgeSplitLength;
        public float partitionBordersSplitLength;
        public float userVertexOnBoundaryTolerance;
        private uint _padding2; // Align next pointer to 8 bytes
        public hkArrayGeneric userVertices;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiNavMeshSimplificationUtils_Settings
    {
        // ... (Keep all the previous float/bool fields the same) ...
        public float maxBoundaryVertexVerticalError;
        [MarshalAs(UnmanagedType.I1)] public bool mergeLongestEdgesFirst;
        
        [JsonIgnore] private byte _padding5;
        [JsonIgnore] private ushort _padding6;
        
        public hkaiNavMeshSimplificationUtils_ExtraVertexSettings extraVertexSettings;
        
        [MarshalAs(UnmanagedType.I1)] public bool saveInputSnapshot;
        
        [JsonIgnore] private byte _padding7;
        [JsonIgnore] private ushort _padding8;
        [JsonIgnore] private uint _padding9;

        // CHANGED: string instead of IntPtr
        [MarshalAs(UnmanagedType.LPStr)] 
        public string snapshotFilename; 
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
        
        // ... (Keep all the primitive settings the same) ...

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
        
        // ... (Keep simplificationSettings and booleans the same) ...

        [MarshalAs(UnmanagedType.I1)] public bool saveInputSnapshot;
        
        [JsonIgnore] private ushort _padding8;
        [JsonIgnore] private uint _padding9; 

        // CHANGED: string instead of IntPtr
        [MarshalAs(UnmanagedType.LPStr)] 
        public string snapshotFilename; 
        
        [JsonIgnore] public hkArrayGeneric overrideSettings;
    }

    // --- The Final Snapshot Struct ---

    [StructLayout(LayoutKind.Sequential)]
    public struct hkaiNavMeshGenerationSnapshot
    {
        public hkGeometry geometry;
        public hkaiNavMeshGenerationUtilsSettings settings;
    }
}
