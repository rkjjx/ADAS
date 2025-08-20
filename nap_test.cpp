#include "nap_test.h"
int nap_test(float res_landmk[98][2], float res_angle[3], int *frame_cnt_eye_ptr, int *frame_cnt_ptr)
{
  static int nap_count = 0;
  
  float eye_score_average = 0;
  static float eye_score_sum = 0;
  static float start_eye_score_average = 0;
  static float last_eye_score_average = 0;
  static bool start_test = false;
  
  static int mouth_cnt = 0;
  
  static int start_angle_pitch_score = 0;
  static int last_angle_pitch_score = 1;
  static bool start_test_angle = false;
  
  int frame_cnt_eye = *frame_cnt_eye_ptr;
  int frame_cnt = *frame_cnt_ptr;
  
  float eye_score = ((res_landmk[65][1]+res_landmk[66][1]+res_landmk[67][1]-res_landmk[61][1]-res_landmk[62][1]-res_landmk[63][1])/(float)(3 * (res_landmk[64][0]-res_landmk[60][0])) + (res_landmk[73][1]+res_landmk[74][1]+res_landmk[75][1]-res_landmk[69][1]-res_landmk[70][1]-res_landmk[71][1])/(float)(3 * (res_landmk[72][0]-res_landmk[68][0])))/2;
  float mouth_score = (res_landmk[86][1]+res_landmk[84][1]-res_landmk[78][1]-res_landmk[80][1])/(float)(2 * (res_landmk[82][0]-res_landmk[76][0]));
  eye_score_sum += eye_score;
  
  if(mouth_score > 0.65)
  {
      mouth_cnt += 1;
      printf("mouth_cnt = %d\n", mouth_cnt);
  }
  
  if(start_test_angle && (-res_angle[1]) < start_angle_pitch_score)
  {
    nap_count += 1;
    printf("head blow!\n");
    start_test_angle = false;
  }
  if((-res_angle[1]) - last_angle_pitch_score > 0.25)
  {
      start_angle_pitch_score = (-res_angle[1]);
      start_test_angle = true;
  }
  last_angle_pitch_score = (-res_angle[1]);
  //printf("angle_pitch_score = %f\n", (-res_angle[1]));
  
  if(frame_cnt_eye%10 == 0)
  {
    //printf("eye_score_average = %f\n", eye_score_sum/frame_cnt_eye);
    eye_score_average = eye_score_sum/frame_cnt_eye;
    if(start_test && eye_score_average > start_eye_score_average)
    {
      nap_count += 1;
      printf("eye blink!\n");
      start_test = false;
    }
    if(last_eye_score_average - eye_score_average > 0.02 && last_eye_score_average - eye_score_average < 0.05)
    {
      start_eye_score_average = eye_score_average;
      start_test = true;
    }
    eye_score_sum = *frame_cnt_eye_ptr = 0;
    last_eye_score_average = eye_score_average;
  }
  if(frame_cnt%50 == 0) 
  {
    if(mouth_cnt > 5)
    {
      printf("mouth open!\n");
      nap_count += 1;
    }
    *frame_cnt_ptr = mouth_cnt = 0;
    start_test = false;
    start_test_angle = false;
  }
  return nap_count;
} 
