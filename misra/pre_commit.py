import os
import sys
import subprocess
import misra_filter
import yaml
from tools  import *

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
def get_git_root_dir(current_path):
    """
    current_path 为 执行git commit命令的路径
    返回项目的根目录，使用 命令git rev-parse --show-toplevel 获取
    """
    try:
        # 使用 subprocess.run 获取 git 根目录
        result = subprocess.run(
            ["git", "-C", current_path, "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
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



def get_changed_files(root_dir, extensions, against="HEAD"):
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
        result = subprocess.run(f"git -C {root_dir} {git_opt} rev-parse --verify {against}", shell=True, capture_output=True, text=True)
        if result.returncode != 0:
            # 仓库没有初始化
            # 4b825dc642cb6eb9a060e54bf8d69288fbee4904 是一个特殊的 Git 空树对象的 SHA-1 哈希值。
            against = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
    except Exception as e:
        print("Error:", e)
        against = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"

    diff_index_command = f"git -C {root_dir} {git_opt} diff-index --cached {against}"
    # print("diff_index_command:", diff_index_command)
    result = subprocess.run(diff_index_command, shell=True, capture_output=True, text=True)
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

def run_cppcheck(changed_files, root_dir):
    """
    运行cppcheck，并且过滤出misra的错误，并且生成html报告。
    :param changed_files: 需要检查的文件列表
    :param root_dir: 仓库根目录
    :return: 0表示成功，1表示有错误
    """
    out_folder = os.path.join(root_dir, "check_results")
    os.makedirs(out_folder, exist_ok=True)

    cppcheck_out_file = os.path.join(out_folder, "cppcheck.xml")
    misra_out_file = os.path.join(out_folder, "misra.xml")
    # 拼接命令
    if changed_files:
        # print("Checking files:", changed_files)
        misra_path = os.path.abspath(os.path.dirname(__file__))
        cppcheck_command = (
            f"cppcheck --enable=style --inline-suppr --inconclusive "
            f"--addon={misra_path}/misra.json "
            f"--suppressions-list={misra_path}/suppressions.txt "
            f"--error-exitcode=0 {' '.join(changed_files)} "
            f"--output-file={cppcheck_out_file} --xml "
            f"--quiet "
        )
        # print("run cppcheck command:", cppcheck_command)
        run_command(cppcheck_command)
        # 过滤cppcheck_out_file，只保留misra的错误
        misra_error_num = misra_filter.filter_main(cppcheck_out_file, misra_out_file)
        # misra_error_num > 0 表示有错误，生成html报告
        if misra_error_num > 0:
            if os.name == "posix":
                html_report_command = (
                    f"cppcheck-htmlreport --title=\"{os.path.basename(root_dir)}\" --file={misra_out_file} "
                    f"--report-dir={os.path.join(out_folder, 'html')} --source-encoding=iso8859-1"
                )
                run_command(html_report_command)
                log_warning("make html report")
            return 1
    return 0

def main():
    # root_dir = os.path.realpath(root_dir)
    # root_dir = get_git_root_dir(root_dir)
    # 当前路径
    current_path = os.getcwd()
    root_dir = get_git_root_dir(current_path)
    # 项目路径
    if not root_dir:
        return 1
    misra_path = os.path.dirname(__file__)
    #home_path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    # 获取支持的文件类型
    # print("homepath", home_path)
    file_types_file = os.path.join(misra_path,"config.yaml")
    file_type_list = get_yaml_list(file_path=file_types_file, key='file_types')
    # 获取检测文件
    changed_files = get_changed_files(root_dir, file_type_list)
    # 执行检测
    return run_cppcheck(changed_files, root_dir)

if __name__ == "__main__":
    sys.exit(main())