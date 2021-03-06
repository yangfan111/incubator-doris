// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <time.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/common.h>

#include "olap/field.h"
#include "olap/column_predicate.h"
#include "olap/in_list_predicate.h"
#include "runtime/mem_pool.h"
#include "runtime/string_value.hpp"
#include "runtime/vectorized_row_batch.h"
#include "util/logging.h"

namespace palo {

namespace datetime {

static uint24_t timestamp_from_date(const char* date_string) {
    tm time_tm;
    strptime(date_string, "%Y-%m-%d", &time_tm);

    int value = (time_tm.tm_year + 1900) * 16 * 32
        + (time_tm.tm_mon + 1) * 32
        + time_tm.tm_mday;
    return uint24_t(value); 
}

static uint64_t timestamp_from_datetime(const std::string& value_string) {
    tm time_tm;
    strptime(value_string.c_str(), "%Y-%m-%d %H:%M:%S", &time_tm);

    uint64_t value = ((time_tm.tm_year + 1900) * 10000L
            + (time_tm.tm_mon + 1) * 100L
            + time_tm.tm_mday) * 1000000L
        + time_tm.tm_hour * 10000L
        + time_tm.tm_min * 100L
        + time_tm.tm_sec;

    return value;
}

static std::string to_date_string(uint24_t& date_value) {
    tm time_tm;
    int value = date_value; 
    memset(&time_tm, 0, sizeof(time_tm));
    time_tm.tm_mday = static_cast<int>(value & 31);
    time_tm.tm_mon = static_cast<int>(value >> 5 & 15) - 1;
    time_tm.tm_year = static_cast<int>(value >> 9) - 1900;
    char buf[20] = {'\0'};
    strftime(buf, sizeof(buf), "%Y-%m-%d", &time_tm);
    return std::string(buf);
}
    
static std::string to_datetime_string(uint64_t& datetime_value) {
    tm time_tm;
    int64_t part1 = (datetime_value / 1000000L);
    int64_t part2 = (datetime_value - part1 * 1000000L);

    time_tm.tm_year = static_cast<int>((part1 / 10000L) % 10000) - 1900;
    time_tm.tm_mon = static_cast<int>((part1 / 100) % 100) - 1;
    time_tm.tm_mday = static_cast<int>(part1 % 100);

    time_tm.tm_hour = static_cast<int>((part2 / 10000L) % 10000);
    time_tm.tm_min = static_cast<int>((part2 / 100) % 100);
    time_tm.tm_sec = static_cast<int>(part2 % 100);

    char buf[20] = {'\0'};
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &time_tm);
    return std::string(buf);
}

};

class TestInListPredicate : public testing::Test {
public:
    TestInListPredicate() : _vectorized_batch(NULL) {
        _mem_tracker.reset(new MemTracker(-1));
        _mem_pool.reset(new MemPool(_mem_tracker.get()));
    }

    ~TestInListPredicate() {
        if (_vectorized_batch != NULL) {
            delete _vectorized_batch;
        }
    }

    void SetFieldInfo(FieldInfo &field_info, std::string name,
            FieldType type, FieldAggregationMethod aggregation,
            uint32_t length, bool is_allow_null, bool is_key) {
        field_info.name = name;
        field_info.type = type;
        field_info.aggregation = aggregation;
        field_info.length = length;
        field_info.is_allow_null = is_allow_null;
        field_info.is_key = is_key;
        field_info.precision = 1000;
        field_info.frac = 10000;
        field_info.unique_id = 0;
        field_info.is_bf_column = false;
    }

    void InitVectorizedBatch(const std::vector<FieldInfo>& schema,
                             const std::vector<uint32_t>& ids, 
                             int size) {
        _vectorized_batch = new VectorizedRowBatch(schema, ids, size);
        _vectorized_batch->set_size(size);
    }
    std::unique_ptr<MemTracker> _mem_tracker;
    std::unique_ptr<MemPool> _mem_pool;
    VectorizedRowBatch* _vectorized_batch;
};

