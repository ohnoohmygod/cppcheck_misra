import argparse
import os
import shutil
import time
from misra_filter import filter_main
from pre_commit import read_yaml_file
import yaml
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
def file_check(file_list, output_path):
    """
    Description: 
        Check the git project,If you what check your project that not a git project,you can use git init command to create a git repository.
    Return:
        None
    """
   
   # Get misra dir: install_path/cppcheck/misra
    misra_path = os.path.abspath(os.path.dirname(__file__))

    # TODO 
    # 未来也许需要支持多种错误类型，不过现在好像没有这个需求

    # 错误类型
    error_level = ["warning", "style", "performance", "information"]
    error_list = ""
    for i, error in enumerate(error_level):
        if i == len(error_level) - 1:
            error_list += error
        else:
            error_list += error + ","
    print(error_list)
    # Check the file list
    if os.path.exists(output_path):
        shutil.rmtree(output_path)
    os.mkdir(output_path)
    cppcheck_result_path = os.path.join(output_path, "cppcheck.xml")
    misra_result_path = os.path.join(output_path, "misra.xml")
    cppcheck_command = (
        f"cppcheck "
        f"{file_list} "
        f"--enable={error_list} "
        f"--inline-suppr "
        f"--inconclusive "
        f"--addon={misra_path}/misra.json "
        f"--suppressions-list={misra_path}/suppressions.txt "
        f"--error-exitcode=0 "
        f"--output-file={cppcheck_result_path} "
        f"--xml "
    )
    print(cppcheck_command)
    process = subprocess.Popen(cppcheck_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    while True:
        output = process.stdout.readline()
        if output == b'' and process.poll() is not None:
            break
        if output:
            print(output.decode().strip())
            
    if not os.path.exists(cppcheck_result_path):
        log_error("cppcheck check failed.")
        return
    
        
    cppcheck_report_command = (
        f"cppcheck-htmlreport "
        f"--title=file_check " #the title of report,it's not important
        f"--file={cppcheck_result_path} "
        f"--report-dir={os.path.join(output_path, 'cppcheck_html')} "
        f"--source-encoding=iso8859-1 "
    )
    misra_report_command = (
        f"cppcheck-htmlreport "
        f"--title=file_check " #the title of report,it's not important
        f"--file={misra_result_path} "
        f"--report-dir={os.path.join(output_path, 'misra_html')} "
        f"--source-encoding=iso8859-1 "
    )
    filter_main(cppcheck_result_path, misra_result_path)
    if os.name == 'posix':
        run_command(cppcheck_report_command)
        run_command(misra_report_command)

    return 

def module_check(module_path, output_path):
    """
    Description: 
        Check a project or module or a directory, If you what check your project that not a git project,you can use git init command to create a git repository.
    Return:
        None
    """
    misra_dir = os.path.abspath(os.path.dirname(__file__))
    if os.path.exists(output_path):
        shutil.rmtree(output_path)
    os.mkdir(output_path)
    cppcheck_result_path = os.path.join(output_path, "cppcheck.xml")
    include_path = None
    if os.path.exists(os.path.join(module_path, "include")):
        include_path = os.path.join(module_path, "include")
    elif os.path.exists(os.path.join(module_path, "inc")):
        include_path = os.path.join(module_path, "inc")
    else:
        include_path = None
    
    cppcheck_command = (
        f"cppcheck "
        f"{module_path} "
        f"--enable=all "
        f"--inline-suppr "
        f"--inconclusive "
        f"--addon={misra_dir}/misra.json "
        f"--suppressions-list={misra_dir}/suppressions.txt "
        f"--error-exitcode=0 "
        f"--output-file={cppcheck_result_path} "
        f"--xml "
    )
    if include_path:
        cppcheck_command += f"-I{include_path} "
    print(cppcheck_command)
    process = subprocess.Popen(cppcheck_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    while True:
        output = process.stdout.readline()
        if output == b'' and process.poll() is not None:
            break
        if output:
            print(output.decode().strip())
    if not os.path.exists(cppcheck_result_path):
        log_error("cppcheck check failed.")
        return
    misra_result_path = os.path.join(output_path, "misra.xml")
    filter_main(cppcheck_result_path, misra_result_path)
    cppcheck_report_command = (
        f"cppcheck-htmlreport "
        f"--title=\"{os.path.basename(module_path)}\""
        f" --file={cppcheck_result_path} "
        f"--report-dir={os.path.join(output_path, 'cppcheck_html')} "
        f"--source-encoding=iso8859-1"
    )
    misra_report_command = (
        f"cppcheck-htmlreport "
        f"--title=\"{os.path.basename(module_path)}\""
        f" --file={misra_result_path} "
        f"--report-dir={os.path.join(output_path, 'misra_html')} "
        f"--source-encoding=iso8859-1"
    )
    if os.name == 'posix':
        log_warning(f"making html report ...")
        run_command(cppcheck_report_command)
        run_command(misra_report_command)

    return
def project_check(project_path, output_path):
    """
    Description: 
        Check the project.
    Return:
        None
    """
    print("进行项目级别检查，需要执行很久...")
    misra_dir = os.path.abspath(os.path.dirname(__file__))
    if os.path.exists(output_path):
        shutil.rmtree(output_path)
    os.mkdir(output_path)
    cppcheck_result_path = os.path.join(output_path, "cppcheck.xml")
    
    cppcheck_command = (
        f"cppcheck "
        f"--enable=warning,style "
        f"--inline-suppr "
        f"--inconclusive "
        f"--addon={misra_dir}/misra.json "
        f"--suppressions-list={misra_dir}/suppressions.txt "
        f"--error-exitcode=0 "
        f"--output-file={cppcheck_result_path} "
        f"--xml "
    )
    compile_commands_path = find_file(project_path, "compile_commands.json")
    if compile_commands_path:
        cppcheck_command += f"--project={compile_commands_path} "
    else:
        cppcheck_command += f"--project={project_path} "
        print("项目中未找到compile_commands.json文件")
        
    
    process = subprocess.Popen(cppcheck_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
    while True:
        output = process.stdout.readline()
        if output == b'' and process.poll() is not None:
            break
        if output:
            print(output.decode().strip())
    
    if not os.path.exists(cppcheck_result_path):
        log_error("cppcheck check failed.")
        return
    misra_result_path = os.path.join(output_path, "misra.xml")
    filter_main(cppcheck_result_path, misra_result_path)
    cppcheck_report_command = (
        f"cppcheck-htmlreport "
        f"--title=\"{os.path.basename(project_path)}\""
        f" --file={cppcheck_result_path} "
        f"--report-dir={os.path.join(output_path, 'cppcheck_html')} "
        f"--source-encoding=iso8859-1"
    )
    misra_report_command = (
        f"cppcheck-htmlreport "
        f"--title=\"{os.path.basename(project_path)}\""
        f" --file={misra_result_path} "
        f"--report-dir={os.path.join(output_path, 'misra_html')} "
        f"--source-encoding=iso8859-1"
    )
    if os.name == 'posix':
        log_warning(f"making html report ...")
        run_command(cppcheck_report_command)
        run_command(misra_report_command)

    return
def install_hook():
    install_path = os.path.dirname(__file__)
    template_path = os.path.join(install_path, ".pre-commit-config.yaml")
    shutil.copy(template_path, ".pre-commit-config.yaml")
    uninstall_hook()
    run_command("pre-commit install")
    log_success("install pre-commit hook done.")

def uninstall_hook():
    run_command("pre-commit uninstall")

def check():
    config = "./misra/check-config.yaml"
    # 判断文件是否存在
    if not os.path.exists(config):
        log_error("没有找到check-config.yaml ，请确认其是否存在。并在其相同的路径下执行检测命令")
        return
    task = make_task.Task(config)
    task.run_check()
def main():
    parser = argparse.ArgumentParser(description="Dscription: Cppcheck with Misra-C2012 support. A tool to check C/C++ code for MISRA violations.")
    group = parser.add_mutually_exclusive_group()
    # group.add_argument('--help', type=str, required=False,  help='Default: $HOME/cppcheck.  The [absolute path] where you want to install cppcheck-misra.')
    group.add_argument('--file', type=str, required=False, help='The file path list you what to check.File path splited by space.')
    group.add_argument('--module', type=str, required=False, help='The module path  you what to check.')
    group.add_argument('--project', type=str, required=False, help='The list of file path you what to check.')
    group.add_argument('--install_hook', action='store_true', required=False, help='Install pre-commit hook.')
    group.add_argument('--uninstall_hook', action='store_true', required=False, help='Remove pre-commit hook.')
    group.add_argument('--uninstall_misra', action='store_true', required=False, help='Uninstall this tool. Include: cppcheck, script.')
    group.add_argument('--check', action='store_true', required=False, help='在当前文件加载check-config.yaml检查')
    args = parser.parse_args()
    #mode = None
    #parameters = None
    
    if args.file:
        #mode = 'file'
        file_check(args.file, "./check_results")
    elif args.module:
        #mode = 'module'
        module_check(args.module, "./check_results")
    elif args.project:
        #mode = 'project'
        project_check(args.project, "./check_results")
    elif args.install_hook:
        install_hook()
    elif args.uninstall_hook:
        uninstall_hook()
    elif args.uninstall_misra:
        uninstall()
    elif args.uninstall_hook:
        uninstall_hook()
    elif args.uninstall_misra:
        uninstall()
    elif args.check:
        check()
    else:
        print('Please use parameter "-h" to see help message!')
        return

if __name__ == '__main__':
    main()