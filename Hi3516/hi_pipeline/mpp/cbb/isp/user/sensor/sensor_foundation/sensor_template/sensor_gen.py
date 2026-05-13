# -*- coding: utf-8 -*-
import os
import re
import sys

def replace_name():
    replace_name = sys.argv[1]
    for root, dirs, files in os.walk("."):
        for file in files:
            if file.endswith(".py"):
                continue
            file_path = os.path.join(root, file)
            with open(file_path, "r") as f:
                content = f.read()
            # replace template_sns with replace_name
            content = re.sub(r"template_sns", replace_name, content)
            # replace TEMPLATE_SNS with replace_name
            content = re.sub(r"TEMPLATE_SNS", replace_name.upper(), content)
            with open(file_path, "w") as f:
                f.write(content)
            # replace file name template_sns with replace_name
            if "template_sns" in file:
                new_file_path = os.path.join(root, file.replace("template_sns", replace_name))
                os.rename(file_path, new_file_path)

def replace_id():
    args = sys.argv[2]
    params = args.split("=")
    if len(params) != 2 or params[0] != "id":
        print("invaid input, id=7")
        exit(1)
    sensor_id = params[1]
    for root, dirs, files in os.walk("."):
        for file in files:
            if file.endswith(".py"):
                continue
            file_path = os.path.join(root, file)
            with open(file_path, "r") as f:
                content = f.read()
            # replace template_sns with replace_name
            content = content.replace("#define TEMPLATE_SNS_ID                    4", "#define TEMPLATE_SNS_ID {}".format(sensor_id))
            with open(file_path, "w") as f:
                f.write(content)

def replace_resulotion():
    args = sys.argv[3]
    params = args.split("=")
    if len(params) != 2 or params[0] != "resolution":
        print("invaid input, resolution=2688,1520")
        exit(1)
    reso = params[1]
    width, height = map(int, reso.split(","))
    print(width)
    print(height)
    for root, dirs, files in os.walk("."):
        for file in files:
            if file.endswith(".py"):
                continue
            file_path = os.path.join(root, file)
            with open(file_path, "r") as f:
                content = f.read()
            content = content.replace("#define SENSOR_TEMPLATE_SNS_WIDTH          2688", "#define SENSOR_TEMPLATE_SNS_WIDTH {}".format(width))
            content = content.replace("#define SENSOR_TEMPLATE_SNS_HEIGHT         1520", "#define SENSOR_TEMPLATE_SNS_HEIGHT {}".format(height))
            with open(file_path, "w") as f:
                f.write(content)

def replace_mode():
    args = sys.argv[4]
    params = args.split("=")
    if len(params) != 2 or params[0] != "mode":
        print("invaid input, mode=4m_30fps_12bit_linear or mode=4m_30fps_12bit_2to1_line_wdr")
        exit(1)
    sensor_mode = params[1]
    if 'wdr' in sensor_mode:
        print("sensor_mode:wdr!")
    else:
        print("sensor_mode:linear!")
    for root, dirs, files in os.walk("."):
        for file in files:
            if file.endswith(".py"):
                continue
            file_path = os.path.join(root, file)
            with open(file_path, "r") as f:
                content = f.read()
            if 'wdr' in sensor_mode:
                content = re.sub(r"4m_30fps_12bit_2to1_line_wdr", sensor_mode, content)
                content = re.sub(r"4M_30FPS_12BIT_2TO1_LINE_WDR", sensor_mode.upper(), content)
                if '2to1_frame_wdr' in sensor_mode:
                    print("sensor_mode:2to1 frame wdr!")
                    content = re.sub(r"2to1_line_wdr", "2to1_frame_wdr", content)
                    content = re.sub(r"2TO1_LINE_WDR", "2TO1_FRAME_WDR", content)
            else:
                content = re.sub(r"4m_30fps_12bit_linear", sensor_mode, content)
                content = re.sub(r"4M_30FPS_12BIT_LINEAR", sensor_mode.upper(), content)
            with open(file_path, "w") as f:
                f.write(content)

# def replace_mode_param():
#     args = sys.argv[2]
#     params = args.split("=")
#     if len(params) != 2 or params[0] != "param":
#         print("invalid input, param=1624,0x2610,0x0,30.0,5.0,2692,1524,0")
#         exit(1)
#     sensor_id = params[1]
#     for root, dirs, files in os.walk("."):
#         for file in files:
#             if file.endswith(".py"):
#                 continue
#             file_path = os.path.join(root, file)
#             with open(file_path, "r") as f:
#                 content = f.read()
#             content = content.replace("#define TEMPLATE_SNS_ID 4", "#define TEMPLATE_SNS_ID {}".format(sensor_id))
#             with open(file_path, "w") as f:
#                 f.write(content)

def main():
    if len(sys.argv) < 4:
        print("param error!")
        print("run: python sensor_gen.py sensor_name id=7 resolution=2688,1520 mode=4m_30fps_12bit_linear")
        print("run: python sensor_gen.py sensor_name id=7 resolution=2688,1520 mode=4m_30fps_12bit_2to1_line_wdr")
        exit()
    print("Starting sensor code generate...")
    replace_id()
    replace_resulotion()
    replace_mode()
    replace_name()
    print("Generate finished.")

if __name__ == "__main__":
    main()
