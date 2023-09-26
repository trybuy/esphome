import json
import os
import sys

#
# esphome idedata .\multimeter121gw\multimeter121gw.yml | python .\generate_cpp_properties.py
#

if sys.stdin.isatty():
    print("Usage:")
    print("     esphome idedata <config.yml> | python .\generate_cpp_properties.py")
    exit(1)

cpp_properties_file = ".vscode/c_cpp_properties.json"

config = json.loads(sys.stdin.read())
print(config)
name = config["env_name"]

print(f"Generating '{cpp_properties_file}', env: '{name}'")

cpp_properties = {
    "name": name,
    "defines": config["defines"], 
    "includePath": [item for row in config["includes"].values() for item in row],
    "compilerArgs": config["cxx_flags"],
    "compilerPath": config["cxx_path"]
}

if os.path.exists(cpp_properties_file):
    with open(cpp_properties_file, "r") as cpp_file:
        target_config = json.loads(cpp_file.read())
        index = next((i for i, config in enumerate(target_config["configurations"]) if config["name"] == name), -1)
        if index != -1:
             target_config["configurations"][index] = cpp_properties
        else:
             target_config["configurations"].append(cpp_properties)
else:
    target_config = {"configurations": [cpp_properties], "version": 4}

with open(cpp_properties_file, "w") as outfile:
        outfile.write(json.dumps(target_config, indent=4))
