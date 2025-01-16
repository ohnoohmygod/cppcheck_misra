import os
import shutil
import sys
import subprocess
import misra_filter
import yaml
from tools  import *



def run_cppcheck(changed_files, root_dir):
    """
    运行cppcheck，并且过滤出misra的错误，并且生成html报告。
    :param changed_files: 需要检查的文件列表
    :param root_dir: 仓库根目录
    :return: 0表示成功，1表示有错误
    """
    out_folder = os.path.join(root_dir, "check_results")
    # 删除out_folder
    if os.path.exists(out_folder):
        shutil.rmtree(out_folder)
        
    os.makedirs(out_folder, exist_ok=True)
    # 创建标记文件，辅助commit-msg脚本判断经过是否cppcheck
    cppcheck_flag = os.path.join(out_folder, "flag.txt")
    with open(cppcheck_flag, "w") as f:
        f.write("")
    cppcheck_out_file = os.path.join(out_folder, "cppcheck.xml")
    misra_out_file = os.path.join(out_folder, "misra.xml")

    # 拼接命令
    if changed_files:
        # print("Checking files:", changed_files)
        misra_path = os.path.abspath(os.path.dirname(__file__))
        cppcheck_command = (
            f"cppcheck "
            f"--enable=all "
            f"--inline-suppr "
            f"--inconclusive "
            f"--addon={misra_path}/misra.json "
            f"--suppressions-list={misra_path}/suppressions.txt "
            f"--error-exitcode=0 {' '.join(changed_files)} "
            f"--output-file={cppcheck_out_file} "
            f"--xml "
        )

        cppcheck_report_command = (
            f"cppcheck-htmlreport --title=\"{os.path.basename(root_dir)}\" --file={cppcheck_out_file} "
            f"--report-dir={os.path.join(out_folder, 'html_cppcheck')} --source-encoding=iso8859-1"
        )
        misra_report_command = (
            f"cppcheck-htmlreport --title=\"{os.path.basename(root_dir)}\" --file={misra_out_file} "
            f"--report-dir={os.path.join(out_folder, 'html_misra')} --source-encoding=iso8859-1"
        )
        # print("run cppcheck command:", cppcheck_command)
        print(cppcheck_command)
        process = subprocess.Popen(cppcheck_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        while True:
            output = process.stdout.readline()
            if output == b'' and process.poll() is not None:
                break
            if output:
                print(output.decode().strip())
        misra_error_num = misra_filter.filter_main(cppcheck_out_file, misra_out_file, hasChangedLines=True)
        if os.name == 'posix':
            run_command(cppcheck_report_command)
            run_command(misra_report_command)
        # 过滤cppcheck_out_file，只保留misra的错误
        log_warning("make html report")
        
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