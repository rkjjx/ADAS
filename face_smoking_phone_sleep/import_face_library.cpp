#include "import_face_library.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
int main(int argc, char *argv[]) 
{
  const char *name = argv[1];
  const char *image_path = argv[2]; 
  
 
  if (argc != 3) 
  {
    printf("Usage: ./import_face_library  id face_name face_path\n");
    return -1;
  }

  open_db();

  int width, height, channels;
  unsigned char *img = stbi_load(image_path, &width, &height, &channels, 0);
  if (img == NULL) {
    printf("Failed to load image\n");
    return 1;
  }
  int target_width = 112;
  int target_height = 112;
  int target_channels = 3; // RGB

  unsigned char *resized_img = (unsigned char *)malloc(target_width * target_height * target_channels);
  if (!resized_img) {
    printf("Failed to allocate memory for resized image\n");
    stbi_image_free(img);
    return 1;
  }
  printf("end of reading image.\n");
  stbir_resize_uint8(img, width, height, 0, resized_img, target_width, target_height, 0, target_channels);
  printf("end of resizing image.\n");
  run_face_recognize(name, resized_img);

  return 0;
}
static void printRKNNTensor(rknn_tensor_attr *attr)
{
  printf("index=%d name=%s n_dims=%d dims=[%d %d %d %d] n_elems=%d size=%d "
         "fmt=%d type=%d qnt_type=%d fl=%d zp=%d scale=%f\n",
         attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2],
         attr->dims[1], attr->dims[0], attr->n_elems, attr->size, 0, attr->type,
         attr->qnt_type, attr->fl, attr->zp, attr->scale);
}

rockx_object_t *get_max_face(rockx_object_array_t *face_array) 
{
  int i;
  rockx_object_t *max_face = NULL;

  if (face_array->count == 0) 
  {
    return NULL;
  }
  
  for (i = 0; i < face_array->count; i++) 
  {
    rockx_object_t *cur_face = &(face_array->object[i]);
    if (max_face == NULL) 
    {
      max_face = cur_face;
      continue;
    }
    int cur_face_box_area = (cur_face->box.right - cur_face->box.left) *
                            (cur_face->box.bottom - cur_face->box.top);
    int max_face_box_area = (max_face->box.right - max_face->box.left) *
                            (max_face->box.bottom - max_face->box.top);
    if (cur_face_box_area > max_face_box_area) 
    {
      max_face = cur_face;
    }
  }
  printf("get_max_face %d\n", i);
  return max_face;
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

int run_face_recognize(const char *name, unsigned char *in_image) 
{
  rknn_context ctx;
  int ret;
  unsigned char *model;
  // Load RKNN Model
  int model_len = 0;
  static char *model_path = "/demo/bin/mobilefacenet.rknn";
  printf("Loading model ...\n");            
  model = load_model(model_path, &model_len);
  ret = rknn_init(&ctx, model, model_len, 0);
  if (ret < 0)
  {
    printf("rknn_init fail! ret=%d\n", ret);
    return -1;
  }
  // Get Model Input Output Info
  rknn_input_output_num io_num;
  ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
  if (ret != RKNN_SUCC)
  {
    printf("rknn_query fail! ret=%d\n", ret);
    return -1;
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
      return -1;
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
      return -1;
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

  
  rknn_input inputs[1];
  memset(inputs, 0, sizeof(inputs));
  inputs[0].index = 0;
  inputs[0].type = RKNN_TENSOR_UINT8;
  inputs[0].size = width * height * channel;
  inputs[0].fmt = RKNN_TENSOR_NHWC;
  inputs[0].buf = in_image;
  ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
  if (ret < 0)
  {
    printf("ERROR: rknn_inputs_set fail! ret=%d\n", ret);
    return -1;
  }
  free(in_image);
  // Run
  printf("rknn_run\n");
  ret = rknn_run(ctx, nullptr);
  if (ret < 0)
  {
    printf("ERROR: rknn_run fail! ret=%d\n", ret);
    return -1;
  }

  // Get Output
  printf("get_output\n");
  rknn_output outputs[io_num.n_output];
  memset(outputs, 0, sizeof(outputs));
  for (int i = 0; i < io_num.n_output; i++)
  {
    outputs[i].want_float = 1;
  }
  ret = rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
  if (ret < 0)
  {
    printf("ERROR: rknn_outputs_get fail! ret=%d\n", ret);
    return -1;
  }
  printf("outputs[0].size = %d\n", outputs[0].size);
  /*
  uint8_t *ptr = (uint8_t *)outputs[0].buf;
  float res[512];
  for(int i = 0; i < 512; i++)
  {
    res[i] = ((float)(*ptr++) - 124)*0.020613;
  }
  */
  rknn_outputs_release(ctx, io_num.n_output, outputs);
  printf("FEATURE_SIZE=%d\n",FEATURE_SIZE);
  insert_face_data_to_database(name, FEATURE_SIZE, (float*)outputs[0].buf);
  rknn_outputs_release(ctx, 1, outputs);
  if (model) 
  {
    delete model;
    model = NULL;
  }
  rknn_destroy (ctx); 
  /*
  float feature_t[512];
  int featureSize_t;

  if (get_face_data_from_database("hcb", feature_t, &featureSize_t) == 0) {
    printf("从数据库恢复的向量长度=%d, 第一个值=%f\n", featureSize_t, feature_t[0]);
  }
  */
  return 0;
}

