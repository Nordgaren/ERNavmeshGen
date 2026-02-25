namespace ERNavmeshGenCS;

public class ERNavmeshGen : IDisposable {
    private string _erPath;
    // This is for the future, in case the DS3NavmeshGen.dll needs to be reloaded.  
    private IntPtr _navgen;
    public ERNavmeshGen(string path) {
        SetupNavmeshDll(path);
    }
    public ERNavmeshGen() {
        string? path = Util.TryGetGameInstallLocation($"\\steamapps\\common\\Elden Ring\\Game\\EldenRing.exe");
        if (path == null) {
            throw new FileNotFoundException("Could not find EldenRing.exe. Please pass a path to the constructor.");
        }
        SetupNavmeshDll(path);
    }
    public bool SetupNavmeshDll(string path) {
        _erPath = path;
        //_navgen = Kernel32.LoadLibrary("DS3NavmeshGen.dll");
        // if (path.EndsWith(".exe")) {
        //     path = Path.GetDirectoryName(path) ?? throw new InvalidOperationException($"{nameof(path)} did not contain a parent directory. {path}");
        // }
        // if (!File.Exists($"{path}\\EldenRing.exe")) {
        //     throw new FileNotFoundException($"could not find {path}\\EldenRing.exe. Please provide a path to the \"Elden Ring\\Game\" folder.");
        // }
        return HavokNavmeshNative.SetGamePath(path);
    }
    public void BatchGenerateNavmesh(string folderPath, string compendiumPath) {
        HavokNavmeshNative.BatchGenerateNavMeshFromCollisionAPI(folderPath, compendiumPath);
    }
    public void GenerateNavmesh(string path, string compendiumPath) {
        HavokNavmeshNative.GenerateNavMeshFromCollisionAPI(path, compendiumPath);
    }
    public void Close() {
        HavokNavmeshNative.Close();
    }
    public void Dispose() {
        //Kernel32.FreeLibrary(_navgen);
    }
    
    
}
