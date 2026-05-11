using Microsoft.Win32;

namespace ERNavmeshGenCS; 

public class Util {
            static (string, string)[] _pathValueTuple = new (string, string)[]
        {
            (@"HKEY_CURRENT_USER\SOFTWARE\Valve\Steam", "SteamPath"),
            (@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Valve\Steam", "InstallPath"),
            (@"HKEY_LOCAL_MACHINE\SOFTWARE\Valve\Steam", "InstallPath"),
            (@"HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Valve\Steam", "SteamPath"),
        };

        public static string? TryGetGameInstallLocation(string gamePath)
        {
            if (!gamePath.StartsWith("\\") && !gamePath.StartsWith("/")) {
                return null;
            }

            string? steamPath = GetSteamInstallPath();

            if (string.IsNullOrWhiteSpace(steamPath)) {
                return null;
            }

            string[] libraryFolders = File.ReadAllLines($@"{steamPath}/SteamApps/libraryfolders.vdf");
            char[] seperator = { '\t' };

            foreach (string line in libraryFolders)
            {
                if (!line.Contains("\"path\"")) {
                    continue;
                }

                string[] split = line.Split(seperator, StringSplitOptions.RemoveEmptyEntries);
                string libpath = split.First(x => x.ToLower().Contains("steam")).Replace("\"", "").Replace("\\\\", "\\");
                string libraryPath = libpath + gamePath;

                if (File.Exists(libraryPath)) {
                    return libraryPath.Replace("\\\\", "\\");
                }
            }

            return null;
        }

        public static string? GetSteamInstallPath()
        {
            string? installPath = null;

            foreach ((string path, string value) in _pathValueTuple)
            {
                string registryKey = path;
                installPath = Registry.GetValue(registryKey, value, null) as string;

                if (installPath != null) {
                    break;
                }
            }

            return installPath;
        }
}
