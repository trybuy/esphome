import json
import argparse
import glob
import os

parser = argparse.ArgumentParser(
                    prog='generate_cpp_properties',
                    description='Converting platformio idedata.json to vscode c_cpp_properties')

parser.add_argument('idedata_file', nargs="?", default="")
parser.add_argument('cpp_properties_file', nargs="?", default="")  
args = parser.parse_args()

if args.idedata_file == "":
    idedata_candidates = glob.glob(".esphome/idedata/*.json")
    if len(idedata_candidates) > 0:
        args.idedata_file = idedata_candidates[0]

if args.idedata_file == "" or not os.path.exists(args.idedata_file):
    print ("idedata json file not found!")
    exit(1)

if args.cpp_properties_file == "":
    args.cpp_properties_file = ".vscode/c_cpp_properties.json"

with open(args.idedata_file) as rf:
        file_contents = rf.read()

config = json.loads(file_contents)
name = config["env_name"]

print(f"Generating '{args.cpp_properties_file}' from '{args.idedata_file}', env: '{name}'")

cpp_properties = {
    "name": name,
    "defines": config["defines"], 
    "includePath": [item for row in config["includes"].values() for item in row],
    "compilerArgs": config["cxx_flags"],
    "compilerPath": config["cxx_path"]
}

with open(args.cpp_properties_file, "w") as outfile:
        outfile.write(json.dumps({"configurations": [cpp_properties], "version": 4}, indent=4))
