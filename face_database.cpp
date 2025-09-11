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
    for(int i = 0; i < 512; i++)
    {
      printf("feature[%d] = %f\n",i, feature[i]);
    }
    char * sql = "insert into face_table(name, size, feature) values (?,?,?);";
    
    /*
    将sql文本转化为sqlite可识别的语句（一种字节码）对象，并存入stmt中。即为执行这个sql语句做准备
    参数：
          参数1：数据库操作句柄
          参数2：UTF8编码sql语句
          参数3：sql语句长度，若大于0，则执行该值长度的sql语句，若为-1，则全部执行 
          参数4：准备语句指针(stmt)
          参数5：指向sql语句未使用的语句
    */
    sqlite3_prepare(db, sql, -1, &stmt, NULL);

    /*
    绑定变量值到准备语句中。
    参数：
          参数1：准备语句指针(stmt)
          参数2：要绑定的下标，即第几个？号的索引，对应前面prepare的sql语句里的？
          参数3：绑定的值
    */
    //sqlite3_bind_int(stmt, 0, id); 

    /*
    将文本绑定到sqlite3准备(prepare)语句中，参数：
          [如果SQL没有问题，则绑定插入的数据 (写入)]
          参数1：准备语句指针(stmt)
          参数2：给SQL语句中的第几个？赋值，对应前面prepare的sql语句里的？
          参数3：写入的内容
          参数4：写入数据的长度
          参数5：系统预留的参数
    */
    sqlite3_bind_text(stmt, 1, name, strlen(name), NULL);

    /*
    绑定变量值到准备语句中。
    参数：
          参数1：准备语句指针(stmt)
          参数2：要绑定的下标，即第几个？号的索引，对应前面prepare的sql语句里的？
          参数3：绑定的值
    */
    sqlite3_bind_int(stmt, 2, featureSize); 

    /*
    image存入数据库的类型选择blob类型，sqlite3_bind_blob也是用于绑定参数到准备语句中。
    参数：
          参数1：准备语句指针(stmt)
          参数2：要绑定的BLOB下标，即第几个？号的索引，对应前面prepare的sql语句里的？
          参数3：BLOB数据的指针(即数据起始指针)
          参数4：BLOB数据长度(以字节为单位，如果是二进制类型绝对不可以给-1，必须具体长度)
          参数5：析构回调函数(告诉sqlite当把数据处理完后调用此函数来析够你的数据)，一般默认为空
    */
    sqlite3_bind_blob(stmt, 3, feature, featureSize, NULL);
    
    //执行前面的准备语句
    sqlite3_step(stmt);
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
            //for(int i = 0; i<512;i++)
            //{
            //  printf("feature[%d] = %f\n", i, ((float*)feature)[i]);
            //}
            int blobSize = sqlite3_column_bytes(stmt, 3);
            
            memset(rockx_face_feature.feature, 0, size);
            memcpy(rockx_face_feature.feature, feature, size);
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
