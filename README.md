![Build Status](https://github.com/CVNLD/CFGComp/workflows/MSBuild/badge.svg)
## CFGComp

![Alt text](https://i.imgur.com/fpIvISS.png)

## Features

- Read PCIe configuration space from physical devices
- Parse COE files containing expected configuration data
- Compare actual device configuration with expected configuration
- Colorized output highlighting matches and mismatches
- Detailed debugging information

## Prerequisites

- Windows operating system
- Administrator privileges (required for accessing PCIe configuration space)
- Visual Studio 2019 or later (for building the project)

## Building the Project

1. Clone this repository:
   ```
   git clone https://github.com/yourusername/CFGComp.git
   ```
2. Open the solution file `CFGComp.sln` in Visual Studio.
3. Build the solution (F7 or Build > Build Solution).

## Usage

Run the program from the command line with administrator privileges:

```
CFGComp.exe <coe_file> <bus:device:function> [--debug]
```

- `<coe_file>`: Path to the COE file containing the expected configuration.
- `<bus:device:function>`: PCIe address of the device to check, in the format `bus:device:function` (hexadecimal).
- `--debug`: (Optional) Enable detailed debug output.

Example:
```
CFGComp.exe input.coe 01:00:0 --debug
```

## Output

The program will display:
- Device information (Vendor ID and Device ID)
- A comparison of the actual configuration space with the expected configuration from the COE file
- Matching bytes in green, mismatching bytes in red
- Total number of mismatches, if any

## Troubleshooting

If you encounter issues:
1. Ensure you're running the program with administrator privileges.
2. Verify that the COE file path and PCIe address are correct.
3. Check for any error messages in the output.
4. Run with the `--debug` flag for more detailed information.

## Contributing

Contributions to CFGComp are welcome! Please feel free to submit pull requests, create issues or spread the word.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Thanks to all contributors and users of CFGComp.
- Special thanks to the open-source community for their invaluable tools and libraries.
