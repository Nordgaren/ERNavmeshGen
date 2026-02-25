using System.Runtime.InteropServices;

namespace ERNavmeshGenCS; 

public enum HavokOutputType {
    Packfile = 0,
    Tagfile = 1,
    XML = 2,
    ALL = 3,
};


public class HavokNavmeshNative {
    
    [DllImport("ERNavmeshGen.dll")]
    public static extern bool SetGameLocation([MarshalAs (UnmanagedType.LPStr)] string path);
    [DllImport("ERNavmeshGen.dll")]
    public static extern bool BatchGenerateNavMeshFromCollisionAPI([MarshalAs (UnmanagedType.LPStr)] string  path, [MarshalAs (UnmanagedType.LPStr)] string compendiumPath);
    [DllImport("ERNavmeshGen.dll")]
    public static extern bool GenerateNavMeshFromCollisionAPI([MarshalAs (UnmanagedType.LPStr)] string folder, [MarshalAs (UnmanagedType.LPStr)] string compendiumPath);
    
}
