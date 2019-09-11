#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <concurrent_queue.h>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <mysql.h>
#include <sstream>
#include <thread>
#include <vector>

#define SLICE_SIZE 80

using namespace ::std;
using namespace ::concurrency;

mutex mtx_full;
condition_variable cv_full;

mutex mtx_empty;
condition_variable cv_empty;

mutex mtx_used;
condition_variable cv_used;

mutex mtx_start;
condition_variable cv_start;

mutex mtx;

concurrent_queue<int> next_slice;
concurrent_queue<int> used_slice;

bool start_run;
bool stopped;

static unsigned long long total_trx;

static vector<unsigned long long> commit_trx;

static vector<unsigned long long> rollback_trx;

static unsigned long long all_committed_trx;
static unsigned long long all_rollback_trx;

static vector<chrono::time_point<chrono::high_resolution_clock>> end_time_point;

void update_func(int table_id, int table_size, int slot_thread_id,
                 int update_nums, int thread_num) {

  MYSQL conn;

  string table_name = "sbtest";

  mysql_init(&conn);
  this_thread::sleep_for(chrono::milliseconds(400));
  if (!mysql_real_connect(&conn, "localhost", "root", "", "sbtest", 3306, NULL,
                          CLIENT_FOUND_ROWS)) {
    mtx.lock();
    cout << "cannot connect to mysql server in thead id: "
         << this_thread::get_id() << ", err: " << mysql_error(&conn) << endl;
    mtx.unlock();
    mysql_close(&conn);
    return;
  }

  int row_start;
  int row_id;
  int slice_id;
  int slices = table_size / SLICE_SIZE;
  bool expect_val = false;
  bool release_expect_val = true;
  string pre_query = "update " + table_name + to_string(table_id);
  pre_query += " set c='";
  string query;
  string pStmt = "' where id=";
  /*char *stmt = new char[256];
  char *query = new char[256];*/
  // snprintf(stmt, 256, "UPDATE sbtest%d SET c='%%d' WHERE id=%%d", table_id);
  stringstream thread_id;
  thread_id << this_thread::get_id();
  srand(stoi(thread_id.str()));

  // wait barrier
  while (!start_run) {
    unique_lock<mutex> lck(mtx_start);
    cv_start.wait(lck);
    lck.unlock();
  }

  // transaction
  mysql_autocommit(&conn, false);
  while (!stopped) {
    // pick up a free-completion slice region from next_slice
    while (!next_slice.try_pop(slice_id)) {
      // cv_full.notify_all();
      unique_lock<mutex> lck(mtx_empty);
      cv_empty.wait(lck);
      lck.unlock();
    }
    if (next_slice.unsafe_size() <= thread_num / 2) {
      cv_full.notify_all();
    }

    ////for multi-thread debug
    //   mtx.lock();
    //   cout << "slice id: " << slice_id << endl;
    //   mtx.unlock();

    row_start = slice_id * SLICE_SIZE + 17;

    mysql_query(&conn, "BEGIN");
    for (int i = 0; i < update_nums; i++) {
      row_id = rand() % (SLICE_SIZE - 34) + row_start;
      query =
          pre_query + to_string(rand() % 100000) + pStmt + to_string(row_id);
      // snprintf(query, 256, stmt, rand(), row_id);
      if (mysql_query(&conn, query.c_str())) {
        mysql_query(&conn, "ROLLBACK");
        rollback_trx[slot_thread_id]++;
        /*mtx.lock();
        cout << "thread: " << this_thread::get_id() << ", error in table "
             << table_id << " on row " << row_id
             << " err: " << mysql_error(&conn) << endl;
        mtx.unlock();*/
        goto Continue;
      }
    }
    mysql_query(&conn, "COMMIT");
    // increment the number of committed trasaction
    commit_trx[slot_thread_id]++;
  Continue:
    used_slice.push(slice_id);
    cv_used.notify_all();
  }
  end_time_point[slot_thread_id] = chrono::high_resolution_clock::now();
  mysql_autocommit(&conn, true);
  mysql_close(&conn);
  /*delete[] stmt;
  delete[] query;*/
  /*mtx.lock();
  cout << "update completed in thread " << slot_thread_id << " and exit"
       << endl;
  mtx.unlock();*/
}

