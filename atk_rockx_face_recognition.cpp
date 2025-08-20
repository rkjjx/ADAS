/****************************************************************************
*
*    Copyright (c) 2017 - 2019 by Rockchip Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Rockchip Corporation. This is proprietary information owned by
*    Rockchip Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Rockchip Corporation.
*
*****************************************************************************/
#include "atk_rockx_face_recognition.h"
#include <cmath>  
#include <math.h>

bool atk_face_recognition_quit = false;
int size, x_start, y_start;
std::unordered_map<std::string, int> name_lnumber_map;
RK_U32 video_width = 720;
RK_U32 video_height = 1280;
void normalize(float* vec, int size) {
    float magnitude = 0.0f;

    // 计算向量的模
    for (int i = 0; i < size; ++i) {
        magnitude += vec[i] * vec[i];
    }
    magnitude = sqrtf(magnitude);

    // 防止除以0的情况
    const float epsilon = 1e-10f;
    if (magnitude < epsilon) {
        return;  // 如果模接近0，直接返回，表示无法归一化
    }

    // 归一化向量
    for (int i = 0; i < size; ++i) {
        vec[i] /= magnitude;
    }
}

float cosine_similarity(float* vec1, float* vec2, int size) {
    // 先对 vec1 和 vec2 进行归一化
    normalize(vec1, size);
    normalize(vec2, size);

    float dot_product = 0.0f;

    // 计算点积
    for (int i = 0; i < size; ++i) {
        dot_product += vec1[i] * vec2[i];
    }

    // 返回余弦相似度（归一化后点积即为余弦相似度）
    return dot_product;
}
float euclidean_distance_normalized(float* vec1, float* vec2, int size) {
    // 对 vec1 和 vec2 进行归一化
    normalize(vec1, size);
    normalize(vec2, size);

    // 计算欧氏距离
    float sum = 0.0f;
    for (int i = 0; i < size; ++i) {
        float diff = vec1[i] - vec2[i];
        sum += diff * diff;
    }

    return std::sqrt(sum);  // 返回平方和的平方根
}

FaceRecognitionFrame *GetFaceRecognitionMediaBuffer() {
    FaceRecognitionFrame *frame;

    frame = (FaceRecognitionFrame*)malloc(sizeof (FaceRecognitionFrame));

    MEDIA_BUFFER mb = NULL;
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, 0, -1);
    if (!mb) {
        printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
        return NULL;
    }

    MB_IMAGE_INFO_S stImageInfo = {0};
    int ret = RK_MPI_MB_GetImageInfo(mb, &stImageInfo);
    if (ret)
        printf("Warn: Get image info failed! ret = %d\n", ret);
#if 0
    printf("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",
           RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb), stImageInfo.u32Width,
           stImageInfo.u32Height, stImageInfo.enImgType);
#endif
    frame->file = RK_MPI_MB_GetPtr(mb);
    frame->size = RK_MPI_MB_GetSize(mb);

    RK_MPI_MB_ReleaseBuffer(mb);

    return frame;
}

FaceRecognitionFrame *GetFace() {
    FaceRecognitionFrame *frame;

    frame = (FaceRecognitionFrame*)malloc(sizeof (FaceRecognitionFrame));

    MEDIA_BUFFER src_mb = NULL;
    src_mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, 1, -1);
    if (!src_mb) {
        printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
        return NULL;
    }
    im_rect src_rect = {x_start, y_start, size, size};
    unsigned char *input_rgb = (unsigned char *)RK_MPI_MB_GetPtr(src_mb);
    unsigned char *output_rgb = (unsigned char *)malloc(size * size * 3);
    rga_buffer_t src = wrapbuffer_virtualaddr(input_rgb, 720, 1280, RK_FORMAT_RGB_888);
    rga_buffer_t dst = wrapbuffer_virtualaddr(output_rgb, size, size, RK_FORMAT_RGB_888);  
    IM_STATUS STATUS = imcrop(src, dst, src_rect);
    if (STATUS != IM_STATUS_SUCCESS)
    {
       printf("imcrop failed: %s\n", imStrError(STATUS));
       return NULL;
    }
    //imresize
    unsigned char *resize_buf = (unsigned char *)malloc(112 * 112 * 3);
    rga_buffer_t resize_dst = wrapbuffer_virtualaddr(resize_buf, 112, 112, RK_FORMAT_RGB_888);
    STATUS = imresize(dst, resize_dst, (double)112/size, (double)112/size);
    if (STATUS != IM_STATUS_SUCCESS)
    {
       printf("imcrop failed: %s\n", imStrError(STATUS));
       return NULL;
    }
    

    frame->file = resize_dst.vir_addr;
    frame->size = 112 * 112 * 3;

    RK_MPI_MB_ReleaseBuffer(src_mb);

    return frame;
}

