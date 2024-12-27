# ExportFonts

ExportFonts is a utility for installing fonts on Windows systems, updating configuration files, and managing font installations with administrator privileges. It can also restart Rainmeter after updating font-related settings.

## Features

- Automatically installs `.ttf` and `.otf` font files from a specified directory.
- Updates a variables configuration file (`Installed_Fonts=1`) to reflect changes.
- Restarts Rainmeter to apply font changes seamlessly.
- Handles font installation with proper registry updates and broadcasts font changes to the system.

## Prerequisites

- Windows operating system.
- .NET Framework or .NET Core runtime compatible with the project.
- Administrator privileges to run the application.

## Usage

### Command Line Arguments

The program requires three arguments to run:

```plaintext
ExportFonts.exe <path-to-font-folder> <path-to-variables-file>
```

1. `<path-to-font-folder>`: The directory containing the font files to be installed.
2. `<path-to-variables-file>`: The path to the file where the `Installed_Fonts` variable is updated.

### Example

```bash
ExportFonts.exe "C:\Fonts" "C:\Rainmeter\variables.ini"
```

### Output

- Fonts from the specified folder are installed to the system.
- The variables file is updated with `Installed_Fonts=1`.
- Rainmeter is restarted to apply the font updates.

## How It Works

1. **Administrator Check**: Ensures the program is running with administrator privileges. If not, it relaunches itself with elevated privileges.
2. **Font Installation**: Copies fonts to the system's font directory and updates the Windows Registry.
3. **Configuration Update**: Modifies the provided variables file to indicate successful font installation.
4. **Rainmeter Restart**: Terminates and relaunches Rainmeter to reflect changes.

## Error Handling

- **Missing Administrator Privileges**: The program automatically relaunches with elevated privileges.
- **Invalid Paths**: Provides descriptive error messages if the specified paths are invalid or inaccessible.
- **Installation Failures**: Logs errors for fonts that fail to install.

## Known Issues

- The application assumes Rainmeter is installed and accessible via `Rainmeter.exe`. Ensure the executable is in your system's PATH or adjust the logic as needed.

## Contributing

Feel free to fork the repository and submit pull requests to enhance functionality or fix bugs.

## License

This project is licensed under the Apache License. See the [LICENSE](LICENSE) file for details.
