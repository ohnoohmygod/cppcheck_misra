import argparse
import os
import shutil
import stat
from tools import *
from uninstall import uninstall
import make_task
def find_file(parsent_path ,file_name):
    # 如果parsent_path及其子路径下有文件file 返回完整的路径
    # 如果没有找到，返回None
    for root, dirs, files in os.walk(parsent_path):
        if file_name in files:
            full_path = os.path.join(root, file_name)
            return full_path
    return None



def load_config():
    install_path = os.path.dirname(__file__)
    increment_config_path = os.path.join(install_path, "increment-check-config.yaml")
    full_config_path = os.path.join(install_path, "full-check-config.yaml")

    # 复制配置文件到当前路径
    if not os.path.exists("increment-check-config.yaml"):
        shutil.copy(increment_config_path, "increment-check-config.yaml")
    if not os.path.exists("full-check-config.yaml"):
        shutil.copy(full_config_path, "full-check-config.yaml")
    # 复制bat/sh文件到当前路径
    if os.name == "nt":
        full_check_bat = os.path.join(install_path, "full_check.bat")
        increment_check_bat = os.path.join(install_path, "incre_check.bat")
        # 当前文件夹下没有full_check.bat和incre_check.bat则复制到当前路径
        if not os.path.exists("full_check.bat"):
            shutil.copy(full_check_bat, "full_check.bat")
            os.chmod("full_check.bat", stat.S_IRWXU)
        if not os.path.exists("incre_check.bat"):
            shutil.copy(increment_check_bat, "incre_check.bat")
            os.chmod("incre_check.bat", stat.S_IRWXU)  
    elif os.name == "posix":
        full_check_sh = os.path.join(install_path, "full_check.sh")
        increment_check_sh = os.path.join(install_path, "incre_check.sh")
        # 当前文件夹下没有full_check.sh/incre_check.sh则复制到当前路径
        if not os.path.exists("full_check.sh"):
            shutil.copy(full_check_sh, "full_check.sh")
            os.chmod("full_check.sh", stat.S_IRWXU)
        if not os.path.exists("incre_check.sh"):
            shutil.copy(increment_check_sh, "incre_check.sh")
            os.chmod("incre_check.sh", stat.S_IRWXU)  
    else:
        print("不支持的操作系统")
        return

    print("加载配置完成！")

def full_check():
    config = "./full-check-config.yaml"
    # 判断文件是否存在
    if not os.path.exists(config):
        print("没有找到check-config.yaml ，请确认其是否存在。并在其相同的路径下执行检测命令")
        return
    task = make_task.FullTask(config)
    task.run_check()

def incre_check():
    config = "./increment-check-config.yaml"
    # 判断文件是否存在
    if not os.path.exists(config):
        print("没有找到check-config.yaml ，请确认其是否存在。并在其相同的路径下执行检测命令")
        return
    task = make_task.IncrementalTask(config)
    task.run_check()

def main():
    parser = argparse.ArgumentParser(description="Dscription: Cppcheck with Misra-C2012 support. A tool to check C/C++ code for MISRA violations.")
    group = parser.add_mutually_exclusive_group()
    # group.add_argument('--help', type=str, required=False,  help='Default: $HOME/cppcheck.  The [absolute path] where you want to install cppcheck-misra.')
    group.add_argument('--load_config', action='store_true', required=False, help='从安装路径加载配置文件到当前路径.')
    group.add_argument('--full', action='store_true', required=False, help='根据当前路径下的full-config.yaml执行全量检测.')
    group.add_argument('--incre', action='store_true', required=False, help='根据路径下的increment-config.yaml执行增量检测.')
    group.add_argument('--uninstall', action='store_true', required=False, help='卸载工具')
    args = parser.parse_args()
    #mode = None
    #parameters = None
    
    if args.load_config:
        load_config()
    elif args.uninstall:
        uninstall()
    elif args.full:
        full_check()
    elif args.incre:
        incre_check()
    else:
        print('Please use parameter "-h" to see help message!')
        return

if __name__ == '__main__':
    main()