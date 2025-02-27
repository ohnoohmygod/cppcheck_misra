import os
from pathlib import Path
import subprocess
import yaml
from tools import *
import xml.etree.ElementTree as ET
from typing import List, Optional
class FullTask:
    def __init__(self, yaml_file):
        # 从YAML文件加载配置
        with open(yaml_file, 'r', encoding='utf-8') as file:
            config = yaml.safe_load(file)

        check_config = config.get('check_path', {})
        # 检测配置信息
        self.level = check_config.get('level', None)
        self.root_path = check_config.get('root_path', 'None')
        self.check_paths = None
        if check_config.get('include_path', None) is not None:
            self.check_paths = []
            for path in check_config.get('include_path'):
                self.check_paths.append(Path(self.root_path) / path)
        
        self.nocheck_paths = None
        if check_config.get('nocheck_path', None) is  not None:
            self.nocheck_paths = []
            for path in check_config.get('nocheck_path'):
                self.nocheck_paths.append(Path(self.root_path) / path)

        self.suppress = None
        if config.get('suppress', 'None') is not None:
            self.suppress = Path(__file__).parent / config.get('suppress', 'None')

        # 文件类型配置
        self.file_types = config.get('file_types', None)
        #misra插件配置
        self.misra = "misra.json"
        # 输出配置
        self.output_path = config.get('output_path', None)
        self.cppcheck_result = "cppcheck.xml"
        self.misra_result = "misra.xml"
        self.html_path =  "html"

        # 检查配置项是否为 None
        if any(value is None for value in [self.level, self.root_path, self.output_path, self.suppress, self.file_types]):
            raise ValueError("One or more configuration items are None")
        

    def __repr__(self):
        return (f"Task(level={self.level}, root_path={self.root_path}, "
                f"check_paths={self.check_paths}, nocheck_paths={self.nocheck_paths}, "
                f"output_path={self.output_path} "
                f"suppress={self.suppress}, file_types={self.file_types})")
    def shell(self, command):
        """
        @description  : 运行命令, 并返回输出。运行报错则打印异常。
        @param ------ command: 命令
        @Return ------- 命令输出
        """
        try:
            # 使用 subprocess.Popen 启动命令，并设置管道
            process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
            output_lines = []
            # 逐行读取命令的输出
            while True:
                line = process.stdout.readline()
                if not line:
                    break
                print(line.strip())  # 实时输出结果
                output_lines.append(line)
            # 等待命令执行完成
            process.wait()
            # 检查命令是否执行成功
            if process.returncode != 0:
                raise subprocess.CalledProcessError(process.returncode, command)
            # 将输出结果合并为一个字符串
            return ''.join(output_lines)
        except subprocess.CalledProcessError as e:
            print(f"命令执行出错: {e}")
            return None

    def get_command(self):
        # 需要检测的路径
        include_path = self.root_path
        if self.check_paths is  not None:
            include_path = ""
            for path in self.check_paths:
                include_path += str(Path(self.root_path) / path) + " "

        # 需要排除的路径
        exclude_path = ""
        if self.nocheck_paths is  not None:
            for path in self.nocheck_paths:
                exclude_path += f"-i{Path(self.root_path) / path} "

        # 抑制规则文件
        suppress_file = Path(__file__).parent / self.suppress

        # misra插件配置
        misra_addon = Path(__file__).parent / self.misra

        # 输出路径
        cppcheck_result_path = Path(self.output_path) / self.cppcheck_result

        cppcheck_command = (
            f"cppcheck "
            f"{include_path} "
            f"{exclude_path} "
            f"--enable={self.level} "
            f"--inline-suppr "
            f"--inconclusive "
            f"--addon={misra_addon} "
            f"--suppressions-list={suppress_file} "
            f"--error-exitcode=0 "
            f"--output-file={cppcheck_result_path} "
            f"--xml "
        )
        print(cppcheck_command)
        return cppcheck_command
    def run_filter(self):
        # 读取cppcheck结果
        cppcheck_result = Path(self.output_path) / self.cppcheck_result
        tree = ET.parse(cppcheck_result)
        root = tree.getroot()
        filtered_results = ET.Element("errors")
        errors_nums = 0
        for error in root.findall(".//error"):
            rule_id = error.get('id', '')
            if rule_id.startswith('misra-config') == False:
                filtered_results.append(error)
                errors_nums += 1
        newroot = ET.Element("results", version="2")
        cppcheck_element = ET.Element("cppcheck", version="2.16.0")
        # Append the 'cppcheck' and 'errors' elements to the root
        newroot.append(cppcheck_element)
        newroot.append(filtered_results)
        
        # Write the tree to the output file
        tree = ET.ElementTree(newroot)
        tree.write(Path(self.output_path) / self.misra_result , encoding='utf-8', xml_declaration=True)
        return errors_nums
    
    def make_html(self):

        misra_report_command = (
            f"cppcheck-htmlreport "
            f"--title=\"{os.path.basename(self.root_path)}\" "
            f" --file={Path(self.output_path) / self.misra_result} "
            f"--report-dir={Path(self.output_path) / self.html_path} "
            f"--source-encoding=iso8859-1"
        )
        # print(misra_report_command)
        run_command(misra_report_command)
        return
    def run_check(self):
        cppcheck_command = self.get_command()
        os.makedirs(self.output_path, exist_ok=True)
        self.shell(cppcheck_command)
        error_nums = self.run_filter()
        print("全量检测完成")
        print("错误数量 ", error_nums)
        if os.name == "posix":
            self.make_html()
        else:
            print("Windows平台下需要使用cppcheck-gui打开html报告")
            

