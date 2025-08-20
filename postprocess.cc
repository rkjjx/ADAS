#include "postprocess.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include "./include/postprocess.h"
#include <stdint.h>
#define LABEL_NALE_TXT_PATH "/demo/bin/drive_3_labels_list.txt"
static char *labels[OBJ_CLASS_NUM];

const int anchor0[6] = {10, 13, 16, 30, 33, 23};
const int anchor1[6] = {30, 61, 62, 45, 59, 119};
const int anchor2[6] = {116, 90, 156, 198, 373, 326};

inline static int clamp(float val, int min, int max)
{
    return val > min ? (val < max ? val : max) : min;
}

char *readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = (char *)malloc(buff_len + 1);
    if (!buffer)
        return NULL; // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF)
    {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = (char *)tmp;

        buffer[i] = (char)ch;
        i++;
    }
    buffer[i] = '\0';

    *len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

int readLines(const char *fileName, char *lines[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s;
    int i = 0;
    int n = 0;
    while ((s = readLine(file, s, &n)) != NULL)
    {
        lines[i++] = s;
        if (i >= max_line)
            break;
    }
    return i;
}

int loadLabelName(const char *locationFilename, char *label[])
{
    printf("loadLabelName %s\n", locationFilename);
    readLines(locationFilename, label, OBJ_CLASS_NUM);
    return 0;
}

static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1)
{
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> &order, float threshold)
{
    for (int i = 0; i < validCount; ++i)
    {
        if (order[i] == -1)
        {
            continue;
        }
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j)
        {
            int m = order[j];
            if (m == -1)
            {
                continue;
            }
            float xmin0 = outputLocations[n * 4 + 0];
            float ymin0 = outputLocations[n * 4 + 1];
            float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
            float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

            float xmin1 = outputLocations[m * 4 + 0];
            float ymin1 = outputLocations[m * 4 + 1];
            float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
            float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold)
            {
                order[j] = -1;
            }
        }
    }
    return 0;
}

static int quick_sort_indice_inverse(
    std::vector<float> &input,
    int left,
    int right,
    std::vector<int> &indices)
{
    float key;
    int key_index;
    int low = left;
    int high = right;
    if (left < right)
    {
        key_index = indices[left];
        key = input[left];
        while (low < high)
        {
            while (low < high && input[high] <= key)
            {
                high--;
            }
            input[low] = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key)
            {
                low++;
            }
            input[high] = input[low];
            indices[high] = indices[low];
        }
        input[low] = key;
        indices[low] = key_index;
        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }
    return low;
}
static int filterClass2Targets(std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId)
{
    // 记录classId为2的索引
    std::vector<int> class2Indices;

    // 遍历classId，找到所有classId为2的索引
    for (int i = 0; i < classId.size(); ++i)
    {
        if (classId[i] == 2)
        {
            class2Indices.push_back(i);
        }
    }

    // 如果没有找到classId为2的目标，直接返回0
    if (class2Indices.empty()) return 0;

    // 找到boxScores中得分最高的classId为2的索引
    int maxScoreIndex = class2Indices[0];
    float maxScore = boxScores[class2Indices[0]];

    for (int i = 1; i < class2Indices.size(); ++i)
    {
        int idx = class2Indices[i];
        if (boxScores[idx] > maxScore)
        {
            maxScore = boxScores[idx];
            maxScoreIndex = idx;
        }
    }

    // 记录删除的classId数量
    int deleteCount = 0;

    // 反向遍历class2Indices，删除所有不是maxScoreIndex的目标
    for (int i = class2Indices.size() - 1; i >= 0; --i)
    {
        int idx = class2Indices[i];
        if (idx != maxScoreIndex)
        {
            // 删除boxes（每个box有4个float）
            boxes.erase(boxes.begin() + idx * 4, boxes.begin() + (idx + 1) * 4);

            // 删除对应的boxScores和classId
            boxScores.erase(boxScores.begin() + idx);
            classId.erase(classId.begin() + idx);

            // 统计删除的classId数量
            deleteCount++;
        }
    }

    // 返回删除的classId数量
    return deleteCount;
}


static float sigmoid(float x)
{
    return 1.0 / (1.0 + expf(-x));
}

static float unsigmoid(float y)
{
    return -1.0 * logf((1.0 / y) - 1.0);
}

inline static int32_t __clip(float val, float min, float max)
{
    float f = val <= min ? min : (val >= max ? max : val);
    return f;
}

static uint8_t qnt_f32_to_affine(float f32, uint8_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    uint8_t res = (uint8_t)__clip(dst_val, 0, 255);
    return res;
}

static float deqnt_affine_to_f32(uint8_t qnt, uint8_t zp, float scale)
{
    return ((float)qnt - (float)zp) * scale;
}


