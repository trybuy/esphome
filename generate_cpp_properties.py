"""Generating cpp_properties configuration based on idedata from platformio
    Usage:
        esphome idedata <config.yml> | python .\\generate_cpp_properties.py
"""

import json
import os
import sys

if sys.stdin.isatty():
    print("Usage:")
    print("     esphome idedata <config.yml> | python .\\generate_cpp_properties.py")
    sys.exit(1)

# presuming that script is running from workspace folder
CPP_PROPERTIES_FILE = ".vscode/c_cpp_properties.json"
workspace_folder_rel = (os.getcwd(), "${workspaceFolder}")
platformio_folder_env = os.getenv('PLATFORMIO_CORE_DIR')
if platformio_folder_env:
    platformio_folder_rel = (platformio_folder_env, "${env:PLATFORMIO_CORE_DIR}")
else:
    platformio_folder_rel = os.path.expanduser('~'), "${env:HOME}"

def make_path_relative(path):
    for r in [workspace_folder_rel, platformio_folder_rel]:
        path = path.replace(r[0], r[1])
        if os.path.sep == '\\':
            path = path.replace(r[0].replace('\\', '/'), r[1])
    return path

config = json.loads(sys.stdin.read())
name = config["env_name"]
includes = set([item for row in config["includes"].values() for item in row])
existing_includes = [item for item in includes if os.path.exists(item)]
relative_includes = [make_path_relative(item) for item in existing_includes]

print(f"Generating '{CPP_PROPERTIES_FILE}', env: '{name}'")

cpp_properties = {
    "name": name,
    "defines": config["defines"], 
    "includePath": sorted(relative_includes),
    "compilerArgs": [make_path_relative(c) for c in config["cxx_flags"]],
    "compilerPath": make_path_relative(config["cxx_path"])
}

if os.path.exists(CPP_PROPERTIES_FILE):
    with open(CPP_PROPERTIES_FILE, "r", encoding="utf8") as cpp_file:
        target_config = json.loads(cpp_file.read())
        configurations = target_config["configurations"]
        index = next((i for i, c in enumerate(configurations) if c["name"] == name), -1)
        if index != -1:
            configurations[index] = cpp_properties
        else:
            configurations.append(cpp_properties)
else:
    target_config = {"configurations": [cpp_properties], "version": 4}

with open(CPP_PROPERTIES_FILE, "w", encoding="utf8") as outfile:
    outfile.write(json.dumps(target_config, indent=4))