#define TEST_IN_LIST_PREDICATE(TYPE, TYPE_NAME, FIELD_TYPE) \
TEST_F(TestInListPredicate, TYPE_NAME##_COLUMN) { \
    std::vector<FieldInfo> schema; \
    FieldInfo field_info; \
    SetFieldInfo(field_info, std::string("TYPE_NAME##_COLUMN"), FIELD_TYPE, \
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true); \
    schema.push_back(field_info); \
    int size = 10; \
    std::vector<uint32_t> return_columns; \
    for (int i = 0; i < schema.size(); ++i) { \
        return_columns.push_back(i); \
    } \
    InitVectorizedBatch(schema, return_columns, size); \
    ColumnVector* col_vector = _vectorized_batch->column(0); \
     \
    /* for no nulls */ \
    col_vector->set_no_nulls(true); \
    TYPE* col_data = reinterpret_cast<TYPE*>(_mem_pool->allocate(size * sizeof(TYPE))); \
    col_vector->set_col_data(col_data); \
    for (int i = 0; i < size; ++i) { \
        *(col_data + i) = i; \
    } \
    \
    std::set<TYPE> values; \
    values.insert(4); \
    values.insert(5); \
    values.insert(6); \
    ColumnPredicate* pred = new InListPredicate<TYPE>(0, std::move(values)); \
    pred->evaluate(_vectorized_batch); \
    ASSERT_EQ(_vectorized_batch->size(), 3); \
    uint16_t* sel = _vectorized_batch->selected(); \
    ASSERT_EQ(*(col_data + sel[0]), 4); \
    ASSERT_EQ(*(col_data + sel[1]), 5); \
    ASSERT_EQ(*(col_data + sel[2]), 6); \
    \
    /* for has nulls */ \
    col_vector->set_no_nulls(false); \
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size)); \
    memset(is_null, 0, size); \
    col_vector->set_is_null(is_null); \
    for (int i = 0; i < size; ++i) { \
        if (i % 2 == 0) { \
            is_null[i] = true; \
        } else { \
            *(col_data + i) = i; \
        } \
    } \
    _vectorized_batch->set_size(size); \
    _vectorized_batch->set_selected_in_use(false); \
    pred->evaluate(_vectorized_batch); \
    ASSERT_EQ(_vectorized_batch->size(), 1); \
    sel = _vectorized_batch->selected(); \
    ASSERT_EQ(*(col_data + sel[0]), 5); \
} \

TEST_IN_LIST_PREDICATE(int8_t, TINYINT, OLAP_FIELD_TYPE_TINYINT)
TEST_IN_LIST_PREDICATE(int16_t, SMALLINT, OLAP_FIELD_TYPE_SMALLINT)
TEST_IN_LIST_PREDICATE(int32_t, INT, OLAP_FIELD_TYPE_INT)
TEST_IN_LIST_PREDICATE(int64_t, BIGINT, OLAP_FIELD_TYPE_BIGINT)
TEST_IN_LIST_PREDICATE(int128_t, LARGEINT, OLAP_FIELD_TYPE_LARGEINT)

TEST_F(TestInListPredicate, FLOAT_COLUMN) {
    std::vector<FieldInfo> schema;
    FieldInfo field_info;
    SetFieldInfo(field_info, std::string("FLOAT_COLUMN"), OLAP_FIELD_TYPE_FLOAT,
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true);
    schema.push_back(field_info);
    int size = 10;
    std::vector<uint32_t> return_columns;
    for (int i = 0; i < schema.size(); ++i) {
        return_columns.push_back(i);
    }
    InitVectorizedBatch(schema, return_columns, size);
    ColumnVector* col_vector = _vectorized_batch->column(0);

    // for no nulls
    col_vector->set_no_nulls(true);
    float* col_data = reinterpret_cast<float*>(_mem_pool->allocate(size * sizeof(float)));
    col_vector->set_col_data(col_data);
    for (int i = 0; i < size; ++i) {
        *(col_data + i) = i + 0.1;
    }
    std::set<float> values;
    values.insert(4.1);
    values.insert(5.1);
    values.insert(6.1);
    ColumnPredicate* pred = new InListPredicate<float>(0, std::move(values));
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 3);
    uint16_t* sel = _vectorized_batch->selected();
    ASSERT_FLOAT_EQ(*(col_data + sel[0]), 4.1);
    ASSERT_FLOAT_EQ(*(col_data + sel[1]), 5.1);
    ASSERT_FLOAT_EQ(*(col_data + sel[2]), 6.1);

    // for has nulls
    col_vector->set_no_nulls(false);
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size));  
    memset(is_null, 0, size);
    col_vector->set_is_null(is_null);
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            is_null[i] = true;
        } else {
            *(col_data + i) = i + 0.1;
        }
    } 
    _vectorized_batch->set_size(size);
    _vectorized_batch->set_selected_in_use(false);
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 1);
    sel = _vectorized_batch->selected();
    ASSERT_FLOAT_EQ(*(col_data + sel[0]), 5.1);
}

