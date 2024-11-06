import xml.etree.ElementTree as ET
import chardet

def detect_encoding(file_path):
    """
    Detect the encoding of a file.
    """
    with open(file_path, 'rb') as f:
        raw_data = f.read()
    result = chardet.detect(raw_data)
    return result['encoding']

def add_suppress_comment(source_file, suppress_info):
    """
    Insert cppcheck suppress comments in the source file at the specified lines,
    while maintaining the original encoding.
    suppress_info: Dictionary with line number as the key and list of suppress messages as value.
    """
    # Detect the encoding of the file
    encoding = detect_encoding(source_file)

    # Read the source file with the detected encoding
    with open(source_file, 'r', encoding=encoding) as f:
        lines = f.readlines()
    # counting lines appended
    append_line = 0
    # Insert suppress comments before the issue lines
    for line_number, suppress_messages in sorted(suppress_info.items()):
        #print(f"Processing line_number{line_number}...")
        fwd_line = lines[line_number - 2 + append_line]
        #print(f"line:{fwd_line}")
        #print(f"suppress_messages: {suppress_messages}")
        new_suppress = suppress_messages
        if 'cppcheck-suppress' in fwd_line:
            old_suppress = fwd_line.replace(']\n', '').replace('// cppcheck-suppress [', '').split(',')
            #print(f"old_suppress: {old_suppress}")
            for s in new_suppress:
                if s not in old_suppress:
                    old_suppress.append(s)

            new_suppress = old_suppress
            #print(f"merged_suppress: {new_suppress}")
            lines.pop(line_number - 2 + append_line)
            append_line -= 1

        suppress_comment = f"// cppcheck-suppress [{', '.join(new_suppress)}]\n"
        lines.insert(line_number - 1 + append_line, suppress_comment)
        append_line += 1
        #print(f"line[-1]{lines[line_number - 1]}...")
        #print(f"line[line_number]{lines[line_number]}...")

    # Write the modified lines back to the file with the same encoding
    with open(source_file, 'w', encoding=encoding) as f:
        f.writelines(lines)

def process_cppcheck_report(xml_report):
    """
    Parse the cppcheck XML report and group suppress messages by file and line,
    ensuring each line gets only one suppress comment even if multiple errors exist.
    """
    tree = ET.parse(xml_report)
    root = tree.getroot()

    # Dictionary to store suppress info by file and line
    files_suppress_info = {}

    for error in root.findall('errors/error'):
        suppress_message = error.get('id')  # The message to suppress (e.g., 'variableScope', 'unusedFunction')
        severity = error.get('severity')  # Severity of the issue

        # Only suppress issues with severity of 'style' (customize as needed)
        if severity in ['style', 'performance', 'warning']:
            for location in error.findall('location'):
                source_file = location.get('file')
                line_number = int(location.get('line'))

                # Prepare the suppress info for this file and line
                if source_file not in files_suppress_info:
                    files_suppress_info[source_file] = {}

                if line_number not in files_suppress_info[source_file]:
                    files_suppress_info[source_file][line_number] = []

                # Avoid duplicate suppress messages for the same line
                if suppress_message not in files_suppress_info[source_file][line_number]:
                    files_suppress_info[source_file][line_number].append(suppress_message)

    # Add suppress comments to each file
    for source_file, suppress_info in files_suppress_info.items():
        print(f"Processing {source_file}...")
        add_suppress_comment(source_file, suppress_info)

if __name__ == "__main__":
    # Path to the cppcheck XML report
    xml_report = './cppcheck_results/misra.xml'

    # Process the cppcheck report and add suppress comments
    process_cppcheck_report(xml_report)

