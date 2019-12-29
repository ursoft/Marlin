using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading;

namespace SyncOnboard
{
    class Program {
        static string SourcePath, DestinationPath, ComPort, OctoprintApiKey;
        const string Extension = "gcode";
        static void Main(string[] args) {
            // Create a new FileSystemWatcher and set its properties.
            using (var watcher = new FileSystemWatcher()) {
                SourcePath = args.Count() > 0 ? args[0] : @"c:\Users\Youra\Documents\Modeling\3д печать\Головоломки\_new\";
                watcher.Path = SourcePath;
                Console.WriteLine($"Watch source: {SourcePath}");
                DestinationPath = args.Count() > 1 ? args[1] : @"h:\";
                Console.WriteLine($"Watch destination: {DestinationPath}");
                ComPort = args.Count() > 2 ? args[2] : @"COM6";
                OctoprintApiKey = args.Count() > 3 ? args[3] : @"SecretApiKey";

                // Watch for changes in LastAccess and renaming of files.
                watcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.FileName;
                watcher.Filter = "*." + Extension;
                watcher.IncludeSubdirectories = true;

                watcher.Changed += OnChanged;
                watcher.Renamed += OnRenamed;

                // Begin watching.
                watcher.EnableRaisingEvents = true;

                Console.WriteLine("Enter 's' to manual sync.");
                while (true) {
                    try {
                        switch (Console.ReadLine()) {
                            case "s":
                                ManualSync(); break;
                        }
                    }
                    catch (Exception ex) {
                        ShowException(ex);
                    }
                }
            }
        }

        private static void ShowException(Exception ex) {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Exception: {ex}");
            Console.ResetColor();
        }
        private static bool IsDirectoryWritable(string dirPath) {
            try {
                using (FileStream fs = File.Create(
                    Path.Combine(
                        dirPath,
                        Path.GetRandomFileName()
                    ),
                    1,
                    FileOptions.DeleteOnClose)
                )
                { }
                return true;
            } catch {
                    return false;
            }
        }
        private static readonly HttpClient client = new HttpClient();
        private static void EnsureDestination() {
            while(true) {
                try { 
                    var dst = new DirectoryInfo(DestinationPath);
                    if (dst.Exists && IsDirectoryWritable(DestinationPath) && dst.GetFiles("firmware.cur").Length == 1)
                        return;
                    using (var key = Registry.CurrentUser.OpenSubKey("Network\\" + DestinationPath[0])) {
                        if (key != null) {
                            var server = key.GetValue("RemotePath").ToString().Replace(@"\\", "http://").Replace(@"\usb", "/api/printer/sd");
                            if (client.DefaultRequestHeaders.Count() == 0)
                                client.DefaultRequestHeaders.Add("X-Api-Key", OctoprintApiKey);
                            //Octoprint M22
                            Console.Write($"EnsureDestination: {server} M22...");
                            do {
                                var content = new StringContent("{\"command\":\"release\"}", Encoding.UTF8, "application/json");
                                var response = client.PostAsync(server, content);
                                if (response.Wait(5000)) {
                                    if (response.Result.StatusCode == HttpStatusCode.NoContent)
                                        break;
                                    else
                                        Console.Write(response.Result.StatusCode);
                                } else throw new Exception("Http timeout trying Octoprint M22");
                                Console.Write(".");
                            } while (true);
                            Console.WriteLine($"OK. Pause 5 sec...");
                            Thread.Sleep(5000);
                            continue;
                        }
                    }
                    // Local COM port M22
                    Console.Write($"EnsureDestination: {ComPort} M22");
                    using (var mySerialPort = new SerialPort(ComPort, 250000)) {
                        mySerialPort.Open();
                        mySerialPort.ReadTimeout = 1000;
                        byte[] data = Encoding.ASCII.GetBytes("M22\r\n");
                        var before = mySerialPort.ReadExisting();
                        mySerialPort.Write(data, 0, data.Length);
                        var after = mySerialPort.ReadExisting();
                        Console.WriteLine($".{before}.{after} OK. Pause 5 sec...");
                        Thread.Sleep(5000);
                    }
                } catch(Exception ex) { 
                    ShowException(ex);
                    Thread.Sleep(20000);
                }
            }
        }

