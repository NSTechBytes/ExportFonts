#include <iostream>
#include <windows.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <filesystem>
#include <fstream>
#include <shlobj.h>  // Required for SHGetKnownFolderPath and FOLDERID_Fonts
#include <knownfolders.h>  // Defines FOLDERID_Fonts


// Link required libraries
#pragma comment(lib, "Shlwapi.lib")




bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin;
}

void RelaunchAsAdmin(int argc, wchar_t* argv[]) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    std::wstring params;
    for (int i = 1; i < argc; ++i) {
        params += L"\"";
        params += argv[i];
        params += L"\" ";
    }

    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.lpParameters = params.c_str();
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteEx(&sei)) {
        std::wcerr << L"Failed to elevate privileges. Error: " << GetLastError() << L"\n";
        exit(1);
    }

    exit(0);
}

void AddFontToRegistry(const std::wstring& fontName, const std::wstring& fontPath) {
    HKEY hKey;
    const wchar_t* regPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegSetValueEx(hKey, fontName.c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(fontPath.c_str()),
            static_cast<DWORD>((fontPath.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS) {
            std::wcout << L"Successfully added " << fontName << L" to registry.\n";
        }
        else {
            std::wcerr << L"Failed to add " << fontName << L" to registry.\n";
        }
        RegCloseKey(hKey);
    }
    else {
        std::wcerr << L"Failed to open registry key.\n";
    }
}

std::wstring GetWindowsFontsFolder() {
    PWSTR fontsFolderPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &fontsFolderPath) == S_OK) {
        std::wstring result(fontsFolderPath);
        CoTaskMemFree(fontsFolderPath);
        return result;
    }
    else {
        throw std::runtime_error("Failed to get Windows Fonts folder path.");
    }
}

void InstallFontsFromFolder(const std::wstring& folderPath) {
    try {
        std::wstring fontsFolder = GetWindowsFontsFolder();

        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                std::wstring fontFile = entry.path().wstring();

                // Extract the font name (file name without extension)
                std::wstring fontName = entry.path().filename().wstring();
                fontName = fontName.substr(0, fontName.find_last_of(L'.'));

                // Copy font file to the Windows Fonts directory
                std::wstring destinationPath = fontsFolder + L"\\" + entry.path().filename().wstring();
                if (CopyFile(fontFile.c_str(), destinationPath.c_str(), FALSE)) {
                    std::wcout << L"Copied " << fontFile << L" to " << destinationPath << L".\n";

                    // Add the font to the registry
                    AddFontToRegistry(fontName, entry.path().filename().wstring());
                }
                else {
                    std::wcerr << L"Failed to copy " << fontFile << L" to Fonts folder.\n";
                }
            }
        }

        // Notify the system to update the fonts
        SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
        std::wcout << L"Font installation completed.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

bool KillProcessByName(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (processName == pe32.szExeFile) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    CloseHandle(hSnapshot);
                    return true;
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return false;
}

void RestartRainmeter() {
    try {
        const std::wstring rainmeterProcessName = L"Rainmeter.exe";

        // Kill Rainmeter process
        std::wcout << L"Checking for Rainmeter process...\n";
        if (KillProcessByName(rainmeterProcessName)) {
            std::wcout << L"Rainmeter process terminated successfully.\n";
        }
        else {
            std::wcout << L"Rainmeter process not found or unable to terminate.\n";
        }

        // Restart Rainmeter
        std::wcout << L"Restarting Rainmeter...\n";
        if (ShellExecute(NULL, L"open", rainmeterProcessName.c_str(), NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE)32) {
            std::wcerr << L"Failed to restart Rainmeter.\n";
        }
        else {
            std::wcout << L"Rainmeter restarted successfully.\n";
        }
    }
    catch (...) {
        std::wcerr << L"An error occurred while restarting Rainmeter.\n";
    }
}

void WriteVariablesFile(const std::wstring& filePath) {
    try {
        std::wofstream outFile(filePath);
        if (outFile.is_open()) {
            outFile << L"[Variables]\n";
            outFile << L"Installed_Fonts=1\n";
            outFile.close();
            std::wcout << L"File written successfully: " << filePath << L"\n";
        }
        else {
            std::wcerr << L"Failed to write to file: " << filePath << L"\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing to file: " << e.what() << "\n";
    }
}

int wmain(int argc, wchar_t* argv[]) {
    if (!IsRunAsAdmin()) {
        std::wcerr << L"Program is not running as administrator. Relaunching with elevated privileges...\n";
        RelaunchAsAdmin(argc, argv);
    }

    if (argc != 3) {
        std::wcerr << L"Usage: FontInstaller.exe <folder-path> <output-file-path>\n";
        return 1;
    }

    std::wstring folderPath = argv[1];
    std::wstring outputFilePath = argv[2];

    if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath)) {
        InstallFontsFromFolder(folderPath);
        WriteVariablesFile(outputFilePath);
        RestartRainmeter();
    }
    else {
        std::wcerr << L"Invalid folder path: " << folderPath << L"\n";
        return 1;
    }

    return 0;
}
