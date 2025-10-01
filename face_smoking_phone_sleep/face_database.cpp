#include "face_database.h"

sqlite3 *db = NULL;
sqlite3_stmt *stmt;

int open_db()
{
    int rc;
    char *zErrMsg = 0;
    char *sql;
    int select_db=1;
    char *facedb;
    facedb="/demo/bin/rockx_face_table.db3";
    rc = sqlite3_open(facedb, &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    else
    {
        printf("You have successfully opened the database sqlite3!\n");
    }

    if(select_db!=0)
    {
        // Create FACE Table
        sql = "CREATE TABLE IF NOT EXISTS face_table("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
        "name TEXT,"\
        "size INTEGER,"\
        "feature BLOB);";
        
        rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
        if (rc != SQLITE_OK) 
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return -1;
        }
        sqlite3_free(zErrMsg);
    }
 
    return 0;
}


void insert_face_data_to_database(const char *name, int featureSize, float feature[512])
{
    // 打印前几个 feature，避免刷屏
    printf("== 插入前检查: name=%s, featureSize=%d ==\n", name, featureSize);
    for (int i = 0; i < 512; i++) {
        if (i < 5 || i >= 498) {
            printf("feature[%d] = %f\n", i, feature[i]);
        }
    }
    printf("...\n");

    const char *sql = "INSERT INTO face_table(name, size, feature) VALUES (?,?,?);";
    sqlite3_stmt *stmt = NULL;

    // 1. 准备语句
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    // 2. 绑定参数
    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        printf("Bind text failed: %s\n", sqlite3_errmsg(db));
    }

    if (sqlite3_bind_int(stmt, 2, featureSize) != SQLITE_OK) {
        printf("Bind int failed: %s\n", sqlite3_errmsg(db));
    }

    if (sqlite3_bind_blob(stmt, 3, feature, featureSize * sizeof(float), SQLITE_TRANSIENT) != SQLITE_OK) {
        printf("Bind blob failed: %s\n", sqlite3_errmsg(db));
    }

    // 3. 执行语句
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Insert failed: %s\n", sqlite3_errmsg(db));
    } else {
        printf("Insert OK: name=%s\n", name);
    }

    // 4. 清理
    sqlite3_finalize(stmt);
}
// 从数据库读取一条记录，并恢复 feature 向量
int get_face_data_from_database(const char *name, float feature_out[512], int *featureSize_out)
{
    const char *sql = "SELECT size, feature FROM face_table WHERE name = ?;";
    sqlite3_stmt *stmt = NULL;
    int rc;

    // 1. 准备语句
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // 2. 绑定参数
    rc = sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        printf("Bind failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    // 3. 执行查询
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // 读取 size
        int size = sqlite3_column_int(stmt, 0);
        *featureSize_out = size;

        // 读取 blob
        const void *blob = sqlite3_column_blob(stmt, 1);
        int bytes = sqlite3_column_bytes(stmt, 1);

        if (blob && bytes == size * sizeof(float)) {
            memcpy(feature_out, blob, bytes);

            // 打印前几个值验证
            printf("读取成功: size=%d, feature[0]=%f, feature[1]=%f ...\n",
                   size, feature_out[0], feature_out[1]);
        } else {
            printf("Blob 数据不匹配: bytes=%d, size=%d\n", bytes, size);
            sqlite3_finalize(stmt);
            return -1;
        }
    } else {
        printf("没有找到 name=%s 的记录\n", name);
        sqlite3_finalize(stmt);
        return -1;
    }

    // 4. 释放 stmt
    sqlite3_finalize(stmt);
    return 0;
}


map<string, rockx_face_feature_t> FaceFeature()
{
    sqlite3_stmt *stmt;
    char *sql = "select name, size, feature from face_table";
    int ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0);
    //int id;
    char * name;
    int size;

    rockx_face_feature_t rockx_face_feature = {0, 0, 0};
    map<string, rockx_face_feature_t> rockx_face_feature_map;

    if (ret == SQLITE_OK)
    {
        printf("\n###############################################");
        printf("\nFace names saved in the databasen\n");
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            /**
             * int sqlite3_column_int(sqlite3_stmt*, int iCol);
             * 查询(query)结果的筛选，返回当前结果的某1列
             * 参数：
             *      sqlite3_stmt* : 准备结构参数指针
             *      iCol要查询的"列"索引值。注意：sqlite3规定最左侧的“列”索引值是 0，也就是“列”索引号从 0 开始
             */
            /*id = sqlite3_column_int(stmt, 0);
            printf("id = %d\n", id);*/

            name = (char *)sqlite3_column_text(stmt, 0);
            printf("name = %s\n", name);
            size = sqlite3_column_int(stmt, 1);
            //printf("size = %d\n", size);
            const void *feature = sqlite3_column_blob(stmt, 2);
            
            memset(rockx_face_feature.feature, 0, size*sizeof(float));
            memcpy(rockx_face_feature.feature, feature, size*sizeof(float));
            rockx_face_feature.len = size;
            string str(name);
            rockx_face_feature_map.insert(pair<string, rockx_face_feature_t>(str, rockx_face_feature));
        }
        printf("###############################################\n\n");
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db); 
    
    return rockx_face_feature_map;
}
