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

CPP_PROPERTIES_FILE = ".vscode/c_cpp_properties.json"

config = json.loads(sys.stdin.read())
print(config)
name = config["env_name"]

print(f"Generating '{CPP_PROPERTIES_FILE}', env: '{name}'")

cpp_properties = {
    "name": name,
    "defines": config["defines"], 
    "includePath": [item for row in config["includes"].values() for item in row],
    "compilerArgs": config["cxx_flags"],
    "compilerPath": config["cxx_path"]
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
