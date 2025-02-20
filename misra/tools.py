import os
import subprocess
from termcolor import colored
import yaml
import xml.etree.ElementTree as ET

def log_success(message):
    print(colored(message, 'green'))

def log_error(message):
    print(colored(message, 'red'))

def log_warning(message):
    print(colored(message, 'yellow'))

def run_command(command):
    """
    @description  : 运行命令, 并返回输出。运行报错则打印异常。
    @param ------ command: 命令
    @Return ------- 命令输出
    """
    try:
        result = subprocess.run(command, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        log_error(e)
        return None
     
def load_config(config_file = "config.yaml"):
    """
    加载配置文件，返回一个字典
    """
    with open(config_file, 'r') as f:
        config = yaml.safe_load(f)
    return config

def get_git_root_dir(current_path):
    """
    @description: 输出当前执行路径的git根目录.
    @param: current_path 为执行git commit命令的路径.
    @Return: 返回的项目git根目录
    """
    try:
        # 使用 命令git rev-parse --show-toplevel 获取
        result = subprocess.run(
            ["git", "-C", current_path, "rev-parse", "--show-toplevel"],
            capture_output=True,
            universal_newlines=True,
            check=True
        )
        root_dir = result.stdout.strip()
        return root_dir
    except subprocess.CalledProcessError:
        print("\033[31m Not a Git repository.\033[0m")
        return None
    except Exception as e:
        print("Error:", e)
        return None
    
def get_changed_files(root_dir, extensions, against="HEAD^"):
    """
    Usage:
    获取当前git仓库下，所有修改的、指定类型的文件。
    Eg: get_changed_files(".", [".c", ".cpp"])
        获取当前仓库下，所有修改的，后缀为 .c 或 .cpp 的文件。
    :param root_dir: 仓库根目录
    :param extensions: 需要过滤的文件扩展名列表，例如 ['.c', '.cpp']
    :param against: 对比的记录，默认为 'HEAD'
    :return: 修改的文件列表
    """
    git_opt = f"--git-dir={os.path.join(root_dir, '.git')}"

    try:
        result = subprocess.run(f"git -C {root_dir} {git_opt} rev-parse --verify {against}", shell=True, capture_output=True, universal_newlines=True)
        if result.returncode != 0:
            # 仓库没有初始化
            # 4b825dc642cb6eb9a060e54bf8d69288fbee4904 是一个特殊的 Git 空树对象的 SHA-1 哈希值。
            against = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
    except Exception as e:
        print("Error:", e)
        against = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"

    diff_index_command = f"git -C {root_dir} {git_opt} diff-index --cached {against}"
    # print("diff_index_command:", diff_index_command)
    result = subprocess.run(diff_index_command, shell=True, capture_output=True, universal_newlines=True)
    output = result.stdout.strip()
    # print("changed files:", output)
    # 解析输出结果并筛选符合扩展名的文件
    changed_files = []
    for line in output.split('\n'):
        # 命令结果的一行的内容分割
        parts = line.split()
        if len(parts) >= 2:
            # status 有 A 或 M，A 表示新增，M 表示修改
            status, file_path = parts[-2], parts[-1]
            if status not in ['A', 'M']:
                continue
            # 文件后缀需要在 extensions 中
            if any(file_path.endswith(ext) for ext in extensions):
                full_path = os.path.realpath(os.path.join(root_dir, file_path))
                changed_files.append(full_path)

    return changed_files

def read_yaml_file(file_path):
    """
    读取 YAML 文件并返回其内容。
    
    :param file_path: YAML 文件的路径
    :return: YAML 文件的内容（字典形式）
    """
    with open(file_path, 'r', encoding='utf-8') as file:
        data = yaml.safe_load(file)
    return data

def get_yaml_list(file_path, key):
    """
    从 YAML 文件中获取指定的列表。
    
    :param file_path: YAML 文件的路径
    :param key: 要获取的键（可以是嵌套的键，用点号分隔）
    :return: 列表值
    """
    data = read_yaml_file(file_path)
    keys = key.split('.')
    value = data
    for k in keys:
        if k in value:
            value = value[k]
        else:
            return None
    return value

    