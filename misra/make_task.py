from pathlib import Path
import yaml
from tools import *
import xml.etree.ElementTree as ET

class Task:
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
                f"output_path={self.output_path}, output_name={self.output_name}, "
                f"suppress={self.suppress}, file_types={self.file_types})")

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
        print(cppcheck_result_path)
        return cppcheck_command
    def run_filter(self):
        # 读取cppcheck结果
        cppcheck_result = Path(self.output_path) / self.cppcheck_result
        tree = ET.parse(cppcheck_result)
        root = tree.getroot()
        filtered_results = ET.Element("errors")
        
        for error in root.findall(".//error"):
            rule_id = error.get('id', '')
            if rule_id.startswith('misra-config') == False:
                filtered_results.append(error)
        newroot = ET.Element("results", version="2")
        cppcheck_element = ET.Element("cppcheck", version="2.16.0")
        # Append the 'cppcheck' and 'errors' elements to the root
        newroot.append(cppcheck_element)
        newroot.append(filtered_results)
        
        # Write the tree to the output file
        tree = ET.ElementTree(newroot)
        tree.write(Path(self.output_path) / self.misra_result , encoding='utf-8', xml_declaration=True)
    
    def make_html(self):
        misra_report_command = (
            f"cppcheck-htmlreport "
            f"--title=\"{os.path.basename(self.root_path)}\" "
            f" --file={Path(self.output_path) / self.cppcheck_result} "
            f"--report-dir={Path(self.output_path) / self.html_path} "
            f"--source-encoding=iso8859-1"
        )
        run_command(misra_report_command)
        return
    def run_check(self):
        cppcheck_command = self.get_command()
        os.makedirs(self.output_path, exist_ok=True)
        run_command(cppcheck_command)
        self.run_filter()
        self.make_html()

# 使用示例
if __name__ == "__main__":
    task = Task('misra/check-config.yaml')

    print(task.check_paths)
    print(task.get_command())