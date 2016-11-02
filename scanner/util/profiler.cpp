/* Copyright 2016 Carnegie Mellon University, NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "scanner/util/profiler.h"
#include "scanner/util/storehouse.h"

#include <cmath>
#include <map>
#include <string>

namespace scanner {

Profiler::Profiler(timepoint_t base_time) : base_time_(base_time), lock_(0) {}

Profiler::Profiler(const Profiler& other)
    : base_time_(other.base_time_), records_(other.records_), lock_(0) {}

Profiler::~Profiler(void) {}

const std::vector<Profiler::TaskRecord>& Profiler::get_records() const {
  return records_;
}

void write_profiler_to_file(storehouse::WriteFile* file, int64_t node,
                            std::string type_name, std::string tag,
                            int64_t worker_num, const Profiler& profiler) {
  // Write worker header information
  // Node
  write(file, node);
  // Worker type
  write(file, type_name);
  // Worker tag
  write(file, tag);
  // Worker number
  write(file, worker_num);
  // Intervals
  const std::vector<scanner::Profiler::TaskRecord>& records =
      profiler.get_records();
  // Perform dictionary compression on interval key names
  uint8_t record_key_id = 0;
  std::map<std::string, uint8_t> key_names;
  for (size_t j = 0; j < records.size(); j++) {
    const std::string& key = records[j].key;
    if (key_names.count(key) == 0) {
      key_names.insert({key, record_key_id++});
    }
  }
  if (key_names.size() > std::pow(2, sizeof(record_key_id) * 8)) {
    fprintf(stderr,
            "WARNING: Number of record keys (%lu) greater than "
            "max key id (%lu). Recorded intervals will alias in "
            "profiler file.\n",
            key_names.size(), std::pow(2, sizeof(record_key_id) * 8));
  }
  // Write out key name dictionary
  int64_t num_keys = static_cast<int64_t>(key_names.size());
  write(file, num_keys);
  for (auto& kv : key_names) {
    std::string key = kv.first;
    uint8_t key_index = kv.second;
    write(file, key);
    write(file, key_index);
  }
  // Number of intervals
  int64_t num_records = static_cast<int64_t>(records.size());
  write(file, num_records);
  for (size_t j = 0; j < records.size(); j++) {
    const scanner::Profiler::TaskRecord& record = records[j];
    uint8_t key_index = key_names[record.key];
    int64_t start = record.start;
    int64_t end = record.end;
    write(file, key_index);
    write(file, start);
    write(file, end);
  }
}
}
