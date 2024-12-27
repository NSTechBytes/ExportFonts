using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Principal;
using Microsoft.Win32;

namespace ExportFonts
{
    class Program
    {
        [DllImport("gdi32.dll")]
        private static extern int AddFontResource(string lpszFilename);

        [DllImport("user32.dll")]
        private static extern bool SendMessage(int hWnd, int Msg, int wParam, int lParam);

        private const int HWND_BROADCAST = 0xFFFF;
        private const int WM_FONTCHANGE = 0x001D;

        static void Main(string[] args)
        {

            if (!IsAdministrator())
            {
                RelaunchAsAdministrator(args);
                return;
            }

            if (args.Length < 2)
            {
                Console.WriteLine("Usage: FontInstaller <path-to-font-folder> <path-to-restart-rainmeter-exe> <path-to-variables-file>");
                return;
            }

            string fontFolderPath = args[0];

            string variablesFilePath = args[1];

            if (!Directory.Exists(fontFolderPath))
            {
                Console.WriteLine($"Error: Directory '{fontFolderPath}' does not exist.");
                return;
            }


            if (!File.Exists(variablesFilePath))
            {
                Console.WriteLine($"Error: File '{variablesFilePath}' does not exist.");
                return;
            }

            try
            {
                Console.WriteLine("Installing fonts...");

                string[] fontFiles = Directory.GetFiles(fontFolderPath, "*.*", SearchOption.TopDirectoryOnly)
                                              .Where(file => file.EndsWith(".ttf", StringComparison.OrdinalIgnoreCase) ||
                                                             file.EndsWith(".otf", StringComparison.OrdinalIgnoreCase))
                                              .ToArray();

                foreach (var fontFile in fontFiles)
                {
                    try
                    {
                        InstallFont(fontFile);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Failed to install font '{Path.GetFileName(fontFile)}': {ex.Message}");
                    }
                }

                Console.WriteLine("All fonts processed successfully.");


                UpdateVariablesFile(variablesFilePath);

                RestartRainmeter();

                Console.WriteLine("Process completed successfully.");
            }
            catch (UnauthorizedAccessException)
            {
                Console.WriteLine("Error: Administrator privileges are required to install fonts.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
            }
        }


        private static bool IsAdministrator()
        {
            using (var identity = WindowsIdentity.GetCurrent())
            {
                var principal = new WindowsPrincipal(identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
        }
        private static void RelaunchAsAdministrator(string[] args)
        {
            var processInfo = new ProcessStartInfo
            {
                FileName = Process.GetCurrentProcess().MainModule.FileName,
                UseShellExecute = true,
                Verb = "runas",
                Arguments = string.Join(" ", args.Select(arg => $"\"{arg}\""))
            };

            try
            {
                Process.Start(processInfo);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Failed to relaunch as administrator: " + ex.Message);
            }
        }


        private static void InstallFont(string fontFilePath)
        {
            if (!File.Exists(fontFilePath))
                throw new FileNotFoundException("Font file not found", fontFilePath);

            int result = AddFontResource(fontFilePath);
            if (result == 0)
                throw new Exception("Failed to add font resource.");


            SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);


            string fontName = Path.GetFileNameWithoutExtension(fontFilePath);
            string windowsFontPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Fonts), Path.GetFileName(fontFilePath));

            if (!File.Exists(windowsFontPath))
                File.Copy(fontFilePath, windowsFontPath);

            using (var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts", writable: true))
            {
                if (key == null)
                    throw new Exception("Failed to access registry.");

                string fontRegistryName = Path.GetExtension(fontFilePath).ToLower() == ".otf" ? fontName + " (OpenType)" : fontName;
                key.SetValue(fontRegistryName, Path.GetFileName(fontFilePath));
            }

            Console.WriteLine($"Successfully installed font: {fontName}");
        }
        private static void UpdateVariablesFile(string variablesFilePath)
        {
            var lines = File.ReadAllLines(variablesFilePath).ToList();
            bool found = false;

            for (int i = 0; i < lines.Count; i++)
            {
                if (lines[i].StartsWith("Installed_Fonts="))
                {
                    lines[i] = "Installed_Fonts=1";
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                lines.Add("Installed_Fonts=1");
            }

            File.WriteAllLines(variablesFilePath, lines);
            Console.WriteLine("Updated variables file: Installed_Fonts=1");
        }

        private static void RestartRainmeter()
        {
            try
            {
                var rainmeterProcess = Process.GetProcessesByName("Rainmeter").FirstOrDefault();

                if (rainmeterProcess != null)
                {
                    Console.WriteLine("Closing Rainmeter...");
                    rainmeterProcess.Kill();
                    rainmeterProcess.WaitForExit();
                }

                Console.WriteLine("Restarting Rainmeter...");
                Process.Start("Rainmeter.exe");
               
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to restart Rainmeter: {ex.Message}");
            }
        }
    }
}