int atk_recognition_init() 
{
  int disp_width = 720;
  int disp_height = 1280;

  RK_CHAR *pDeviceName = "rkispp_scale0";
  RK_CHAR *pcDevNode = "/dev/dri/card0";
  char *iq_file_dir = "/etc/iqfiles";
  RK_S32 s32CamId = 0;
  RK_U32 u32BufCnt = 3;
  RK_U32 fps = 20;
  int ret;
  pthread_t rkmedia_vi_face_tidp;
  RK_BOOL bMultictx = RK_FALSE;
  
  printf("\n###############################################\n");
  printf("VI CameraIdx: %d\npDeviceName: %s\nResolution: %dx%d\n\n",
          s32CamId,pDeviceName,video_width,video_height);
  printf("VO pcDevNode: %s\nResolution: %dx%d\n",
          pcDevNode,disp_height,disp_width);
  printf("###############################################\n\n");

  if (iq_file_dir) 
  {
#ifdef RKAIQ
    printf("#Rkaiq XML DirPath: %s\n", iq_file_dir);
    printf("#bMultictx: %d\n\n", bMultictx);
    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, iq_file_dir);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
#endif
  }

  open_db();
  
  RK_MPI_SYS_Init();
  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = pDeviceName;
  vi_chn_attr.u32BufCnt = u32BufCnt;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 1, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 1);
  if (ret) 
  {
    printf("ERROR: create VI[0:1] error! ret=%d\n", ret);
    return -1;
  }
  RGA_ATTR_S stRgaAttr1;
  memset(&stRgaAttr1, 0, sizeof(stRgaAttr1));
  stRgaAttr1.bEnBufPool = RK_TRUE;
  stRgaAttr1.u16BufPoolCnt = 3;
  stRgaAttr1.u16Rotaion = 270;
  stRgaAttr1.stImgIn.u32X = 0;
  stRgaAttr1.stImgIn.u32Y = 0;
  stRgaAttr1.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr1.stImgIn.u32Width = video_width;
  stRgaAttr1.stImgIn.u32Height = video_height;
  stRgaAttr1.stImgIn.u32HorStride = video_width;
  stRgaAttr1.stImgIn.u32VirStride = video_height;
  stRgaAttr1.stImgOut.u32X = 0;
  stRgaAttr1.stImgOut.u32Y = 0;
  stRgaAttr1.stImgOut.imgType = IMAGE_TYPE_BGR888;
  stRgaAttr1.stImgOut.u32Width = video_width;
  stRgaAttr1.stImgOut.u32Height = video_height;
  stRgaAttr1.stImgOut.u32HorStride = video_width;
  stRgaAttr1.stImgOut.u32VirStride = video_height;
  ret = RK_MPI_RGA_CreateChn(1, &stRgaAttr1);
  if (ret) {
      printf("ERROR: create RGA[0:1] falied! ret=%d\n", ret);
      return -1;
  }
  
  RGA_ATTR_S stRgaAttr;
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 3;
  stRgaAttr.u16Rotaion = 0;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_BGR888;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_height;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_BGR888;
  stRgaAttr.stImgOut.u32Width = video_width;
  stRgaAttr.stImgOut.u32Height = video_height;
  stRgaAttr.stImgOut.u32HorStride = video_width;
  stRgaAttr.stImgOut.u32VirStride = video_height;
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
      printf("ERROR: create RGA[0:0] falied! ret=%d\n", ret);
      return -1;
  }
  
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
  printf("Bind VI[0:1] to RGA[0:1]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32DevId = s32CamId;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) 
  {
    printf("ERROR: Bind VI[0:1] to RGA[0:1] failed! ret=%d\n", ret);
    return -1;
  }
  pthread_create(&rkmedia_vi_face_tidp, NULL, rkmedia_vi_face_thread, NULL);
  printf("%s initial finish\n", __func__);
  
  
  
  while (!atk_face_recognition_quit) 
  {
    usleep(500000);//0.5s
  }

  printf("%s exit!\n", __func__);
  printf("Unbind VI[0:1] to RGA[0:1]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stSrcChn.s32DevId = s32CamId;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) 
  {
    printf("ERROR: unbind VI[0:1] to RGA[0:1] failed! ret=%d\n", ret);
    return -1;
  }
  RK_MPI_RGA_DestroyChn(1);
  RK_MPI_RGA_DestroyChn(0);
  printf("Destroy VI[0:1] channel\n");
  ret = RK_MPI_VI_DisableChn(s32CamId, 1);
  if (ret) 
  {
    printf("ERROR: destroy VI[0:1] error! ret=%d\n", ret);
    return -1;
  }

  if (iq_file_dir) 
  {
#if RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }
  return 0;
}
static unsigned char *load_model(const char *filename, int *model_size)
{
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    printf("fopen %s fail!\n", filename);
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  unsigned int model_len = ftell(fp);
  unsigned char *model = (unsigned char *)malloc(model_len);
  fseek(fp, 0, SEEK_SET);

  if (model_len != fread(model, 1, model_len, fp))
  {
    printf("fread %s fail!\n", filename);
    free(model);
    return NULL;
  }
  *model_size = model_len;

  if (fp)
  {
    fclose(fp);
  }
  return model;
}


static void printRKNNTensor(rknn_tensor_attr *attr)
{
  printf("index=%d name=%s n_dims=%d dims=[%d %d %d %d] n_elems=%d size=%d "
         "fmt=%d type=%d qnt_type=%d fl=%d zp=%d scale=%f\n",
         attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2],
         attr->dims[1], attr->dims[0], attr->n_elems, attr->size, 0, attr->type,
         attr->qnt_type, attr->fl, attr->zp, attr->scale);
}


void *rkmedia_vi_face_thread(void *args) 
{
  pthread_detach(pthread_self());//将线程状态改为unjoinable状态，确保资源的释放
  map<string, rockx_face_feature_t> database_face_map = FaceFeature();
  map<string, rockx_face_feature_t>::iterator database_iter;
  int ret;
  rknn_context ctx;
  int model_len = 0;
  unsigned char *model;
  static char *model_path = "/demo/bin/best.pre.rknn";
  
  // Load RKNN Model
  printf("Loading best.pre.rknn model ...\n");            
  model = load_model(model_path, &model_len);
  ret = rknn_init(&ctx, model, model_len, 0);
  if (ret < 0)
  {
    printf("rknn_init fail! ret=%d\n", ret);
    return NULL;
  }

  // Get Model Input Output Info
  rknn_input_output_num io_num;
  ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
  if (ret != RKNN_SUCC)
  {
    printf("rknn_query fail! ret=%d\n", ret);
    return NULL;
  }
  printf("model input num: %d, output num: %d\n", io_num.n_input,io_num.n_output);

  // print input tensor
  printf("input tensors:\n");
  rknn_tensor_attr input_attrs[io_num.n_input];
  memset(input_attrs, 0, sizeof(input_attrs));
  for (unsigned int i = 0; i < io_num.n_input; i++)
  {
    input_attrs[i].index = i;
    ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]),sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC)
    {
      printf("rknn_query fail! ret=%d\n", ret);
      return NULL;
    }
    printRKNNTensor(&(input_attrs[i]));
  }

  // print output tensor
  printf("output tensors:\n");
  rknn_tensor_attr output_attrs[io_num.n_output];
  memset(output_attrs, 0, sizeof(output_attrs));
  for (unsigned int i = 0; i < io_num.n_output; i++)
  {
    output_attrs[i].index = i;
    ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]),sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC)
    {
      printf("rknn_query fail! ret=%d\n", ret);
      return NULL;
    }
    printRKNNTensor(&(output_attrs[i]));
  }

  // get model's input image width and height
  int channel = 3;
  int width = 0;
  int height = 0;
  if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
  {
      printf("model is NCHW input fmt\n");
      width = input_attrs[0].dims[0];
      height = input_attrs[0].dims[1];
  }
  else
  {
      printf("model is NHWC input fmt\n");
      width = input_attrs[0].dims[1];
      height = input_attrs[0].dims[2];
  }
  printf("model input height=%d, width=%d, channel=%d\n", height, width, channel);
  
  int frame_cnt = 0;
  int frame_cnt_eye = 0;
  int nap_count = 0;
  
  float res_landmk[98][2];
  float res_angle[3];
  int x, y;
  
  
  rknn_context ctx_1;
  int model_len_1 = 0;
  unsigned char *model_1;
  static char *model_path_1 = "/demo/bin/pfpld.pre.rknn";
   
  rknn_context ctx_2;
  int model_len_2 = 0;
  unsigned char *model_2;
  static char *model_path_2 = "/demo/bin/face_recognition.pre.rknn";
  string face_name = "";
  // Load RKNN Model
  printf("Loading pfpld.pre.rknn model ...\n");            
  model_1 = load_model(model_path_1, &model_len_1);
  ret = rknn_init(&ctx_1, model_1, model_len_1, 0);
  if (ret < 0)
  {
    printf("rknn_init fail! ret=%d\n", ret);
    return NULL;
  }
  
  // Get Model Input Output Info
  rknn_input_output_num io_num_1;
  ret = rknn_query(ctx_1, RKNN_QUERY_IN_OUT_NUM, &io_num_1, sizeof(io_num_1));
  if (ret != RKNN_SUCC)
  {
    printf("rknn_query fail! ret=%d\n", ret);
    return NULL;
  }
  printf("model input num: %d, output num: %d\n", io_num_1.n_input,io_num_1.n_output);

  // print input tensor
  printf("input tensors:\n");
  rknn_tensor_attr input_attrs_1[io_num_1.n_input];
  memset(input_attrs_1, 0, sizeof(input_attrs_1));
  for (unsigned int i = 0; i < io_num_1.n_input; i++)
  {
    input_attrs_1[i].index = i;
    ret = rknn_query(ctx_1, RKNN_QUERY_INPUT_ATTR, &(input_attrs_1[i]),sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC)
    {
      printf("rknn_query fail! ret=%d\n", ret);
      return NULL;
    }
    printRKNNTensor(&(input_attrs_1[i]));
  }

  // print output tensor
  printf("output tensors:\n");
  rknn_tensor_attr output_attrs_1[io_num_1.n_output];
  memset(output_attrs_1, 0, sizeof(output_attrs_1));
  for (unsigned int i = 0; i < io_num_1.n_output; i++)
  {
    output_attrs_1[i].index = i;
    ret = rknn_query(ctx_1, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs_1[i]),sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC)
    {
      printf("rknn_query fail! ret=%d\n", ret);
      return NULL;
    }
    printRKNNTensor(&(output_attrs_1[i]));
  }

  // get model's input image width and height
  int channel_1 = 3;
  int width_1 = 0;
  int height_1 = 0;
  if (input_attrs_1[0].fmt == RKNN_TENSOR_NCHW)
  {
      printf("model is NCHW input fmt\n");
      width_1 = input_attrs_1[0].dims[0];
      height_1 = input_attrs_1[0].dims[1];
  }
  else
  {
      printf("model is NHWC input fmt\n");
      width_1 = input_attrs_1[0].dims[1];
      height_1 = input_attrs_1[0].dims[2];
  }
  printf("model input height_1=%d, width_1=%d, channel_1=%d\n", height_1, width_1, channel_1);
  
  // Load RKNN Model
  printf("Loading face_recognition.pre.rknn model ...\n");            
  model_2 = load_model(model_path_2, &model_len_2);
  ret = rknn_init(&ctx_2, model_2, model_len_2, 0);
  if (ret < 0)
  {
    printf("rknn_init fail! ret=%d\n", ret);
    return NULL;
  }
  
  while (!atk_face_recognition_quit) 
  {
    MEDIA_BUFFER src_mb = NULL;
    frame_cnt += 1;
    frame_cnt_eye += 1;
    src_mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, 1, -1);
    if (!src_mb) 
    {
      printf("ERROR: RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }
    
    rga_context rga_ctx;
    drm_context drm_ctx;
    memset(&rga_ctx, 0, sizeof(rga_context));
    memset(&drm_ctx, 0, sizeof(drm_context));

     // DRM alloc buffer
    int drm_fd = -1;
    int buf_fd = -1; // converted from buffer handle
    unsigned int handle;
    size_t actual_size = 0;
    void *drm_buf = NULL;

    drm_fd = drm_init(&drm_ctx);
    drm_buf = drm_buf_alloc(&drm_ctx, drm_fd, video_width, video_height, channel * 8, &buf_fd, &handle, &actual_size);
    memcpy(drm_buf, (uint8_t *)RK_MPI_MB_GetPtr(src_mb) , video_width * video_height * channel);
    void *resize_buf = malloc(height * width * channel);
    // init rga context
    RGA_init(&rga_ctx);
    img_resize_slow(&rga_ctx, drm_buf, video_width, video_height, resize_buf, width, height);
    uint32_t input_model_image_size = width * height * channel;

    // Set Input Data
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = input_model_image_size;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = resize_buf;
    ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
    if (ret < 0)
    {
      printf("ERROR: rknn_inputs_set fail! ret=%d\n", ret);
      return NULL;
    }
    free(resize_buf);
    drm_buf_destroy(&drm_ctx, drm_fd, buf_fd, handle, drm_buf, actual_size);
    drm_deinit(&drm_ctx, drm_fd);
    RGA_deinit(&rga_ctx);
    
    //printf("best rknn_run start %d times.\n", ++count);
    ret = rknn_run(ctx, nullptr);
    //printf("best rknn_run end %d times.\n", count);
    if (ret < 0)
    {
      printf("ERROR: rknn_run fail! ret=%d\n", ret);
      return NULL;
    }

    // Get Output
    rknn_output outputs[io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        outputs[i].want_float = 0;
    }
    ret = rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
    if (ret < 0)
    {
      printf("ERROR: rknn_outputs_get fail! ret=%d\n", ret);
      return NULL;
    }

    detect_result_group_t detect_result_group;
    memset(&detect_result_group, 0, sizeof(detect_result_group));
    std::vector<float> out_scales;
    std::vector<uint8_t> out_zps;
    for (int i = 0; i < io_num.n_output; ++i)
    {
        out_scales.push_back(output_attrs[i].scale);
        out_zps.push_back(output_attrs[i].zp);
    }
    
    const float vis_threshold = 0.1;
    const float nms_threshold = 0.1;
    const float conf_threshold = 0.4;
    float scale_w = (float)width / video_width;
    float scale_h = (float)height / video_height;

    post_process((uint8_t *)outputs[0].buf, (uint8_t *)outputs[1].buf, (uint8_t *)outputs[2].buf, height, width,
                 conf_threshold, nms_threshold, vis_threshold, scale_w, scale_h, out_zps, out_scales, &detect_result_group);
    
    for (int i = 0; i < detect_result_group.count; i++)
    {
      detect_result_t *det_result = &(detect_result_group.results[i]);
      int left = det_result->box.left;
      int top = det_result->box.top;
      int right = det_result->box.right;
      int bottom = det_result->box.bottom;
      if (left < 0)
      {
        left = 0;
      }
      if (top < 0)
      {
        top = 0;
      }
      
      if(!strncmp(detect_result_group.results[i].name,"face", OBJ_NUMB_MAX_SIZE))
      {
        //imcrop
        if((right - left) > (bottom - top))
        {
            size = 1.1 * (right - left);
        }else
        {
            size = 1.1 * (bottom - top);
        }
        size = ((size / 112) + 1) * 112;
        x_start = ((right + left) / 2) - size/2 > 2 ? ((right + left) / 2) - size/2 : 2;
        y_start = ((bottom + top) / 2) - size/2 > 2 ? ((bottom + top) / 2) - size/2 : 2;
        while(size + x_start > 720 || size + y_start > 1280)
        {
            size -= 112;
            x_start = ((right + left) / 2) - size/2 > 2 ? ((right + left) / 2) - size/2 : 2;
            y_start = ((bottom + top) / 2) - size/2 > 2 ? ((bottom + top) / 2) - size/2 : 2;
        }
        if(size == 0)
        {
          continue;
        }
        im_rect src_rect = {x_start, y_start, size, size};
        unsigned char *input_rgb = (unsigned char *)RK_MPI_MB_GetPtr(src_mb);
        unsigned char *output_rgb = (unsigned char *)malloc(size * size * channel);
        rga_buffer_t src = wrapbuffer_virtualaddr(input_rgb, video_width, video_height, RK_FORMAT_RGB_888);
        rga_buffer_t dst = wrapbuffer_virtualaddr(output_rgb, size, size, RK_FORMAT_RGB_888);  
        IM_STATUS STATUS = imcrop(src, dst, src_rect);
        if (STATUS != IM_STATUS_SUCCESS)
        {
           printf("imcrop failed: %s\n", imStrError(STATUS));
           return NULL;
        }
        //imresize
        unsigned char *resize_buf = (unsigned char *)malloc(width_1 * height_1 * channel_1);
        rga_buffer_t resize_dst = wrapbuffer_virtualaddr(resize_buf, width_1, height_1, RK_FORMAT_RGB_888);
        STATUS = imresize(dst, resize_dst, (double)112/size, (double)112/size);
        if (STATUS != IM_STATUS_SUCCESS)
        {
           printf("imcrop failed: %s\n", imStrError(STATUS));
           return NULL;
        }
        //set_input
        rknn_input inputs[1];
        memset(inputs, 0, sizeof(inputs));
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_UINT8;
        inputs[0].size = width_1 * height_1 * channel_1;
        inputs[0].fmt = RKNN_TENSOR_NHWC;
        inputs[0].buf = resize_buf;
        ret = rknn_inputs_set(ctx_1, io_num_1.n_input, inputs);
        if (ret < 0)
        {
          printf("ERROR: rknn_inputs_set fail! ret=%d\n", ret);
          return NULL;
        }
        
        // Run
        ret = rknn_run(ctx_1, nullptr);
        if (ret < 0)
        {
          printf("ERROR: rknn_run fail! ret=%d\n", ret);
          return NULL;
        }
        // Get Output
        rknn_output outputs_1[io_num_1.n_output];
        memset(outputs_1, 0, sizeof(outputs_1));
        for (int i = 0; i < io_num_1.n_output; i++)
        {
            outputs_1[i].want_float = 1;
        }
        ret = rknn_outputs_get(ctx_1, io_num_1.n_output, outputs_1, NULL);
        if (ret < 0)
        {
          printf("ERROR: rknn_outputs_get fail! ret=%d\n", ret);
          return NULL;
        }
        get_result_hcb((float*)outputs_1[1].buf, &res_landmk, (float*)outputs_1[0].buf, &res_angle);
        nap_count = nap_test(res_landmk, res_angle, &frame_cnt_eye, &frame_cnt);
        // Set Input Data
        rknn_input inputs_2[1];
        memset(inputs_2, 0, sizeof(inputs_2));
        inputs_2[0].index = 0;
        inputs_2[0].type = RKNN_TENSOR_UINT8;
        inputs_2[0].size = 37632;
        inputs_2[0].fmt = RKNN_TENSOR_NHWC;
        inputs_2[0].buf = resize_buf;
        ret = rknn_inputs_set(ctx_2, 1, inputs_2);
        if (ret < 0)
        {
          printf("ERROR: rknn_inputs_set fail! ret=%d\n", ret);
          return NULL;
        }
        free(resize_buf);
        free(output_rgb);
      
        ret = rknn_run(ctx_2, nullptr);
        if (ret < 0)
        {
          printf("ERROR: rknn_run fail! ret=%d\n", ret);
          return NULL;
        }
  
        // Get Output
        rknn_output outputs_2[1];
        memset(outputs_2, 0, sizeof(outputs_2));
        for (int i = 0; i < 1; i++)
        {
            outputs_2[i].want_float = 1;
        }
        ret = rknn_outputs_get(ctx_2, 1, outputs_2, NULL);
        if (ret < 0)
        {
          printf("ERROR: rknn_outputs_get fail! ret=%d\n", ret);
          return NULL;
        }
        /*
        float res[512];
        uint8_t *ptr = (uint8_t *)outputs_2[0].buf;
        for(int i = 0; i < 512; i++)
        {
          res[i] = ((float)(*ptr++) - 124)*0.020613;
        }
        */
        rknn_outputs_release(ctx_2, 1, outputs_2);
        
        float similarity = 0.0;
        float max_similarity = 0.0;
        std::string best_match = ""; // 用于存储相似度最高的 name

        for (database_iter = database_face_map.begin(); database_iter != database_face_map.end(); database_iter++) 
        {
            // 计算相似度
            similarity = cosine_similarity(database_iter->second.feature, (float*)outputs_2[0].buf, 512);
            //printf("%s simple_value = %f\n", database_iter->first.c_str(), similarity); // 使用 c_str() 将 std::string 转为 C 风格字符串

            // 如果当前相似度大于最大相似度，则更新最大相似度和对应的 name
            if (similarity > max_similarity) 
            {
                max_similarity = similarity;
                best_match = database_iter->first;
            }
        }
        // 遍历完成后，判断最大相似度是否符合条件
        if (max_similarity >= 0.3) 
        {
            face_name = best_match;
        }
        else
        {
            face_name = "";
            //printf("No match found with similarity >= 0.5\n");
        }
        
        /*
        float euclidean_distance = 0.0;
        float min_euclidean_distance = 10.0;
        std::string best_match = ""; // 用于存储相似度最高的 name
        for (database_iter = database_face_map.begin(); database_iter != database_face_map.end(); database_iter++) 
        {
            // 计算相似度
            euclidean_distance = euclidean_distance_normalized(database_iter->second.feature, res, 512);
            printf("%s euclidean_distance = %f\n", database_iter->first.c_str(), euclidean_distance); // 使用 c_str() 将 std::string 转为 C 风格字符串

            // 如果当前相似度大于最大相似度，则更新最大相似度和对应的 name
            if (euclidean_distance < min_euclidean_distance) 
            {
                min_euclidean_distance = euclidean_distance;
                best_match = database_iter->first;
            }
        }
        // 遍历完成后，判断最大相似度是否符合条件
        if (min_euclidean_distance <= 2) 
        {
            face_name = best_match;
        }
        else
        {
            //printf("No match found with similarity >= 0.5\n");
        }
      */
        
      }
      using namespace cv;
      Mat orig_img = Mat(video_height, video_width, CV_8UC3, RK_MPI_MB_GetPtr(src_mb));//黑白灰图案
      // 采用opencv来绘制矩形框,颜色格式是B、G、R
      cv::rectangle(orig_img,cv::Point(left, top),cv::Point(right, bottom),cv::Scalar(0,255,255),5,8,0);
      putText(orig_img, detect_result_group.results[i].name, Point(left, top-16), FONT_HERSHEY_TRIPLEX, 3, Scalar(0,0,255),4,8,0);
      putText(orig_img, "state:", cv::Point(20, 1000), FONT_HERSHEY_SIMPLEX, 2, Scalar(0, 255, 0), 6, 8, false);
      putText(orig_img, "name:", cv::Point(20, 1100), FONT_HERSHEY_SIMPLEX, 2, Scalar(0, 255, 0), 6, 8, false);
      putText(orig_img, std::to_string(nap_count), cv::Point(200, 1000), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 0), 6, 8, false);
      putText(orig_img,  face_name, cv::Point(200, 1100), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 0), 6, 8, false);
    }
    
    RK_MPI_SYS_SendMediaBuffer(RK_ID_RGA, 0, src_mb);
    RK_MPI_MB_ReleaseBuffer(src_mb);
    src_mb = NULL;
  }
  if (model) 
   {
     delete model;
     model = NULL;
   }
  rknn_destroy (ctx);
  if (model_1) 
   {
     delete model_1;
     model_1 = NULL;
   }
  rknn_destroy (ctx_1);
  if (model_2) 
   {
     delete model_2;
     model_2 = NULL;
   }
  rknn_destroy (ctx_2);
  printf("face_pthread end!!!\n");
  return NULL;
}
string get_hcb()
{
  return "Hcb";
}