TEST_F(TestInListPredicate, DOUBLE_COLUMN) {
    std::vector<FieldInfo> schema;
    FieldInfo field_info;
    SetFieldInfo(field_info, std::string("DOUBLE_COLUMN"), OLAP_FIELD_TYPE_DOUBLE,
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true);
    schema.push_back(field_info);
    int size = 10;
    std::vector<uint32_t> return_columns;
    for (int i = 0; i < schema.size(); ++i) {
        return_columns.push_back(i);
    }
    InitVectorizedBatch(schema, return_columns, size);
    ColumnVector* col_vector = _vectorized_batch->column(0);

    // for no nulls
    col_vector->set_no_nulls(true);
    double* col_data = reinterpret_cast<double*>(_mem_pool->allocate(size * sizeof(double)));
    col_vector->set_col_data(col_data);
    for (int i = 0; i < size; ++i) {
        *(col_data + i) = i + 0.1;
    }
    std::set<double> values;
    values.insert(4.1);
    values.insert(5.1);
    values.insert(6.1); 

    ColumnPredicate* pred = new InListPredicate<double>(0, std::move(values));
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 3);
    uint16_t* sel = _vectorized_batch->selected();
    ASSERT_DOUBLE_EQ(*(col_data + sel[0]), 4.1);
    ASSERT_DOUBLE_EQ(*(col_data + sel[1]), 5.1);
    ASSERT_DOUBLE_EQ(*(col_data + sel[2]), 6.1);

    // for has nulls
    col_vector->set_no_nulls(false);
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size));  
    memset(is_null, 0, size);
    col_vector->set_is_null(is_null);
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            is_null[i] = true;
        } else {
            *(col_data + i) = i + 0.1;
        }
    }
    _vectorized_batch->set_size(size);
    _vectorized_batch->set_selected_in_use(false);
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 1);
    sel = _vectorized_batch->selected();
    ASSERT_DOUBLE_EQ(*(col_data + sel[0]), 5.1);
}

TEST_F(TestInListPredicate, DECIMAL_COLUMN) {
    std::vector<FieldInfo> schema;
    FieldInfo field_info;
    SetFieldInfo(field_info, std::string("DECIMAL_COLUMN"), OLAP_FIELD_TYPE_DECIMAL,
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true);
    schema.push_back(field_info);
    int size = 10;
    std::vector<uint32_t> return_columns;
    for (int i = 0; i < schema.size(); ++i) {
        return_columns.push_back(i);
    }
    InitVectorizedBatch(schema, return_columns, size);
    ColumnVector* col_vector = _vectorized_batch->column(0);

    // for no nulls
    col_vector->set_no_nulls(true);
    decimal12_t* col_data = reinterpret_cast<decimal12_t*>(_mem_pool->allocate(size * sizeof(decimal12_t)));
    col_vector->set_col_data(col_data);
    for (int i = 0; i < size; ++i) {
        (*(col_data + i)).integer = i;
        (*(col_data + i)).fraction = i;
    }

    std::set<decimal12_t> values;
    decimal12_t value1(4, 4);
    values.insert(value1);

    decimal12_t value2(5, 5);
    values.insert(value2);

    decimal12_t value3(6, 6);
    values.insert(value3);

    ColumnPredicate* pred = new InListPredicate<decimal12_t>(0, std::move(values));
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 3);
    uint16_t* sel = _vectorized_batch->selected();
    ASSERT_EQ(*(col_data + sel[0]), value1);
    ASSERT_EQ(*(col_data + sel[1]), value2);
    ASSERT_EQ(*(col_data + sel[2]), value3);

    // for has nulls
    col_vector->set_no_nulls(false);
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size));  
    memset(is_null, 0, size);
    col_vector->set_is_null(is_null);
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            is_null[i] = true;
        } else {
            (*(col_data + i)).integer = i;
            (*(col_data + i)).fraction = i;
        }
    }

    _vectorized_batch->set_size(size);
    _vectorized_batch->set_selected_in_use(false);
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 1);
    sel = _vectorized_batch->selected();
    ASSERT_EQ(*(col_data + sel[0]), value2);
}

