#pragma once
#include <cstdio>
#include <cstdlib>
#include <httplib.h>
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>

namespace order_system {

static MYSQL* MySQLInit() {
    // 1.创建数据库句柄
    MYSQL* mysql = mysql_init(NULL);
    // 2.建立句柄和数据库间的联系
    if (mysql_real_connect(mysql, "127.0.0.1", "root", "order_system", "789",  3306, NULL, 0) == NULL) {
        printf("mysql connection failed\n %s", mysql_error(mysql));
        return NULL;
    }
    // 3.设置编码格式 
    mysql_set_character_set(mysql, "utf8");
    return mysql;
}
static void MySQLRelease(MYSQL* mysql) {
    mysql_close(mysql);
}

// 使用 DishTable 实现对菜品的数据操作
class DishTable {
    DishTable(MYSQL* mysql)
        : _mysql(mysql)
    {}

    ~DishTable() 
    {   MySQLRelease(_mysql);  }

    bool Insert(const Json::Value& dish) {
        char sql[1024] = {0};
        sprintf(sql, "insert into dish_table values(null, %s, %d)", dish["name"].asCString(), dish["price"].asInt());
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("insert error \n  %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }
    }

    bool SelectAll(Json::Value* dishes) {
        char sql[1024 * 4] = {0};
        sprintf(sql, "select dish_id, name, price from dish_table");
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("selectall error \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }

        // 获取结果
        MYSQL_RES* result = mysql_store_result(_mysql);
        if (result == NULL) {
            printf("获取结果失败 \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }

        int rows = mysql_num_rows(result);
        for (int i = 0; i < rows; ++i) {
            MYSQL_ROW row = mysql_fetch_row(result);
            Json::Value dish;
            dish["dish_id"] = atoi(row[0]);
            dish["name"] = row[1];
            dish["price"] = row[2];
            dishes->append(dish);
        }
        return true;
    }
    
    bool SelectOne(int32_t dish_id, Json::Value* dish) {
        char sql[1024 * 4] = {0};
        sprintf(sql, "select dish_id, name, price from dish_table where dish_id = %d", dish_id);
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("selectone error \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }

        // 获取结果
        MYSQL_RES* result = mysql_store_result(_mysql);
        if (result == NULL) {
            printf("获取结果失败 \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }

        int rows = mysql_num_rows(result);
        if (rows != 1) {
            printf("查找结果不等于一条， rows = %d\n", rows);
            mysql_close(_mysql);
            return false;
        }
        for (int i = 0; i < rows; ++i) {
            MYSQL_ROW row = mysql_fetch_row(result);
            (*dish)["dish_id"] = atoi(row[0]);
            (*dish)["name"] = row[1];
            (*dish)["price"] = atoi(row[2]);
            break;
        }
        return true;
    }
    
    bool Update(const Json::Value dish) {
        char sql[1024 * 4] = {0};
        sprintf(sql, "update dish_table set name = %s, price = %d where dish_id = %d", 
                dish["name"].asCString(), dish["price"].asInt(), dish["dish_id"].asInt());
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("update error \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }
        return true;
    }
    
    bool Delete(int dish_id) {
        char sql[1024 * 4] = {0};
        sprintf(sql, "delete from dish_table where dish_id = %d", dish_id);
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("delete error \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }
        return true;
    }
private:
    MYSQL* _mysql;
};  // DishTable 类

// 使用 OrderTable 实现对订单的数据操作
class OrderTable {
    OrderTable(MYSQL* mysql)
        : _mysql(mysql)
    {}

    ~OrderTable() 
    {   MySQLRelease(_mysql);  }
    bool Insert(const Json::Value& order) {
        char sql[1024] = {0};
        sprintf(sql, "insert into order_table values(null, %s, %s, %s, %d)", 
                order["table_id"].asCString(), order["time"].asCString(), order["orderes"].asCString(),order["status"].asInt() );
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("insert error \n  %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }
    }

    bool SelectAll(Json::Value* dishes) {
        char sql[1024 * 4] = {0};
        sprintf(sql, "select order_id, table_id, time, dishes, status from dish_table");
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("selectall error \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }

        // 获取结果
        MYSQL_RES* result = mysql_store_result(_mysql);
        if (result == NULL) {
            printf("获取结果失败 \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }

        int rows = mysql_num_rows(result);
        for (int i = 0; i < rows; ++i) {
            MYSQL_ROW row = mysql_fetch_row(result);
            Json::Value order;
            order["order_id"] = atoi(row[0]);
            order["table_id"] = row[1];
            order["time"] = row[2];
            order["dishes"] = row[3];
            order["status"] = row[4];
            dishes->append(order);
        }
        return true;
    }
    
    bool ChangeStatus(const Json::Value& order) {
        char sql[1024 * 4] = {0};
        sprintf(sql, "update order_table set status = %d where order_id = %d",
                order["status"].asInt(), order["order_id"].asInt());
        // 执行 sql 语句
        int ret = mysql_query(_mysql, sql);
        if (ret != 0) {
            printf("update error \n %s", mysql_error(_mysql));
            mysql_close(_mysql);
            return false;
        }
        return true;
    }
    
private:
    MYSQL* _mysql;
};  // OrderTable 类

}   // end of order_system
