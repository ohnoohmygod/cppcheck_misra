import xml.etree.ElementTree as ET
from get_modified_lines import GitDiffInfo
from get_modified_lines import get_modified_lines
from typing import Optional, List
from tools import *



def get_file(file: str, files: List[GitDiffInfo]) -> Optional[GitDiffInfo]:
    for f in files:
        if f.filename in file:
            return f

def parse_cppcheck_xml(file_path):
    """Parse the XML file and return the root element."""
    tree = ET.parse(file_path)
    root = tree.getroot()
    return root

def filter_misra_c2012_results(root, files=None):
    """
    Description:
        Filter results where the ID starts with 'MISRA-C2012'.(If files is not None, only save the modified lines.)
    Return:
        - the filtered results as a new XML element.
    Parameters:
        - files: the list of GitDiffInfo objects representing modified files.
    """
    # Create a new XML element for filtered results
    filtered_results = ET.Element("errors")
    
    for error in root.findall(".//error"):
        rule_id = error.get('id', '')
        if rule_id.startswith('misra-config') == False:
            # Clone the error element and add it to the filtered results
            if files: # 检测修改过的文件
                for location in  error.findall('location'):
                    file = location.get('file')
                    line = int(location.get('line'))
                    info = get_file(file, files)
                    if info is None:
                        continue
                    # 检测新文件的错误
                    if info.is_new_file:
                        filtered_results.append(error)
                    else:
                    # 旧文件检测修改的行
                        for start_line, num_lines in info.changes:
                            if line >= start_line and line < start_line + num_lines:
                                filtered_results.append(error)
            else:
                filtered_results.append(error)
    return filtered_results

def create_cppcheck_element(version):
    """Create a new 'cppcheck' element with a version attribute."""
    cppcheck_element = ET.Element("cppcheck", version=version)
    return cppcheck_element

def append_errors_root(filtered_results):
    """Create a new 'errors' root and append the filtered results."""
    errors_root = ET.Element("errors")
    errors_root.append(filtered_results)
    return errors_root

def save_filtered_results(cppcheck_element, errors_root, output_file):
    """Save the XML structure with the new 'cppcheck' and 'errors' roots to a file."""
    # Create a new root element
    root = ET.Element("results", version="2")
    
    # Append the 'cppcheck' and 'errors' elements to the root
    root.append(cppcheck_element)
    root.append(errors_root)
    
    # Write the tree to the output file
    tree = ET.ElementTree(root)
    tree.write(output_file, encoding='utf-8', xml_declaration=True)

def filter_main(input_file, output_file, hasChangedLines=False):
    """
    Description:
        Filter the results in the input XML file and save the filtered results to the output XML file.It only saves the misra errors.
    Parameters:
        - input_file: [in]The path to the input XML file.
        - output_file: [in]The path to the output XML file.
        - hasChangedLines:[in] Whether to filter the results based on modified lines.If True, only save errors in new / update  lines. This function is based on commond [git diff].
    Return:
        The number of misra errors in output file.

    """
    root = parse_cppcheck_xml(input_file)
    files = None
    if hasChangedLines:
        files = get_modified_lines()
    filtered_results = filter_misra_c2012_results(root, files)
    cppcheck_element = create_cppcheck_element("2.16.0")
    #errors_root = append_errors_root(filtered_results)
    save_filtered_results(cppcheck_element,filtered_results, output_file)
    log_success(f"misra results have been saved to: {output_file}")
    if 0 == len(filtered_results):
        log_success("there is no misra error")
    else:
        log_error(f"there are {len(filtered_results)} misra errors")
    return len(filtered_results)
    


