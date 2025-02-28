import os
from pathlib import Path
import shutil
import sys
import argparse

from misra.tools import *


PYTHON_EXE = sys.executable

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
        # 判断 args.path 是一个合法的路径
        if not os.path.isdir(args.path):
            print(f"{args.path} 为非法路径.")
            exit(1)
        else:
            # 检查args.path 中是否有cppcheck.exe
            if not os.path.exists(os.path.join(args.path, "cppcheck.exe")):
                print(f"{args.path} 中没有cppcheck.exe, 请核对安装路径后重试")
                print(r"路径格式举例 --path=D:/cppcheck 或 --path=D:\cppcheck")
                return
            # 将args.path 转化为标准的路径格式
            args.path = os.path.abspath(args.path)
            print(f"安装路径为: {args.path}")
            cppcheck_dir = args.path
        # cppcheck_dir = cppcheck_dir.replace("\\", "/")
    else:
        print("Please specify the path to install cppcheck-misra")

    # 删除安装路径之前的misra文件夹
    if os.path.exists(os.path.join(cppcheck_dir,"misra")):
        shutil.rmtree(os.path.join(cppcheck_dir,"misra"))
    
    # 将安装包中的misra复制到安装路径
    shutil.copytree("misra", os.path.join(cppcheck_dir,"misra"))
    # Modify misra configuration 修改cfg文件中的占位符
    place_holder = "REPLACE_ME"
    replace_path = os.path.dirname(cppcheck_dir)
    
    
    # misra.json
    misra_json_in = Path("conf/misra.json.in")
    misra_json_out = Path(os.path.join(cppcheck_dir, "misra", "misra.json"))
    
    with misra_json_in.open('r') as f:
        content = f.read().replace(place_holder, replace_path)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
        content = content.replace("\\\\", "/")  # 将 \\ 替换为 /
        content = content.replace("\\", "/")    # 将 \ 替换为 /
        
    with misra_json_out.open('w') as f:
        f.write(content)
    # misra.sh / misra.bat
    misra_bat_in = Path("conf/misra.bat.in")
    misra_bat_out = Path(os.path.join(cppcheck_dir, "misra.bat"))
    with misra_bat_in.open('r') as f:
        content = f.read().replace(place_holder, replace_path)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
    with misra_bat_out.open('w') as f:
        f.write(content)
    
    # 复制 full_check.sh / full_check.bat 到cppcheck_dir/misra文件夹中
    full_check_in = Path("conf/full_check.bat.in")
    full_check_out = Path(os.path.join(cppcheck_dir, "misra", "full_check.bat"))
    print(full_check_out)
    shutil.copy(full_check_in, full_check_out)
    # 复制 incre_check.sh / incre_check.bat 到cppcheck_dir/misra文件夹中
    incre_check_in = Path("conf/incre_check.bat.in")
    incre_check_out = Path(os.path.join(cppcheck_dir, "misra", "incre_check.bat"))
    shutil.copy(incre_check_in, incre_check_out)

    

    print("## Cppcheck-Misra Install Successful!")

if __name__ == "__main__":
    main()