void gen_slice_id(int table_size, int thread_num) {
  int slice_num = table_size / SLICE_SIZE;
  deque<int> shuffle_slice(slice_num);
  // init
  for (int i = 0; i < slice_num; i++) {
    shuffle_slice[i] = i;
  }
  // generate non-overlapped slice id
  int idx;
  int val;
  stringstream str;
  str << this_thread::get_id();
  srand((unsigned int)stoi(str.str()));
  while (!stopped) {
    // wait when next_slice is up to full
    while (next_slice.unsafe_size() >= thread_num * 4) {
      // cv_empty.notify_all();
      unique_lock<mutex> lck(mtx_full);
      cv_full.wait(lck);
      lck.unlock();
    }
    // retrieve all items from used_slice
    while (shuffle_slice.empty() && used_slice.empty()) {
      unique_lock<mutex> lck(mtx_used);
      cv_used.wait(lck);
      lck.unlock();
    }
    if (shuffle_slice.size() <= slice_num / 2 && !used_slice.empty()) {
      while (used_slice.try_pop(val)) {
        shuffle_slice.push_front(val);
      }
    }
    // generate next slice id and send it to next_slice
    idx = rand() % shuffle_slice.size();
    swap(shuffle_slice[idx], shuffle_slice[shuffle_slice.size() - 1]);
    next_slice.push(shuffle_slice.back());
    shuffle_slice.pop_back();
    // notify all trans thread when next_slice has more than half of threads
    if (next_slice.unsafe_size() >= thread_num / 2) {
      cv_empty.notify_all();
    }
  }
}

void sysbench(int table_id, int thread_num, int duration, int table_size,
	int batch_size) {
  vector<thread> trx_threads;
  stopped = false;
  start_run = false;
  total_trx = 0;
  all_committed_trx = 0;
  all_rollback_trx = 0;
  commit_trx.resize(thread_num, 0);
  rollback_trx.resize(thread_num, 0);
  end_time_point.resize(thread_num);
  for (int i = 0; i < thread_num; i++) {
    commit_trx[i] = 0;
    rollback_trx[i] = 0;
  }

  for (int i = 0; i < thread_num; i++) {
    trx_threads.push_back(
        thread(update_func, table_id, table_size, i, batch_size, thread_num));
  }
  thread slice_id_produce(gen_slice_id, table_size, thread_num);

  // start oltp test
  this_thread::sleep_for(chrono::seconds(3));
  start_run = true;
  cv_start.notify_all();

  // processing for 5 seconds
  chrono::time_point<chrono::high_resolution_clock> start =
      chrono::high_resolution_clock::now();
  this_thread::sleep_for(chrono::seconds(duration));
  stopped = true;
  for (auto &th : trx_threads) {
    th.join();
  }
  next_slice.clear();
  used_slice.push(1);
  cv_full.notify_all();
  cv_used.notify_all();
  slice_id_produce.join();

  chrono::time_point<chrono::high_resolution_clock> end;
  for (int i = 0; i < end_time_point.size(); i++) {
    if (i == 0) {
      end = end_time_point[0];
    }
    end = (end < end_time_point[i]) ? end_time_point[i] : end;
  }

  chrono::duration<double, milli> real_elapsed =
      chrono::duration_cast<chrono::milliseconds>(end - start);

  for (int i = 0; i < thread_num; i++) {
    all_committed_trx += commit_trx[i];
    all_rollback_trx += rollback_trx[i];
  }

  total_trx = all_committed_trx + all_rollback_trx;

  cout << "------------------------------------------------------------------"
       << endl;

  cout << "thread number: " << thread_num << endl;

  cout << "table size: " << table_size << endl;

  cout << "batch size: " << batch_size << endl;

  cout << "consuming time: " << real_elapsed.count() / 1000 << "sec" << endl;

  cout << "commit trans: " << all_committed_trx << endl;

  cout << "rollback trans: " << all_rollback_trx << endl;

  cout << "total trans: " << total_trx << endl;

  cout << "TPS: " << 1000 * all_committed_trx / real_elapsed.count() << "/sec"
       << endl;

  cout << "-------------------------------------------------------------------"
       << endl;
}

int main() {

  vector<thread> trx_threads;

  int thread_num = 1;
  int max_thead = 40;
  int table_size = 10000;
  int batch_size = 4;
  int duration = 30;
  int table_id = 1;

  cout << "table size: ";
  cin >> table_size;
  cout << "table id: ";
  cin >> table_id;
  cout << "init thread number: ";
  cin >> thread_num;
  cout << "max thread number: ";
  cin >> max_thead;

  cout << "start oltp test..." << endl;

  for (; thread_num < max_thead; thread_num++) {
    sysbench(table_id, thread_num, duration, table_size, batch_size);
    this_thread::sleep_for(chrono::milliseconds(1000));
  }

  return 0;
}