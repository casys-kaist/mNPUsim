#ifndef __COMMON__
#define __COMMON__

#include <iostream>
#include <string>
#include <fstream>
#include <tuple>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include <queue>
#include <list>
#include <cassert>
#include <cstring>
#include <regex>
#include <random>

#define INTERMEDIATE_DIR string("intermediate/")
#define INTERMEDIATE_CONFIG_DIR string("intermediate_config/")
#define RESULT_DIR string("result/")
#define DRAMREQ_NPU_DIR string("dramreq_npu/")
#define BITS_TO_BYTES 8.0

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

//trace option
#define SRAM_TRACE false
#define DRAMREQ_NPU_TRACE false

//addressing
#define BOUNDARY_IFMAP_OFMAP_ARRAY 1000000

//debug option
#define DEBUG false

using namespace std;

string extract_name(string file_name);
string generate_intermediate_path(string result_path, string file_name, int npu_idx);

void clear_output(string result_path);
void write_output(string result_path, string file_name, string write_str);

#endif