class IncrementalTask:
    class GitDiffInfo:
        """
        Description:
            用于保存文件信息
        Attributes:
            filename: 文件名
            is_new_file: 是否为新文件
            changes: 改动点列表
        """
        def __init__(self, filename):
            self.filename = filename
            self.is_new_file = False
            self.changes = []
        def __repr__(self):
            return (f"GitDiffInfo(filename={self.filename}, is_new_file={self.is_new_file}, changes={self.changes})")
    def __init__(self, yaml_file):
        # 从YAML文件加载配置
        with open(yaml_file, 'r', encoding='utf-8') as file:
            config = yaml.safe_load(file)

        check_config = config.get('check_path', {})
        # 检测配置信息
        self.level = check_config.get('level', None)
        self.root_path = check_config.get('root_path', 'None')    
        self.suppress = None
        if config.get('suppress', 'None') is not None:
            self.suppress = Path(__file__).parent / config.get('suppress', 'None')

        # 不检测目录
        self.nocheck_paths = None
        if check_config.get('nocheck_path', None) is  not None:
            self.nocheck_paths = []
            for path in check_config.get('nocheck_path'):
                self.nocheck_paths.append(Path(self.root_path) / path)

        # 文件类型配置
        self.file_types = config.get('file_types', None)
        #misra插件配置
        self.misra = "misra.json"
        # 输出配置
        self.output_path = config.get('output_path', None)
        self.cppcheck_result = "cppcheck.xml"
        self.misra_result = "misra.xml"
        self.html_path =  "html"
        self.version = "2.16.0"

        # 检查配置项是否为 None
        if any(value is None for value in [self.level, self.root_path, self.output_path, self.suppress, self.file_types]):
            raise ValueError("One or more configuration items are None")
    def shell(self, command):
        """
        @description  : 运行命令, 并返回输出。运行报错则打印异常。
        @param ------ command: 命令
        @Return ------- 命令输出
        """
        try:
            # 使用 subprocess.Popen 启动命令，并设置管道
            process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
            output_lines = []
            # 逐行读取命令的输出
            while True:
                line = process.stdout.readline()
                if not line:
                    break
                print(line.strip())  # 实时输出结果
                output_lines.append(line)
            # 等待命令执行完成
            process.wait()
            # 检查命令是否执行成功
            if process.returncode != 0:
                raise subprocess.CalledProcessError(process.returncode, command)
            # 将输出结果合并为一个字符串
            return ''.join(output_lines)
        except subprocess.CalledProcessError as e:
            print(f"命令执行出错: {e}")
            return None

    def get_remote_branch_name(self):
        try:
            # 执行 Git 命令获取远程分支名
            # 命令的格式在linux和windows需要区分
            cmd = ''
            result = ''
            if os.name == "nt":
                cmd = '''git rev-parse --abbrev-ref "@{u}"'''
                result = subprocess.run(
                    cmd,
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )
            else:
                cmd = ['git', 'rev-parse', '--abbrev-ref', '@{u}']
                result = subprocess.run(
                    cmd,
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )

            return result.stdout.strip()
        except subprocess.CalledProcessError as e:
            # 处理命令执行错误
            print(f"Error: {e.stderr}")
            print("检测不到当前分支未设置上游分支，无法进行增量检测。")
            exit(1)
            return None
    def get_remote_newest_commit(self, remote_branch):
        # 解析remote_branch对应的commit hash
        command = f"git rev-parse {remote_branch}"
        result = run_command(command).strip()

        return result

    def get_modified_files(self):
        remote_branch = self.get_remote_branch_name()
        old_commit_hash = self.get_remote_newest_commit(remote_branch)
        root_dir = get_git_root_dir(os.getcwd())
        git_opt = f"--git-dir={os.path.join(self.root_path, '.git')}"

        current_commit = "HEAD"
        git_command = [ "git", "diff", "--name-only", old_commit_hash, current_commit]
        print("################### ", f"git diff --name-only {old_commit_hash} HEAD")
        # 尝试不同的解码方式
        encodings = [
            'utf-8',     # 通用编码，覆盖全球语言
            'ASCII'      # 基础英文编码（优先级需调整）
            'GB2312',    # 早期中文编码（已被GBK/GB18030覆盖，但可保留）
            'GBK',       # 中文扩展编码（兼容GB2312）
            'GB18030',   # 最新中文国家标准，覆盖最全
            'Big5',      # 繁体中文（台湾/香港地区）
            'ISO-8859-1', # 西欧语言
            'latin1',    # 类似ISO-8859-1
        ]
        for index, encoding in enumerate(encodings):
            try:
                result = subprocess.run(git_command, capture_output=True, universal_newlines=True, check=True, encoding=encoding)
                break
            except UnicodeDecodeError as e:
                print(f"尝试用{encoding}解码结果失败")
                if index == len(encodings) - 1:
                    print("尝试所有编码方式均失败，退出")
                    exit(1)
                continue
            except Exception as e:
                print("Error:", e)
                exit(1)

        output = result.stdout.strip()
        changed_files = []
        for file_path in output.split('\n'):
            if any(file_path.endswith(ext) for ext in self.file_types):
                changed_files.append(file_path)

        return changed_files


    def execute_git_diff(self):
        """
        Description:
            执行git diff命令，获取git仓库的改动信息
        Return:
            git diff的输出
        """
        remote_branch = self.get_remote_branch_name()
        old_commit_hash = self.get_remote_newest_commit(remote_branch)
        current_commit = "HEAD"
        git_command = [ "git", "diff", old_commit_hash, "-U0", current_commit]
        print("################### ", f"git diff {old_commit_hash} -U0 HEAD")
        # 下面尝试用不同的解码方式, 后续可以在此添加编码格式
        # 吉利项目用utf8，自研架构用GB2312
        encodings = [
            'utf-8',     # 通用编码，覆盖全球语言
            'ASCII'      # 基础英文编码（优先级需调整）
            'GB2312',    # 早期中文编码（已被GBK/GB18030覆盖，但可保留）
            'GBK',       # 中文扩展编码（兼容GB2312）
            'GB18030',   # 最新中文国家标准，覆盖最全
            'Big5',      # 繁体中文（台湾/香港地区）
            'ISO-8859-1', # 西欧语言
            'latin1',    # 类似ISO-8859-1
        ]
        for index, encoding in enumerate(encodings):
            try:
                result = subprocess.run(git_command, capture_output=True, universal_newlines=True, check=True, encoding=encoding)
                break
            except UnicodeDecodeError as e:
                print(f"尝试用{encoding}解码结果失败")
                if index == len(encodings) - 1:
                    print("尝试所有编码方式均失败，退出")
                    exit(1)
                continue
            except Exception as e:
                print("Error:", e)
                exit(1)    
        return result.stdout
    def parse_git_diff(self, diff_output):
        """
        Description:
            解析git diff的输出，返回一个GitDiffInfo类的列表
        Args:
            diff_output: git diff的输出
        Return:
            每个GitDiffInfo：一个修改的文件名以及修改点列表。
        """

    # 此函数为实现增量检查的核心逻辑， 得到增量的文件列表以及每个文件的改动点。

    # 如果下一行开头是diff，执行解析函数
    # 解析函数的逻辑：
    # 依次读取每一行
    # 第一行以diff --git开头，采用split函数，取最后一项为文件名
    # 第二行进行split分割，若以index开头，说明此文件为已有文件。若以new开头，说明是新文件。新文件的第三行仍是index，跳过不处理。
    # 跳过所有以+ - 开头的行
    # @开头的行，解析这几部分 "@@ -69,0 +70 @@"，得到+后面的内容，70说明改动发生在70行。如果该数字后面还有,n ，则说明改动的行数为70开始的n行。
        lines = diff_output.splitlines()
        # GitDiffInfo列表
        files = []
        info = None

        for i, line in enumerate(lines):
            if line.startswith("diff --git"):
                # 新文件的开始，创建新的 GitDiffInfo 对象
                if info:
                    files.append(info)
                parts = line.split()
                full_path = parts[-1]
                # 提取第一个 / 后的内容
                filename = full_path.partition('/')[2]
                info = self.GitDiffInfo(filename)
                info.is_new_file = False
            elif line.startswith("index"):
                continue
            elif line.startswith("new file mode"):
                info.is_new_file = True
                continue
            elif line.startswith("@@"):
                parts = line.split()
                range_info = parts[2].split(',')
                start_line = int(range_info[0][1:])
                if len(range_info) > 1:
                    num_lines = int(range_info[1])
                else:
                    num_lines = 1
                info.changes.append((start_line, num_lines))

        if info:
            files.append(info)

        return files


    def get_modified_lines(self):
        """
        Description:
            执行git diff命令, 获取git仓库的改动信息
        Return:
            git diff的输出
        """
        diff_output = self.execute_git_diff()
        #print("##################### ", diff_output)
        if not diff_output:
            print("No modified files found.")
            return []

        diffInfos = self.parse_git_diff(diff_output)
        if diffInfos == None:
            diffInfos = []
        return diffInfos
    
    def get_command(self, modified_files):
            
            # 抑制规则文件
            suppress_file = Path(__file__).parent / self.suppress

            # misra插件配置
            misra_addon = Path(__file__).parent / self.misra

            # 输出路径
            cppcheck_result_path = Path(self.output_path) / self.cppcheck_result

            modified_files = " ".join(modified_files)

            # 需要排除的路径
            exclude_path = ""
            if self.nocheck_paths is  not None:
                for path in self.nocheck_paths:
                    exclude_path += f"-i{Path(self.root_path) / path} "
            cppcheck_command = (
                f"cppcheck "
                f"{modified_files} "
                f"{exclude_path} "
                f"--enable={self.level} "
                f"--inline-suppr "
                f"--inconclusive "
                f"--addon={misra_addon} "
                f"--suppressions-list={suppress_file} "
                f"--error-exitcode=0 "
                f"--output-file={cppcheck_result_path} "
                f"--xml "
            )
            
            # print(cppcheck_result_path)
            return cppcheck_command
        
    

    def filter_results(self, modified_files, diffInfos): 
        tree = ET.parse(os.path.join(self.output_path, self.cppcheck_result))
        root = tree.getroot()
        filter_erros = ET.Element("errors")
        # print("modfied_files", modified_files)
        # print("diffinofs", diffInfos)
        if len(modified_files) != 0 and len(diffInfos) != 0:
            # 创建一个字典，用于快速查找文件名对应的 GitDiffInfo 对象
            diff_info_dict = {diff.filename: diff for diff in diffInfos}
            for error in root.findall(".//error"):
                rule_id = error.get('id', '')
                if rule_id.startswith('misra-config') == False:
                    # Clone the error element and add it to the filtered results
                    if modified_files: # 检测修改过的文件
                        for location in  error.findall('location'):
                            file = location.get('file')
                            line = int(location.get('line'))
                            # 从字典中查找文件名对应的 GitDiffInfo 对象
                            info = diff_info_dict.get(file)
                            # file不在modified_files中，跳过
                            if info is None:
                                continue 
                            
                            # 新文件
                            if info.is_new_file:
                                filter_erros.append(error)
                            else:
                            # 旧文件检测修改的行
                                for start_line, num_lines in info.changes:
                                    if line >= start_line and line < start_line + num_lines:
                                        filter_erros.append(error)
                    else:
                        break
        cppcheck_element = ET.Element("cppcheck", version=self.version)
        new_root = ET.Element("results", version="2")
        # 组装xml文件的结构
        new_root.append(cppcheck_element)
        new_root.append(filter_erros)

        # 写入xml文件
        new_tree = ET.ElementTree(new_root)
        new_tree.write(os.path.join(self.output_path, self.misra_result), encoding='utf-8', xml_declaration=True)
        
        error_nums = len(filter_erros)
        print(f"增量检查的结果已经被保存到： {self.output_path}")
        print(f"错误数量:{error_nums}")
        return error_nums

    def make_html(self):
        misra_report_command = (
            f"cppcheck-htmlreport "
            f"--title=\"{os.path.basename(self.root_path)}\" "
            f" --file={Path(self.output_path) / self.misra_result} "
            f"--report-dir={Path(self.output_path) / self.html_path} "
            f"--source-encoding=iso8859-1"
        )
        # print(misra_report_command)
        run_command(misra_report_command)
        return
        
    def run_check(self):
        # step 1: 获取差异文件
        modifed_files = self.get_modified_files()
        if 0 != len(modifed_files): 
        
            # step 2: 获取差异的行号
            diffInfo = self.get_modified_lines()

            # step 3: 执行cppcheck
            os.makedirs(self.output_path, exist_ok=True)
            cppcheck_commond = self.get_command(modifed_files)
            self.shell(cppcheck_commond)
            

            # step 4: 使用行号过滤cppcheck的结果
            error_nums = self.filter_results(modifed_files, diffInfo)

            # step 5: 生成html
            if os.name == "posix":
                self.make_html()
            else:
                print("Windows平台下需要使用cppcheck-gui打开xml报告进行可视化")

            # step 6: 添加commit-msg消息，在提交信息中添加错误数量
            self.add_check_result_to_msg(error_nums)

            print("------------增量检测完成------------")
            print(f"----------错误数量:{error_nums}--------")
            # step 6: 提交
            #git_commit_command = "git commit --amend --no-edit"
            #run_command()
        else:
            print("未发现符合检测类型的修改文件.")
        return 0
    
    def add_check_result_to_msg(self, error_nums):
        # 定义插入的内容格式
        error_format = "Cppcheck_Errors:   错误数量：{0}    检测类型: 增量    差异分支: {1} -> HEAD"
        try:
            # 获取当前项目的.git路径
            git_root_dir = get_git_root_dir(os.getcwd())
            git_dir = os.path.join(git_root_dir, ".git")
            commit_editmsg_path = os.path.join(git_dir, "COMMIT_EDITMSG")

            # 按行读取文件内容并过滤注释行
            with open(commit_editmsg_path, 'r', encoding='utf-8') as file:
                lines = [line for line in file.readlines() if not line.strip().startswith('#')]

            change_id_index = -1
            cppcheck_errors_index = -1

            # 查找 Change-Id: 和 Cppcheck_Errors: 的位置
            for i, line in enumerate(lines):
                if line.startswith("Change-Id:"):
                    change_id_index = i
                elif line.startswith("Cppcheck_Errors:"):
                    cppcheck_errors_index = i

            # 获取旧提交哈希
            remote_branch = self.get_remote_branch_name()
            old_commit_hash = self.get_remote_newest_commit(remote_branch)

            # 如果没有 Cppcheck_Errors:
            if cppcheck_errors_index == -1:
                if change_id_index != -1:
                    # 插入到 Change-Id 的上一行
                    lines.insert(change_id_index, error_format.format(error_nums, old_commit_hash) + "\n")
                else:
                    # 若 Change-Id 也不存在，插入到文件末尾
                    lines.append(error_format.format(error_nums, old_commit_hash) + "\n")
            # 如果有 Cppcheck_Errors:，更新错误数量
            else:
                lines[cppcheck_errors_index] = error_format.format(error_nums, old_commit_hash) + "\n"

            # 拼接lines内容为字符串
            commit_message = ''.join(lines).strip()
            # 构建git commit --amend -m命令
            # 执行命令
            subprocess.run(['git', 'commit', '--amend', '-m', commit_message])

        except FileNotFoundError:
            print(f"错误: 未找到 {commit_editmsg_path} 文件。")
        except Exception as e:
            print(f"发生未知错误: {e}")



 
            




 

        
        

# 使用示例
if __name__ == "__main__":

    # # 全量检测 测试
    # task = FullTask('misra/check-config.yaml')
    # task.run_check()
    # print(task.check_paths)
    # print(task.get_command())

    # 增量检测 测试
    inc_task = IncrementalTask('increment-check-config.yaml')
    # remote_branch = inc_task.get_remote_branch_name()
    # print("remote/branch: ", remote_branch)

    # old_commit_hash = inc_task.get_remote_newest_commit(remote_branch)
    # print("old_commit_hash: ", old_commit_hash)

    # modifed_files = inc_task.get_modified_files()
    # print("modifed_files: ", modifed_files)

    # cppcheck_command = inc_task.get_command(modifed_files)
    # print("cppcheck: ",cppcheck_command)
    # os.makedirs(inc_task.output_path, exist_ok=True)
    # run_command(cppcheck_command)
    # diffInfo = inc_task.get_modified_lines()
    # print("diffInfo: ", diffInfo)

    # error_nums = inc_task.filter_results(modifed_files, diffInfo)
    # print(error_nums)

    # # 插入到msg
    # inc_task.add_check_result_to_msg(error_nums)
    
    # inc_task.run_check()
    # inc_task.add_check_result_to_msg(0)
    inc_task.run_check()

    