using System.Runtime.InteropServices;

namespace ERNavmeshGenCS; 

public static class Kernel32 {
    [DllImport("kernel32", SetLastError = true)]
    static extern IntPtr LoadLibraryW([MarshalAs(UnmanagedType.LPWStr)]string lpFileName);
        
    [DllImport("kernel32.dll")]
    public static extern uint GetLastError();
        
    [DllImport("kernel32.dll", SetLastError=true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool FreeLibrary(IntPtr hModule);

    public static IntPtr LoadLibrary(string path) {
        IntPtr handle = LoadLibraryW(path);
        if (handle != IntPtr.Zero) {
            return handle;
        }
        uint error = GetLastError();
        throw new DllNotFoundException($"{Path.GetFileName(path)} not found at path {path}\n" +
            $"Last Error = {error}");
    }
}
