import sys
from xml.dom.minidom import parse
import xml.dom.minidom

for i in range(1, len(sys.argv)):
    k = 1
    DOMTree = xml.dom.minidom.parse(sys.argv[i])
    Data = DOMTree.documentElement
    file_names = Data.getElementsByTagName("group")[0]
    file_name = file_names.getAttribute("name")
    file_name_c = file_name.lower() + ".c"
    array = "uint32_t " + file_name.lower() + "[OTAK_MAX+1] = {\n"
    output = open(file_name_c, 'w')
    output.write('#include "../key_api.h"\n')
    output.write('#include "gxtype.h"\n')
    output.write(array)
    keys = Data.getElementsByTagName("key")
    for key in keys:
        if key.hasAttribute("name"):
            key_name = key.getAttribute("name")
            if key_name == "GUIK_0" or key_name == "GUIK_1" \
            or key_name == "GUIK_2" or key_name == "GUIK_3" \
            or key_name == "GUIK_4" or key_name == "GUIK_5" \
            or key_name == "GUIK_6" or key_name == "GUIK_7" \
            or key_name == "GUIK_8" or key_name == "GUIK_9" \
            or key_name == "GUIK_RETURN" or key_name == "GUIK_UP" \
            or key_name == "GUIK_DOWN" or key_name == "GUIK_LEFT" \
            or key_name == "GUIK_RIGHT":
                key_value = key.firstChild.data
                output.write(key_value + ',')
                if k%4 == 0:
                    output.write("\n")
                k=k+1
    output.write("0xffff")
    output.write("\n")
    output.write("};\n")
    output.close();
output = open("key_api.h", 'w')
output.write("#ifndef __KEY_API_H__\n")
output.write("#define __KEY_API_H__\n")
output.write('#include "gxtype.h"\n\n')
output.write("typedef enum {\n")
output.write("      OTAK_0,\n")
output.write("      OTAK_1,\n")
output.write("      OTAK_2,\n")
output.write("      OTAK_3,\n")
output.write("      OTAK_4,\n")
output.write("      OTAK_5,\n")
output.write("      OTAK_6,\n")
output.write("      OTAK_7,\n")
output.write("      OTAK_8,\n")
output.write("      OTAK_9,\n")
output.write("      OTAK_OK,\n")
output.write("      OTAK_UP,\n")
output.write("      OTAK_DOWN,\n")
output.write("      OTAK_LEFT,\n")
output.write("      OTAK_RIGHT,\n")
output.write("      OTAK_MAX\n")
output.write("}key_type;\n")

for i in range(1, len(sys.argv)):
    DOMTree = xml.dom.minidom.parse(sys.argv[i])
    Data = DOMTree.documentElement
    file_names = Data.getElementsByTagName("group")[0]
    file_name = file_names.getAttribute("name")
    output.write("extern uint32_t " + file_name.lower() + "[OTAK_MAX+1];\n")

output.write("\nint32_t key_register(uint32_t key[OTAK_MAX+1]);\n")
output.write("key_type get_key(uint32_t code);\n")
output.write("#endif")
output.close();