static int process(uint8_t *input, int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                   std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                   float threshold, uint8_t zp, float scale)
{
    int validCount = 0;
    float thres = unsigmoid(threshold);
    uint8_t thres_u8 = qnt_f32_to_affine(thres, zp, scale);

    // 遍历三个不同的输入通道
    for (int a = 0; a < 3; a++)
    {
        for (int i = 0; i < grid_h; i++)
        {
            for (int j = 0; j < grid_w; j++)
            {
                int offset = (a * grid_h * grid_w + i * grid_w + j) * PROP_BOX_SIZE;
                uint8_t *in_ptr = input + offset;

                uint8_t box_confidence = in_ptr[4]; // 第 4 个值为置信度
                if (box_confidence >= thres_u8)
                {
                    float box_x = sigmoid(deqnt_affine_to_f32(in_ptr[0], zp, scale)) * 2.0 - 0.5;
                    float box_y = sigmoid(deqnt_affine_to_f32(in_ptr[1], zp, scale)) * 2.0 - 0.5;
                    float box_w = sigmoid(deqnt_affine_to_f32(in_ptr[2], zp, scale)) * 2.0;
                    float box_h = sigmoid(deqnt_affine_to_f32(in_ptr[3], zp, scale)) * 2.0;

                    box_x = (box_x + j) * (float)stride;
                    box_y = (box_y + i) * (float)stride;
                    box_w = box_w * box_w * (float)anchor[a * 2];
                    box_h = box_h * box_h * (float)anchor[a * 2 + 1];
                    box_x -= (box_w / 2.0);
                    box_y -= (box_h / 2.0);

                    boxes.push_back(box_x);
                    boxes.push_back(box_y);
                    boxes.push_back(box_w);
                    boxes.push_back(box_h);
                    //printf("start\n");
                    float box_conf_f32 = sigmoid(deqnt_affine_to_f32(box_confidence, zp, scale));
                    //printf("box_conf_f32 = %f\t", box_conf_f32);
                    boxScores.push_back(box_conf_f32);

                    // 找到最大的类别概率
                    uint8_t maxClassProbs = in_ptr[5];
                    
                    //printf("in_ptr[5] = %u\t", in_ptr[5]);
                    int maxClassId = 0;
                    for (int k = 1; k < OBJ_CLASS_NUM; ++k)
                    {
                        uint8_t prob = in_ptr[5 + k];
                        //printf("in_ptr[6] = %u\n", in_ptr[5 + k]);
                        if (prob > maxClassProbs)
                        {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }
                    //printf("end\n");
                    classId.push_back(maxClassId);
                    validCount++;
                }
            }
        }
    }
    return validCount;
}

int post_process(uint8_t *input0, uint8_t *input1, uint8_t *input2, int model_in_h, int model_in_w,
                 float conf_threshold, float nms_threshold, float vis_threshold, float scale_w, float scale_h,
                 std::vector<uint8_t> &qnt_zps, std::vector<float> &qnt_scales,
                 detect_result_group_t *group)
{
    static int init = -1;
    if (init == -1)
    {
        int ret = 0;
        ret = loadLabelName(LABEL_NALE_TXT_PATH, labels);
        if (ret < 0)
        {
            return -1;
        }

        init = 0;
    }
    memset(group, 0, sizeof(detect_result_group_t));

    std::vector<float> filterBoxes;
    std::vector<float> boxesScore;
    std::vector<int> classId;
    int stride0 = 8;
    int grid_h0 = model_in_h / stride0;
    int grid_w0 = model_in_w / stride0;
    int validCount0 = 0;
    validCount0 = process(input0, (int *)anchor0, grid_h0, grid_w0, model_in_h, model_in_w,
                          stride0, filterBoxes, boxesScore, classId, conf_threshold, qnt_zps[0], qnt_scales[0]);

    int stride1 = 16;
    int grid_h1 = model_in_h / stride1;
    int grid_w1 = model_in_w / stride1;
    int validCount1 = 0;
    validCount1 = process(input1, (int *)anchor1, grid_h1, grid_w1, model_in_h, model_in_w,
                          stride1, filterBoxes, boxesScore, classId, conf_threshold, qnt_zps[1], qnt_scales[1]);

    int stride2 = 32;
    int grid_h2 = model_in_h / stride2;
    int grid_w2 = model_in_w / stride2;
    int validCount2 = 0;
    validCount2 = process(input2, (int *)anchor2, grid_h2, grid_w2, model_in_h, model_in_w,
                          stride2, filterBoxes, boxesScore, classId, conf_threshold, qnt_zps[2], qnt_scales[2]);

    int validCount = validCount0 + validCount1 + validCount2;
    // no object detect
    if (validCount <= 0)
    {
        return 0;
    }
    int deleteCount = filterClass2Targets(filterBoxes, boxesScore, classId);
    validCount -= deleteCount;
    std::vector<int> indexArray;
    for (int i = 0; i < validCount; ++i)
    {
        indexArray.push_back(i);
    }

    quick_sort_indice_inverse(boxesScore, 0, validCount - 1, indexArray);

    nms(validCount, filterBoxes, indexArray, nms_threshold);

    int last_count = 0;
    int k = 1;
    group->count = 0;
    /* box valid detect target */
    for (int i = 0; i < validCount; ++i)
    {
        if (indexArray[i] == -1 || boxesScore[i] < vis_threshold || i >= OBJ_NUMB_MAX_SIZE)
        {
            continue;
        }
        int n = indexArray[i];
        
        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];
        group->results[last_count].box.left = (int)(clamp(x1, 0, model_in_w) / scale_w);
        group->results[last_count].box.top = (int)(clamp(y1, 0, model_in_h) / scale_h);
        group->results[last_count].box.right = (int)(clamp(x2, 0, model_in_w) / scale_w);
        group->results[last_count].box.bottom = (int)(clamp(y2, 0, model_in_h) / scale_h);
        group->results[last_count].prop = boxesScore[n];
        char *label = labels[id];
        strncpy(group->results[last_count].name, label, OBJ_NAME_MAX_SIZE);

        // printf("result %2d: (%4d, %4d, %4d, %4d), %s\n", i, group->results[last_count].box.left, group->results[last_count].box.top,
        //        group->results[last_count].box.right, group->results[last_count].box.bottom, label);
        last_count++;
    }
    group->count = last_count;

    return 0;
}
int get_result_hcb(float *input, float (*res)[98][2], float *input_1, float (*res_1)[3])
{
    for(int i = 0; i < 98; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            (*res)[i][j] = *input;
            input++;
        }
    }
    for(int i = 0; i < 3; i++)
    {
        (*res_1)[i] = *input_1;
        input_1++;
    }
    return 0;
}