TEST_F(TestInListPredicate, CHAR_COLUMN) {
    std::vector<FieldInfo> schema;
    FieldInfo field_info;
    SetFieldInfo(field_info, std::string("STRING_COLUMN"), OLAP_FIELD_TYPE_CHAR,
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true);
    schema.push_back(field_info);
    int size = 10;
    std::vector<uint32_t> return_columns;
    for (int i = 0; i < schema.size(); ++i) {
        return_columns.push_back(i);
    }
    InitVectorizedBatch(schema, return_columns, size);
    ColumnVector* col_vector = _vectorized_batch->column(0);

    // for no nulls
    col_vector->set_no_nulls(true);
    StringValue* col_data = reinterpret_cast<StringValue*>(_mem_pool->allocate(size * sizeof(StringValue)));
    col_vector->set_col_data(col_data);
    
    char* string_buffer = reinterpret_cast<char*>(_mem_pool->allocate(50));
    memset(string_buffer, 0, 50);
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j <= 5; ++j) {
            string_buffer[j] = 'a' + i;
        }
        (*(col_data + i)).len = 5;
        (*(col_data + i)).ptr = string_buffer;
        string_buffer += 5;
    }

    std::set<StringValue> values; 
    StringValue value1;
    const char* value1_buffer = "aaaaa";
    value1.ptr = const_cast<char*>(value1_buffer);
    value1.len =  5;
    values.insert(value1);

    StringValue value2;
    const char* value2_buffer = "bbbbb";
    value2.ptr = const_cast<char*>(value2_buffer);
    value2.len = 5;
    values.insert(value2);

    StringValue value3;
    const char* value3_buffer = "ccccc";
    value3.ptr = const_cast<char*>(value3_buffer);
    value3.len = 5;
    values.insert(value3);

    ColumnPredicate* pred = new InListPredicate<StringValue>(0, std::move(values));
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 3);
    uint16_t* sel = _vectorized_batch->selected();
    ASSERT_EQ(*(col_data + sel[0]), value1);
    ASSERT_EQ(*(col_data + sel[1]), value2);
    ASSERT_EQ(*(col_data + sel[2]), value3);

    // for has nulls
    col_vector->set_no_nulls(false);
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size));  
    memset(is_null, 0, size);
    col_vector->set_is_null(is_null);
    string_buffer = reinterpret_cast<char*>(_mem_pool->allocate(50));
    memset(string_buffer, 0, 50);
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            is_null[i] = true;
        } else {
            for (int j = 0; j <= 5; ++j) {
                string_buffer[j] = 'a' + i;
            }
            (*(col_data + i)).len = 5;
            (*(col_data + i)).ptr = string_buffer;
        }
        string_buffer += 5;
    }

    _vectorized_batch->set_size(size);
    _vectorized_batch->set_selected_in_use(false);
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 1);
    sel = _vectorized_batch->selected();
    ASSERT_EQ(*(col_data + sel[0]), value2);
}

