namespace ERNavmeshGenCS;

public class ERNavmeshGen : IDisposable {
    private string _erPath;
    // This is for the future, in case the DS3NavmeshGen.dll needs to be reloaded.  
    private IntPtr _navgen;
    public ERNavmeshGen(string path) {
        if (!SetupNavmeshDll(path))
        {
            throw new FileNotFoundException("Could not set Elden Ring path. Please pass a valid path to the constructor.");
        }
    }
    public ERNavmeshGen() {
        string? path = Util.TryGetGameInstallLocation($"\\steamapps\\common\\Elden Ring\\Game\\EldenRing.exe");
        if (path == null) {
            throw new FileNotFoundException("Could not find EldenRing.exe. Please pass a path to the constructor.");
        }

        if (!SetupNavmeshDll(path))
        {
            throw new FileNotFoundException("Could not set Elden Ring path.");
        }
    }
    public bool SetupNavmeshDll(string path) {
        _erPath = path;
        if (!path.EndsWith(".exe")) {
            throw new FileNotFoundException($"could not find {path}\\EldenRing.exe. Please provide a path to the \"Elden Ring\\Game\" folder.");
        }
        Kernel32.SetDllDirectory(Path.GetDirectoryName(path));
        
        if (!HavokNavmeshNative.SetGamePath(path))
        {
            return false;
        }
        
        return HavokNavmeshNative.Init();
    }
    public bool BatchGenerateNavmesh(string folderPath, string? outFolderPath, string? compendiumPath) {
        return HavokNavmeshNative.BatchGenerateNavMeshFromCollisionAPI(folderPath, outFolderPath, compendiumPath);
    }
    public bool GenerateNavmesh(string path, string? outPath, string? compendiumPath) {
        return HavokNavmeshNative.GenerateNavMeshFromCollisionAPI(path, outPath, compendiumPath);
    }
    public bool Close() {
        return HavokNavmeshNative.Close();
    }
    public void Dispose()
    {
        Close();
    }
    // Won't be useful until we can use the DLL fully from within C#
    private bool SetNavMeshGenerationSettings(hkaiNavMeshGenerationSnapshot settings)
    {
        HavokNavmeshNative.SetNavmeshGenerationSettings(ref settings);
        return true;
    }
    public hkaiNavMeshGenerationUtilsSettings GetDefaultNavMeshGenerationSettings()
    {
        HavokNavmeshNative.GetDefaultNavMeshGenerationSettings(out hkaiNavMeshGenerationUtilsSettings settings);
        return settings;
    }
    public static int GetSettingsStructSize()
    {
        return HavokNavmeshNative.GetSettingsStructSize();
    }
}
