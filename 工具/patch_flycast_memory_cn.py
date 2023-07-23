import subprocess
import socket
import argparse
import xml.etree.ElementTree as ET

def get_ip_address(xml_file):
    # 从XML文件中获取hostaddress元素的addr4属性值
    try:
        tree = ET.parse(xml_file)
        root = tree.getroot()

        hostaddress = root.find("psocn_server_config/hostaddress")
        if hostaddress is not None and "addr4" in hostaddress.attrib:
            return hostaddress.attrib["addr4"]
    except Exception as e:
        pass

    return None

def replace_destination(original_destination, new_destination):
    # 查找并替换指定的目标地址
    process = subprocess.Popen(['memwatch', 'Flycast.app', 'find', original_destination], stdout=subprocess.PIPE)
    output = process.communicate()[0].decode('utf-8')

    for line in output.splitlines():
        parts = line.split()
        if parts:
            address = parts[0]
            write_command = ['memwatch', 'Flycast.app', 'write', address, new_destination, '00']
            subprocess.Popen(write_command)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='在Flycast中替换目标地址')
    parser.add_argument('original_destination', help='原始目标地址')
    parser.add_argument('xml_file', help='XML文件路径')

    args = parser.parse_args()

    # 从XML文件中获取新的目标地址
    new_destination = get_ip_address(args.xml_file)

    if new_destination is None:
        print('无法从XML文件中获取新的目标地址')
        exit(1)

    print(f'正在查找 "{args.original_destination}" 的出现次数')
    replace_destination(args.original_destination, new_destination)
