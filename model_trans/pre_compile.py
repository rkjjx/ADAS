# -*- coding: utf-8 -*- 

import sys

#if __name__ == '__main__':

if len(sys.argv) != 3:
    print('Usage: python {} xxx.rknn xxx.hw.rknn'.format(sys.argv[0]))
    print('Such as: python {} yolov5.rknn yolov5.pre.rknn'.format(sys.argv[0]))
    exit(1)

from rknn.api import RKNN

orig_rknn = sys.argv[1]
hw_rknn = sys.argv[2]

# Create RKNN object
rknn = RKNN()

# Load rknn model
print('--> Loading RKNN model')
ret = rknn.load_rknn(orig_rknn)
if ret != 0:
    print('Load RKNN model failed!')
    exit(ret)
print('done')

# Init runtime environment
print('--> Init runtime environment')


ret = rknn.init_runtime(target='rv1126', rknn2precompile=True)
if ret != 0:
    print('Init runtime environment failed')
    exit(ret)
print('done')

ret = rknn.export_rknn_precompile_model(hw_rknn)

rknn.release()