TEST_F(TestInListPredicate, VARCHAR_COLUMN) {
    std::vector<FieldInfo> schema;
    FieldInfo field_info;
    SetFieldInfo(field_info, std::string("STRING_COLUMN"), OLAP_FIELD_TYPE_VARCHAR,
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true);
    schema.push_back(field_info);
    int size = 10;
    std::vector<uint32_t> return_columns;
    for (int i = 0; i < schema.size(); ++i) {
        return_columns.push_back(i);
    }
    InitVectorizedBatch(schema, return_columns, size);
    ColumnVector* col_vector = _vectorized_batch->column(0);

    // for no nulls
    col_vector->set_no_nulls(true);
    StringValue* col_data = reinterpret_cast<StringValue*>(_mem_pool->allocate(size * sizeof(StringValue)));
    col_vector->set_col_data(col_data);
    
    char* string_buffer = reinterpret_cast<char*>(_mem_pool->allocate(55));
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j <= i; ++j) {
            string_buffer[j] = 'a' + i;
        }
        (*(col_data + i)).len = i + 1;
        (*(col_data + i)).ptr = string_buffer;
        string_buffer += i + 1;
    }

    std::set<StringValue> values; 
    StringValue value1;
    const char* value1_buffer = "a";
    value1.ptr = const_cast<char*>(value1_buffer);
    value1.len =  1;
    values.insert(value1);

    StringValue value2;
    const char* value2_buffer = "bb";
    value2.ptr = const_cast<char*>(value2_buffer);
    value2.len = 2;
    values.insert(value2);

    StringValue value3;
    const char* value3_buffer = "ccc";
    value3.ptr = const_cast<char*>(value3_buffer);
    value3.len = 3;
    values.insert(value3);

    ColumnPredicate* pred = new InListPredicate<StringValue>(0, std::move(values));
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 3);
    uint16_t* sel = _vectorized_batch->selected();
    ASSERT_EQ(*(col_data + sel[0]), value1);
    ASSERT_EQ(*(col_data + sel[1]), value2);
    ASSERT_EQ(*(col_data + sel[2]), value3);

    // for has nulls
    col_vector->set_no_nulls(false);
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size));  
    memset(is_null, 0, size);
    col_vector->set_is_null(is_null);
    string_buffer = reinterpret_cast<char*>(_mem_pool->allocate(55));
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            is_null[i] = true;
        } else {
            for (int j = 0; j <= i; ++j) {
                string_buffer[j] = 'a' + i;
            }
            (*(col_data + i)).len = i + 1;
            (*(col_data + i)).ptr = string_buffer;
        }
        string_buffer += i + 1;
    }

    _vectorized_batch->set_size(size);
    _vectorized_batch->set_selected_in_use(false);
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 1);
    sel = _vectorized_batch->selected();
    ASSERT_EQ(*(col_data + sel[0]), value2);
}

TEST_F(TestInListPredicate, DATE_COLUMN) {
    std::vector<FieldInfo> schema;
    FieldInfo field_info;
    SetFieldInfo(field_info, std::string("DATE_COLUMN"), OLAP_FIELD_TYPE_DATE,
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true);
    schema.push_back(field_info);
    int size = 6;
    std::vector<uint32_t> return_columns;
    for (int i = 0; i < schema.size(); ++i) {
        return_columns.push_back(i);
    }
    InitVectorizedBatch(schema, return_columns, size);
    ColumnVector* col_vector = _vectorized_batch->column(0);

    // for no nulls
    col_vector->set_no_nulls(true);
    uint24_t* col_data = reinterpret_cast<uint24_t*>(_mem_pool->allocate(size * sizeof(uint24_t)));
    col_vector->set_col_data(col_data);
    
    std::vector<std::string> date_array;
    date_array.push_back("2017-09-07");
    date_array.push_back("2017-09-08");
    date_array.push_back("2017-09-09");
    date_array.push_back("2017-09-10");
    date_array.push_back("2017-09-11");
    date_array.push_back("2017-09-12");
    for (int i = 0; i < size; ++i) {
        uint24_t timestamp = datetime::timestamp_from_date(date_array[i].c_str());
        *(col_data + i) = timestamp;
    }

    std::set<uint24_t> values;
    uint24_t value1 = datetime::timestamp_from_date("2017-09-09");
    values.insert(value1);

    uint24_t value2 = datetime::timestamp_from_date("2017-09-10");
    values.insert(value2);

    uint24_t value3 = datetime::timestamp_from_date("2017-09-11");
    values.insert(value3);

    ColumnPredicate* pred = new InListPredicate<uint24_t>(0, std::move(values));
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 3);
    uint16_t* sel = _vectorized_batch->selected();
    ASSERT_EQ(datetime::to_date_string(*(col_data + sel[0])), "2017-09-09");
    ASSERT_EQ(datetime::to_date_string(*(col_data + sel[1])), "2017-09-10");
    ASSERT_EQ(datetime::to_date_string(*(col_data + sel[2])), "2017-09-11");

    // for has nulls
    col_vector->set_no_nulls(false);
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size));  
    memset(is_null, 0, size);
    col_vector->set_is_null(is_null);
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            is_null[i] = true;
        } else {
            uint24_t timestamp = datetime::timestamp_from_date(date_array[i].c_str());
            *(col_data + i) = timestamp;
        }
    }

    _vectorized_batch->set_size(size);
    _vectorized_batch->set_selected_in_use(false);
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 1);
    sel = _vectorized_batch->selected();
    ASSERT_EQ(datetime::to_date_string(*(col_data + sel[0])), "2017-09-10");
}

