

#  Setting up visual studio code 
```
https://code.visualstudio.com/download

Install extension C/C++ & Native debug (webfreak)
Also install the C/C++ extension from Microsoft 
```

Look att my examples in the directory .vscode

# Compiling
Visual studio code works fine for navigating both qemu and esp32 sources
Press ctrl-shift-P 
type tasks, select configure task runner, select other, This is for compiling qemu.

Newer versions of visual studio expects something like this

```
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "command": "make",
    "tasks": [
        {
            "type": "shell",
            "command": "./compile.sh",
            "taskName": "Makefile",
            // Make this the default build command.
            // Show the output window only if unrecognized errors occur.
            // No args
            "args": ["all"],
            // Use the standard less compilation problem matcher. ${workspaceRoot}
            "problemMatcher": {
                "owner": "cpp",
                "fileLocation": ["relative", "/"],
                "pattern": {
                    "regexp": "^(.*):(\\d+):(\\d+):\\s+(warning|error):\\s+(.*)$",
                    "file": 1,
                    "line": 2,
                    "column": 3,
                    "severity": 4,
                    "message": 5
                }
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "reveal": "always",
                "panel": "new"
            }

        }
    ]
}
```

# Navigating source when using CMake

.vscode/c_cpp_properties.json 
```

{
    "configurations": [
        {
            "name": "linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/sdkconfig",
                "${workspaceFolder}/build",
                "${env:IDF_PATH}/components",
                ""
            ],
           
            "compilerPath": "~/esp/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc",
            "cStandard": "c99",
            "cppStandard": "c++11",
            "intelliSenseMode": "clang-x64",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json"
        }
    ],
    "version": 4
}
```



#  Navigating sources  (Make version)
.vscode/c_cpp_properties.json 
```
{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "/usr/include",
                "/usr/local/include",
                "${workspaceRoot}"
            ],
            "defines": [],
            "intelliSenseMode": "clang-x64",
            "browse": {
                "path": [
                    "/usr/include",
                    "/usr/local/include",
                    "${workspaceRoot}"
                ],
                "limitSymbolsToIncludedHeaders": true,
                "databaseFilename": ""
            },
            "macFrameworkPath": [
                "/System/Library/Frameworks",
                "/Library/Frameworks"
            ]
        },
        {
            "name": "Linux",
            "includePath": [
                "~/esp/esp-idf/components",
                "~/esp/esp-idf/components/driver/include",
                "~/esp/esp-idf/components/newlib/include",
                "~/esp/esp-idf/components/freertos/include",
                "~/esp/esp-idf/components/esp32/include",
                "~/esp/esp-idf/components/log/include",
                "~/esp/esp-idf/components/nvs_flash/include",
                "~/esp/esp-idf/components/heap/include",
                "~/esp/esp-idf/components/tcpip_adapter/include",
                "~/esp/esp-idf/components/soc/include",
                "~/esp/esp-idf/components/lwip/include/lwip",
                "~/esp/esp-idf/components/lwip/include/lwip/port",
                "~/esp/esp-idf/components/soc/esp32/include",
                "${workspaceRoot}/build/include",
                "${workspaceRoot}/components/tft",
                "${workspaceRoot}/components/spiffs",
                "${workspaceRoot}/components/spidriver",
                "/usr/include/c++/7",
                "/usr/include/c++/7.2.0/tr1",
                "/usr/include/c++/7.2.0",
                "/usr/include/c++/7.2.0/x86_64-pc-linux-gnu/",                                    
                "/usr/include/x86_64-linux-gnu/c++/7",
                "/usr/include/c++/7/backward",
                "/usr/lib/gcc/x86_64-linux-gnu/7/include",
                "/usr/local/include",
                "/usr/lib/gcc/x86_64-linux-gnu/7/include-fixed",
                "/usr/include/x86_64-linux-gnu",
                "/usr/include",
		        "/usr/include/linux",
                "${workspaceRoot}"
            ],
            "defines": [],
            "intelliSenseMode": "clang-x64",
            "browse": {
                "path": [
                    "~/esp/esp-idf/components",
                    "~/esp/esp-idf/components/newlib/include/",
                    "~/esp/esp-idf/components/driver/include",
                    "~/esp/esp-idf/components/newlib/include",
                    "~/esp/esp-idf/components/freertos/include",
                    "~/esp/esp-idf/components/esp32/include",
                    "~/esp/esp-idf/components/log/include",
                    "~/esp/esp-idf/components/nvs_flash/include",
                    "~/esp/esp-idf/components/heap/include",
                    "~/esp/esp-idf/components/tcpip_adapter/include",
                    "~/esp/esp-idf/components/soc/include",
                    "~/esp/esp-idf/components/lwip/include/lwip",
                    "~/esp/esp-idf/components/lwip/include/lwip/port",
                    "~/esp/esp-idf/components/soc/esp32/include",
                    "${workspaceRoot}/build/include",    
                    "/usr/include/c++/7",
                    "/usr/include/c++/7.2.0/tr1",
                    "/usr/include/c++/7.2.0",
                    "/usr/include/c++/7.2.0/x86_64-pc-linux-gnu/",                    
                    "/usr/include/x86_64-linux-gnu/c++/7",
                    "/usr/include/c++/7/backward",
                    "/usr/lib/gcc/x86_64-linux-gnu/7/include",
                    "/usr/local/include",
                    "/usr/lib/gcc/x86_64-linux-gnu/7/include-fixed",
                    "/usr/include/x86_64-linux-gnu",
                    "/usr/include",
                    "${workspaceRoot}"
                ],
                "limitSymbolsToIncludedHeaders": true,
                "databaseFilename": ""
            }
        },
        {
            "name": "Win32",
            "includePath": [
                "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include",
                "${workspaceRoot}"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE"
            ],
            "intelliSenseMode": "msvc-x64",
            "browse": {
                "path": [
                    "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include/*",
                    "${workspaceRoot}"
                ],
                "limitSymbolsToIncludedHeaders": true,
                "databaseFilename": ""
            }
        }
    ],
    "version": 3
}
```

Now go to File->Preferences->Keyboard Shortcuts and add the following key binding for the build task:

```
// Place your key bindings in this file to overwrite the defaults
[
    { "key": "f8",          "command": "workbench.action.tasks.build" }
]
```

# More info

https://github.com/espressif/esp-idf/issues/303

# Debugging
Look at the .vscode/launch.json file.
```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug",
            "gdbpath": "~/esp/xtensa-esp32-elf/bin/xtensa-esp32-elf-gdb",
            "type": "gdb",
            "request": "attach",
            "executable" : "${workspaceRoot}/build/app-template.elf",
            "target": ":1234",
            "remote": true,
            "cwd": "${workspaceRoot}"
        }
    ]
}
```

When running the debugger, start qemu before launching the debugger then do
```
> interrupt
> b app_main
Then do reset in qemu
> c
```
A bit annoying but for hight level c functions it is nice to have a gdb gui.

[[https://raw.githubusercontent.com/Ebiroll/qemu_esp32/master/img/vscode.png]]

![debugger](img/vscode.png)



## Setting up platformio
Here is another ide that uses the atom editor, could be useful if it gets better.
[I would not currentrly recomed this, Setup Platform.io](./platformio.md)
http://platformio.org/platformio-ide