        private static void ManualSync() {
            int equals = 0, synced = 0, newcopy = 0;
            var src = new DirectoryInfo(SourcePath);
            EnsureDestination();
            var dst = new DirectoryInfo(DestinationPath);
            foreach (var sd in src.GetDirectories()) {
                foreach(var sf in sd.GetFiles("*." + Extension)) {
                    var existing = dst.EnumerateFiles(sf.Name);
                    switch(existing.Count()) {
                        case 1: 
                            if (SyncTo(sf, existing.First())) 
                                synced++; 
                            else 
                                equals++; 
                            break;
                        case 0: 
                            CopyTo(sf, sf.Name); ++newcopy;  
                            break;
                        default: throw new Exception($"EnumerateFiles({sf.Name}).Count={existing.Count()}");
                    }
                }
            }
            Console.WriteLine($"ManualSync: equals={equals}, synced={synced}, new={newcopy}");
        }

        private static bool SyncTo(FileInfo sf, FileInfo df) {
            if (!df.Exists || Math.Abs((sf.LastWriteTimeUtc - df.LastWriteTimeUtc).TotalSeconds) > 2) {
                Console.Write($"Syncing {SimplifyPath(sf.FullName)} to {SimplifyPath(df.FullName)}...");
                SafeCopyTo(sf, df, true);
                Console.WriteLine("OK");
                return true;
            }
            else if (Math.Abs((sf.LastWriteTimeUtc - df.LastWriteTimeUtc).TotalSeconds) < 3) {
                Console.WriteLine($"{SimplifyPath(sf.FullName)} == {SimplifyPath(df.FullName)}");
                return false;
            } 
            throw new Exception($"SyncTo {SimplifyPath(sf.FullName)} to {SimplifyPath(df.FullName)} failed on {df.LastWriteTime} > {sf.LastWriteTime}");
        }

        public static bool IsFileReady(string filename) {
            // If the file can be opened for exclusive access it means that the file
            // is no longer locked by another process.
            try {
                using (FileStream inputStream = File.Open(filename, FileMode.Open, FileAccess.Read, FileShare.None))
                    return true;
            } catch (Exception) {
                return false;
            }
        }
        private static void SafeCopyTo(FileInfo sf, FileInfo df, bool overWrite)
        {
            while(!IsFileReady(sf.FullName)) {
                Thread.Sleep(250);
            }
            sf.CopyTo(df.FullName, overWrite);
        }

        private static void CopyTo(FileInfo sf, string exFilter) {
            string destFileName = Path.Combine(DestinationPath, exFilter);
            Console.Write($"Copying {SimplifyPath(sf.FullName)} to {SimplifyPath(destFileName)}...");
            SafeCopyTo(sf, new FileInfo(destFileName), true);
            Console.WriteLine("OK");
        }

        private static void OnChanged(object source, FileSystemEventArgs e) {
            var w = (FileSystemWatcher)source;
            try {
                w.EnableRaisingEvents = false;
                Console.WriteLine($"{DateTime.Now.ToLongTimeString()} OnChanged: {SimplifyPath(e.FullPath)} {e.ChangeType}");
                var src = new FileInfo(e.FullPath);
                switch (e.ChangeType) {
                    case WatcherChangeTypes.Changed:
                        EnsureDestination();
                        SyncTo(src, new FileInfo(Path.Combine(DestinationPath, src.Name)));
                        break;
                    default: throw new Exception($"OnChanged: unexpected {e}");
                }
            } catch(Exception ex) {
                ShowException(ex);
            } finally {
                w.EnableRaisingEvents = true;
            }
        }

        private static void OnRenamed(object source, RenamedEventArgs e) {
            Console.WriteLine($"{DateTime.Now.ToLongTimeString()} OnRenamed: {SimplifyPath(e.OldFullPath)} renamed to {SimplifyPath(e.FullPath)}");
            try
            {
                var src = new FileInfo(e.FullPath);
                EnsureDestination();
                var dest = new FileInfo(Path.Combine(DestinationPath, new FileInfo(e.OldFullPath).Name));
                if(dest.Exists) {
                    dest.Delete();
                    Console.WriteLine($"Deleted: {SimplifyPath(dest.FullName)}");
                }
                dest = new FileInfo(Path.Combine(DestinationPath, src.Name));
                SyncTo(src, dest);
            } catch (Exception ex) {
                ShowException(ex);
            }
        }

        private static string SimplifyPath(string p) {
            if(p.StartsWith(SourcePath, StringComparison.CurrentCultureIgnoreCase)) {
                return "[S]" + p.Substring(SourcePath.Length);
            }
            return p;
        }
    }
}
