import os
from pathlib import Path
import shutil
import sys
import argparse

from misra.tools import *


PYTHON_EXE = "python3"
def checkPythonVersion():
    global PYTHON_EXE
    resString = run_command(PYTHON_EXE + " --version" )
    if (resString != None):
        print(resString)
        return
    else:
        PYTHON_EXE = "python"
        resString = run_command(PYTHON_EXE + " --version" )
        if (resString != None):
            version = resString.strip()[1].split('.')[0]
            if version == '2':
                print("Need python version >= 3.x")
                exit(1)
            else:
                print(resString)
                return
        else:
            print("依赖命令python/python3")
            print("如果指定python路径, 请更改PYTHON_EXE")
            exit(1)

def ensure_directory_exists(directory_path):
    directory = Path(directory_path)
    if not directory.exists():
        directory.mkdir(parents=True, exist_ok=True)
def main():
    parser = argparse.ArgumentParser(description="Description:Install Cppcheck with Misra-C2012 support\n"
                                     "usage: python/python3 install_win.py [D:/]\n")
    parser.add_argument('--path', type=str, required=True, help='The [absolute path] where you want to install cppcheck-misra.')
    args = parser.parse_args()
    if os.name != 'nt':
        print("This script only works on Windows.")
        return
    cppcheck_dir = None
    if args.path:
        cppcheck_dir = args.path
        # cppcheck_dir = cppcheck_dir.replace("\\", "/")
    else:
        print("Please specify the path to install cppcheck-misra")
    
    if not os.path.exists(os.path.join(cppcheck_dir, "misra")):
        os.mkdir(os.path.join(cppcheck_dir, "misra"))
    # Modify misra configuration 修改cfg文件中的占位符
    place_holder = "REPLACE_ME"
    replace_path = os.path.dirname(cppcheck_dir)
    # misra.json

    misra_json_in = Path("conf/misra.json.in")
    misra_json_out = Path(os.path.join(cppcheck_dir, "misra", "misra.json"))
    
    with misra_json_in.open('r') as f:
        content = f.read().replace(place_holder, replace_path)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
    with misra_json_out.open('w') as f:
        f.write(content)


    # # pre-commit-config

    # pre_commit_config_in = Path("conf/.pre-commit-config.yaml.in")
    # pre_commit_config_out = Path(os.path.join("misra", ".pre-commit-config.yaml"))
    # with pre_commit_config_in.open('r') as f:
    #     content = f.read().replace(place_holder, replace_path)
    #     content = content.replace("PYTHON_EXE", PYTHON_EXE)
    # with pre_commit_config_out.open('w') as f:
    #     f.write(content)

    # misra.sh / misra.bat
    misra_bat_in = Path("conf/misra.bat.in")
    misra_bat_out = Path(os.path.join(cppcheck_dir, "misra.bat"))
    with misra_bat_in.open('r') as f:
        content = f.read().replace(place_holder, replace_path)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
    with misra_bat_out.open('w') as f:
        f.write(content)
    # Copy misra files to the appropriate location
    if os.path.exists(os.path.join(cppcheck_dir,"misra")):
        shutil.rmtree(os.path.join(cppcheck_dir,"misra"))
    shutil.copytree("misra", os.path.join(cppcheck_dir,"misra"))
    print("## modify misra config")


    print("## Cppcheck-Misra Install Successful!")

if __name__ == "__main__":
    checkPythonVersion()
    main()


