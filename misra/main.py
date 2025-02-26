import argparse
import os
import shutil
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
    increment_config_path = os.path.join(install_path, "increment-config.yaml")
    full_config_path = os.path.join(install_path, "full-config.yaml")
    # 删除当前路径下的配置文件
    if os.path.exists("increment-config.yaml"):
        os.remove("increment-config.yaml")
    if os.path.exists("full-config.yaml"):
        os.remove("full-config.yaml")

    shutil.copy(increment_config_path, "increment-config.yaml")
    shutil.copy(full_config_path, "full-config.yaml")
    print("加载配置完成！")

def full_check():
    config = "./misra/full-check-config.yaml"
    # 判断文件是否存在
    if not os.path.exists(config):
        print("没有找到check-config.yaml ，请确认其是否存在。并在其相同的路径下执行检测命令")
        return
    task = make_task.FullTask(config)
    task.run_check()

def incre_check():
    config = "./misra/increment-check-config.yaml"
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