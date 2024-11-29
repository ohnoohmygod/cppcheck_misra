import argparse
import os
from abc import ABC, abstractmethod
import shutil
import xml.etree.ElementTree as ET
from misra_filter import filter_main
from pre_commit import read_yaml_file
import yaml
from tools import *
from uninstall import uninstall
def file_check(file_list, output_path):
    """
    Description: 
        Check the git project,If you what check your project that not a git project,you can use git init command to create a git repository.
    Return:
        None
    """
   
   # Get misra dir: install_path/cppcheck/misra
    misra_path = os.path.abspath(os.path.dirname(__file__))
    if os.path.exists(output_path):
        shutil.rmtree(output_path)
    os.mkdir(output_path)
    cppcheck_result_path = os.path.join(output_path, "cppcheck.xml")
    cppcheck_command = (
        f"cppcheck "
        f"{file_list} "
        f"--enable=style "
        f"--inline-suppr "
        f"--inconclusive "
        f"--addon={misra_path}/misra.json "
        f"--suppressions-list={misra_path}/suppressions.txt "
        f"--error-exitcode=0 "
        f"--output-file={cppcheck_result_path} "
        f"--xml "
        f"--quiet "
    )
    run_command(cppcheck_command)
    if not os.path.exists(cppcheck_result_path):
        log_error("cppcheck check failed.")
        return
    misra_result_path = os.path.join(output_path, "misra.xml")
    filter_main(cppcheck_result_path, misra_result_path)
    report_result_path = os.path.join(output_path, "report")
    html_report_command = (
        f"cppcheck-htmlreport "
        f"--title=file_check " #the title of report,it's not important
        f"--file={misra_result_path} "
        f"--report-dir={os.path.join(report_result_path, 'html')} "
        f"--source-encoding=iso8859-1 "
    )
    if os.name == 'posix':
        log_warning(f"making html report ...")
        run_command(html_report_command)
        if not os.path.exists(report_result_path):
            log_error("report result path not exists.")
            return
        else:
            log_success("make html report done.")
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
    cppcheck_command = (
        f"cppcheck "
        f"{module_path} "
        f"--enable=style "
        f"--inline-suppr "
        f"--inconclusive "
        f"--addon={misra_dir}/misra.json "
        f"--suppressions-list={misra_dir}/suppressions.txt "
        f"--error-exitcode=0 "
        f"--output-file={cppcheck_result_path} "
        f"--xml "
        f"--quiet "
    )
    #print(cppcheck_command)
    run_command(cppcheck_command)
    if not os.path.exists(cppcheck_result_path):
        log_error("cppcheck check failed.")
        return
    misra_result_path = os.path.join(output_path, "misra.xml")
    filter_main(cppcheck_result_path, misra_result_path)
    report_result_path = os.path.join(output_path, "report")
    html_report_command = (
        f"cppcheck-htmlreport "
        f"--title=\"{os.path.basename(module_path)}\""
        f" --file={misra_result_path} "
        f"--report-dir={os.path.join(report_result_path, 'html')} "
        f"--source-encoding=iso8859-1"
    )
    if os.name == 'posix':
        log_warning(f"making html report ...")
        run_command(html_report_command)
        if not os.path.exists(report_result_path):
            log_error("report result path not exists.")
            return
        else:
            log_success("make html report done.")
    return
def project_check(project_path, output_path):
    """
    Description: 
        Check the git project.
    Return:
        None
    """
    module_check(project_path, output_path)
def install_hook():
    install_path = os.path.dirname(__file__)
    template_path = os.path.join(install_path, ".pre-commit-config.yaml")
    shutil.copy(template_path, ".pre-commit-config.yaml")
    run_command("pre-commit clean")
    run_command("pre-commit install")
    log_success("install pre-commit hook done.")

def uninstall_hook():
    run_command("pre-commit uninstall")

def uninstall_cppcheck():
    remove_cppcheck()
def main():
    parser = argparse.ArgumentParser(description="Dscription: Cppcheck with Misra-C2012 support. A tool to check C/C++ code for MISRA violations.")
    group = parser.add_mutually_exclusive_group()
    # group.add_argument('--help', type=str, required=False,  help='Default: $HOME/cppcheck.  The [absolute path] where you want to install cppcheck-misra.')
    group.add_argument('--file', type=str, required=False, help='The file path list you what to check.File path splited by space.')
    group.add_argument('--module', type=str, required=False, help='The module path  you what to check.')
    group.add_argument('--project', type=str, required=False, help='The list of file path you what to check.')
    group.add_argument('--install_hook', action='store_true', required=False, help='Install pre-commit hook.')
    group.add_argument('--uinstall_hook', action='store_true', required=False, help='Remove pre-commit hook.')
    group.add_argument('--uninstall_misra', action='store_true', required=False, help='Uninstall this tool. Include: cppcheck, script.')
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
    else:
        print('Please use parameter "-h" to see help message!')
        return

if __name__ == '__main__':
    main()