#include <Windows.h>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <mysql.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <time.h>

using namespace std;

static mutex mtx;

int main() {
  MYSQL *conn;

  // random
  srand(static_cast<unsigned int>(time(NULL)));

  string table_name = "sbtest";
  string engine;
  int table_size = 10000;
  int tables_count = 1;
  int table_id = 1;

  cout << "engine: ";
  cin >> engine;
  cout << "table size: ";
  cin >> table_size;
  cout << "table id: ";
  cin >> table_id;

  conn = mysql_init(0);
  conn = mysql_real_connect(conn, "localhost", "root", "", "sbtest", 3306, NULL,
                            0);

  if (conn) {
    cout << "connection is ok" << endl;

    string query;
    char *c_query = new char[512];

    for (int i = 0; i < tables_count; i++) {
      // drop tables
      query = "DROP TABLE IF EXISTS " + table_name + to_string(i + table_id);
      if (mysql_query(conn, query.c_str())) {
        cout << "drop table " << i + table_id << " error" << endl;
        mysql_close(conn);
        delete[] c_query;
        exit(-1);
      }
      // create table
      query = "create table " + table_name + to_string(i + table_id);
      query += "(id int not null, k int default 0 not null, c char(120) "
               "default \"\" not null, pad char(60) default \"\" not null, "
               "primary key(id), key kid(k))engine=" +
               engine;
      if (mysql_query(conn, query.c_str())) {
        cout << "create table " << i + table_id << "error" << endl;
        mysql_close(conn);
        delete[] c_query;
        exit(-1);
      }
      // insert data
      for (int row_id = 1; row_id <= table_size; row_id++) {
        query = "insert into %s (id, k, c, pad) values (%d, %d, "
                "'##############', '##############')";
        snprintf(c_query, 512, query.c_str(),
                 (table_name + to_string(i + table_id)).c_str(), row_id,
                 rand() % table_size + 1);
        if (mysql_query(conn, c_query)) {
          cout << "insert table " << i + table_id << "error"
               << "in row " << row_id << endl;
          mysql_close(conn);
          delete[] c_query;
          exit(-1);
        }
      }
    }
    // close connection
    delete[] c_query;
    mysql_close(conn);
  } else {
    cout << "connection is failed" << endl;
  }

  return 0;
}