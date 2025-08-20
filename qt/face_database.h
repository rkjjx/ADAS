#ifndef _FACE_DATABASE_H
#define _FACE_DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

#include "rockx.h"
#include "sqlite3.h"

using namespace std;

int open_db();
//void insert_face_data_to_database(const char *name, int featureSize, float feature[512]);
map<string, rockx_face_feature_t> FaceFeature();

#endif