TEST_F(TestInListPredicate, DATETIME_COLUMN) {
    std::vector<FieldInfo> schema;
    FieldInfo field_info;
    SetFieldInfo(field_info, std::string("DATETIME_COLUMN"), OLAP_FIELD_TYPE_DATETIME,
                 OLAP_FIELD_AGGREGATION_REPLACE, 1, false, true);
    schema.push_back(field_info);
    int size = 6;
    std::vector<uint32_t> return_columns;
    for (int i = 0; i < schema.size(); ++i) {
        return_columns.push_back(i);
    }
    InitVectorizedBatch(schema, return_columns, size);
    ColumnVector* col_vector = _vectorized_batch->column(0);

    // for no nulls
    col_vector->set_no_nulls(true);
    uint64_t* col_data = reinterpret_cast<uint64_t*>(_mem_pool->allocate(size * sizeof(uint64_t)));
    col_vector->set_col_data(col_data);
    
    std::vector<std::string> date_array;
    date_array.push_back("2017-09-07 00:00:00");
    date_array.push_back("2017-09-08 00:01:00");
    date_array.push_back("2017-09-09 00:00:01");
    date_array.push_back("2017-09-10 01:00:00");
    date_array.push_back("2017-09-11 01:01:00");
    date_array.push_back("2017-09-12 01:01:01");
    for (int i = 0; i < size; ++i) {
        uint64_t timestamp = datetime::timestamp_from_datetime(date_array[i].c_str());
        *(col_data + i) = timestamp;
    }

    std::set<uint64_t> values;
    uint64_t value1 = datetime::timestamp_from_datetime("2017-09-09 00:00:01");
    values.insert(value1);

    uint64_t value2 = datetime::timestamp_from_datetime("2017-09-10 01:00:00");
    values.insert(value2);

    uint64_t value3 = datetime::timestamp_from_datetime("2017-09-11 01:01:00");
    values.insert(value3);

    ColumnPredicate* pred = new InListPredicate<uint64_t>(0, std::move(values));
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 3);
    uint16_t* sel = _vectorized_batch->selected();
    ASSERT_EQ(datetime::to_datetime_string(*(col_data + sel[0])), "2017-09-09 00:00:01");
    ASSERT_EQ(datetime::to_datetime_string(*(col_data + sel[1])), "2017-09-10 01:00:00");
    ASSERT_EQ(datetime::to_datetime_string(*(col_data + sel[2])), "2017-09-11 01:01:00");

    // for has nulls
    col_vector->set_no_nulls(false);
    bool* is_null = reinterpret_cast<bool*>(_mem_pool->allocate(size));  
    memset(is_null, 0, size);
    col_vector->set_is_null(is_null);
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            is_null[i] = true;
        } else {
            uint64_t timestamp = datetime::timestamp_from_datetime(date_array[i].c_str());
            *(col_data + i) = timestamp;
        }
    }

    _vectorized_batch->set_size(size);
    _vectorized_batch->set_selected_in_use(false);
    pred->evaluate(_vectorized_batch);
    ASSERT_EQ(_vectorized_batch->size(), 1);
    sel = _vectorized_batch->selected();
    ASSERT_EQ(datetime::to_datetime_string(*(col_data + sel[0])), "2017-09-10 01:00:00");
}

} // namespace palo

int main(int argc, char** argv) {
    std::string conffile = std::string(getenv("PALO_HOME")) + "/conf/be.conf";
    if (!palo::config::init(conffile.c_str(), false)) {
        fprintf(stderr, "error read config file. \n");
        return -1;
    }
    palo::init_glog("be-test");
    int ret = palo::OLAP_SUCCESS;
    testing::InitGoogleTest(&argc, argv);
    palo::CpuInfo::init();
    ret = RUN_ALL_TESTS();
    google::protobuf::ShutdownProtobufLibrary();
    return ret;
}
