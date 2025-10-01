# -*- coding: utf-8 -*-

import os
import urllib
import traceback
import time
import sys
import numpy as np
from rknn.api import RKNN
import cv2


ONNX_MODEL = './model_data/mobilefacenet/mobilefacenet.onnx'  # onnx 模型的路径
RKNN_MODEL = './model_data/mobilefacenet/mobilefacenet.rknn'  # 转换后的 RKNN 模型保存路径
DATASET = './model_data/webface/dataset.txt'  # 数据集文件路径

QUANTIZE_ON = False  # 是否进行量化

if __name__ == '__main__':

    # 创建 RKNN 对象
    print('start')
    rknn = RKNN(verbose=True)
    print('end')
    # 检查 ONNX 模型文件是否存在
    if not os.path.exists(ONNX_MODEL):
        print('model not exist')
        exit(-1)
    else:
        print('model exist')

    # 配置模型预处理参数
    print('--> Config model')
    rknn.config(reorder_channel='0 1 2',  # 表示 RGB 通道
                mean_values=[[0, 0, 0]],  # 每个通道的像素均值，预处理时对应通道减去该值
                std_values=[[255, 255, 255]],  # 每个通道的像素标准差，每个通道除以该值
                optimization_level=3,  # 优化级别
                target_platform='rv1126',  # 指定目标平台为rv1126
                output_optimize=1,  # 输出优化为真
                quantize_input_node=QUANTIZE_ON)  # 对时输入节点进行量化
    print('done')

    # 加载 ONNX 模型
    print('--> Loading model')
    ret = rknn.load_onnx(model=ONNX_MODEL,outputs=['output'])#
    if ret != 0:
        print('Load yolov5 failed!')
        exit(ret)
    print('done')

    # 构建模型
    print('--> Building model')
    ret = rknn.build(do_quantization=QUANTIZE_ON,dataset=DATASET)
    if ret != 0:
        print('Build yolov5 failed!')
        exit(ret)
    print('done')

    # 导出 RKNN 模型
    print('--> Export RKNN model')
    ret = rknn.export_rknn(RKNN_MODEL)
    if ret != 0:
        print('Export yolov5rknn failed!')
        exit(ret)
    print('done')
