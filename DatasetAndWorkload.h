//
// Created by kliment on 2/26/22.
//

#ifndef SURF_DATASETANDWORKLOAD_H
#define SURF_DATASETANDWORKLOAD_H

#include <string>
#include <vector>
#include "PointQuery.h"
#include <iostream>
#include <set>
#include <map>
#include <algorithm>
#include "fstream"
#include "Trie.h"

using namespace std;

class RangeFilterTemplate;
class MemorizedPointQuery;

vector<string> read_dataset(const string& file_path);

class Dataset: public vector<string>
{
private:
    mutable const vector<string>* prev_dataset  = nullptr;
    mutable vector<string>::const_iterator prev_iter;
    static int reads_from_cache;
    static int total_reads;
    static int cache_read_attempts;

    const MemorizedPointQuery *ground_truth_point_query = nullptr;

public:

    Dataset() = default;
    explicit Dataset(const vector<string>& _dataset): vector<string>(_dataset) {}

    inline bool cached_contains(
            const vector<string>& dataset,
            const string& left, const string& right,
            bool& has_ret,
            vector<string>::const_iterator& lb,
            vector<string>::const_iterator& hb,
            vector<string>::const_iterator& at) const {
        if(prev_dataset != nullptr) {
            cache_read_attempts ++;
            at = prev_iter;
            if (at == dataset.end()) {
                if (left > *dataset.rbegin()) {
                    has_ret = true;
                    return false;
                } else {
//                    assert(left <= *dataset.rbegin());
                    assert(at > dataset.begin());
                    at--;
                    if (right >= *dataset.rbegin()) {
                        has_ret = true;
                        return true;
                    } else {
                        hb = dataset.end()-1;
                    }
                }
            } else {
                int iter = 0;
                while (iter < 3) {
                    iter++;
                    if (left < *at) {
                        if (*at > right) {
                            if (at > dataset.begin()) {
                                at--;
                                if (left > *at) {
                                    has_ret = true;
                                    at++;
                                    return false;
                                } else if (left == *at) {
                                    has_ret = true;
                                    return true;
                                } else {
                                    hb = at + 1;
                                }
                            } else {
                                assert(at == dataset.begin());
                                has_ret = true;
                                return false;
                            }
                        }
                        else {
                            has_ret = true;
                            return true;
                        }
                    } else if (*at < left) {
                        lb = at+1;
                        at++;
                        if(at == dataset.end()) {
                            break;
                        }
                    }
                    else {
                        has_ret = true;
                        return true;
                    }
                }
            }
        }
        has_ret = false;
        return true;
    }

    bool contains(const string& left, const string& right) const
    {
        const vector<string>& dataset = *this;
        if(dataset.empty()) {
            return false;
        }

        auto lb = dataset.begin();
        auto hb = dataset.end();
        vector<string>::const_iterator at;

        bool has_ret = false;
        bool ret = cached_contains(dataset, left, right, has_ret, lb, hb, at);
//        bool ret = true;

        total_reads+=1;

        if(has_ret) {
            reads_from_cache += 1;

            prev_dataset = &dataset;
            prev_iter = at;

            return ret;
        }

        auto new_at = lower_bound(lb, hb, left);
        prev_dataset = &dataset;
        prev_iter = new_at;
//        cout << *prev_iter << endl;

        if(new_at == dataset.end()) {
            if(has_ret) {
                assert(!ret);
            }
            return false;
        }

//        assert(new_at != hb);
        if(has_ret) {
            assert(*new_at == *at);
            assert(new_at == at);
        }
        else {
            at = new_at;
        }


        if(at == dataset.end())
        {
            if(has_ret) {
                assert(!ret);
            }
            return false;
        }
        assert(*at >= left);
        if(right < *at)
        {
            if(has_ret) {
                assert(!ret);
            }
            return false;
        }

        if(has_ret) {
            assert(ret);
        }
        return true;
    }

    void remove_duplicates();

    int get_num_inserts_in_point_query(size_type cutoff) const;

    void set_ground_truth_point_query(const MemorizedPointQuery *_ground_truth_point_query);

    size_t get_max_length();
};

class DatasetAndWorkloadMetaData
{

protected:

    string to_binary_string(size_t x, size_t n) const
    {
        assert(_base == 2);
        string ret;
        while(x > 0) {
            ret += std::to_string(x % _base);
            x/=_base;
        }
        while(ret.size() < n)
        {
            ret += "0";
        }
        assert(ret.size() == n);
        reverse(ret.begin(), ret.end());
        return ret;
    }


    string to_binary_string(const string& str)
    {
        string new_str;
        for(auto c: str)
        {
            assert(!char_to_binary_string[(size_t)(unsigned char)c].empty());
            new_str+=char_to_binary_string[(size_t)(unsigned char)c];
        }
        return new_str;
    }


    string char_to_binary_string[CHAR_SIZE];
    char char_to_new_char[CHAR_SIZE];

    size_t max_length = 0;
    size_t max_length_of_key = 0;
    char min_char = (char)127;
    char max_char = (char)0;

    int max_id = -1;
    int count_num_char[CHAR_SIZE];

    map<char, size_t> char_to_id;
    vector<char> id_to_char;
    int num_bits = -1;
    size_t _base = 2;
    const size_t final_base = 64;
    int num_bits_per_char = -1;

public:
    const DatasetAndWorkloadMetaData& original_meta_data;
    const string _workload_difficulty;
    const int _impossible_depth;

//    DatasetAndWorkloadMetaData(): original_meta_data(*this) {
//        memset(count_num_char, 0, sizeof(count_num_char));
//    }

    DatasetAndWorkloadMetaData(const string& __workload_difficulty, const int __impossible_depth): original_meta_data(*this),
            _workload_difficulty(__workload_difficulty),
            _impossible_depth(__impossible_depth){}

    DatasetAndWorkloadMetaData(const DatasetAndWorkloadMetaData& is_original, bool assert_true): original_meta_data(is_original),
        _workload_difficulty(is_original._workload_difficulty),
        _impossible_depth(is_original._impossible_depth) {
        assert(assert_true);
        max_length = is_original.max_length;
        max_length_of_key = is_original.max_length_of_key;
        min_char = is_original.min_char;
        max_char = is_original.max_char;
        memcpy(count_num_char, is_original.count_num_char, sizeof(count_num_char));
    }

    string binary_string_to_string_in_base(const string& str) const
    {
        assert(num_bits != -1);
        string new_str;
        int num = 0;
        int p = 1<<(num_bits-1);
        int bit_id = 0;
//        cout << "str: " << str << endl;
        for(size_t i = 0;i<str.size();i++)
        {
            char c = str[i];
            assert(p>=1);
            if(c == '1')
            {
                num+=p;
            }
            else
            {
                assert(c == '0');
            }

//            cout << c <<" " << num << endl;
            p/=2;
            bit_id++;
            if(bit_id == num_bits)
            {
//                cout << " = " <<num << endl;
                assert(0 <= num && num <= 127);
                new_str+=(char)num;
                p = 1<<(num_bits-1);
                num = 0;
                bit_id = 0;
            }
//            new_str += to_binary_string(char_to_id[c], num_bits_per_char);
        }
        if(bit_id >= 1) {
//            cout << " = " << num << " last" << endl;
            new_str += (char) num;
        }
        return new_str;
    }

    string original_string_to_string_in_base(const string& str) const
    {
        string ret;
        for(size_t i = 0;i<str.size();i++) {
//            assert(char_to_id.find(str[i]) != char_to_id.end());
//            ret += (char)char_to_id.at(str[i]);
            ret += char_to_new_char[(int)str[i]];
        }
        return ret;
    }

    void process_str(const string& str)
    {
        assert(!str.empty());
        max_length = max(max_length, str.size());
        for(size_t i = 0; i<str.size();i++) {
            char c = str[i];
            min_char = min(min_char, c);
            max_char = max(max_char, c);
            count_num_char[(size_t)(unsigned char)c]+=1;
        }
    }

    static int binary_str_to_int(string binary_num)
    {
        int ret = 0;
        int p = 1;
        for(int i = (int)binary_num.size()-1; i>=0;i--)
        {
            int bit = 0;
            if(binary_num[i] == '1')
            {
                bit = 1;
            }
            ret += bit*p;
            p*=2;
        }
        cout << binary_num <<" == " << ret << endl;
        return ret;
    }

public:
    string translate_back(const string& to_translate) const {
        if(num_bits_per_char == -1) {
            return to_translate;
        }
        assert(num_bits_per_char == 6);
        string ret;
        string binary_num;
        for(auto id : to_translate) {
            assert(id != 0);
            assert((int)id < (int)id_to_char.size());
            ret += id_to_char[id];
        }
        return ret;
    }

};

class DatasetAndWorkloadSeed
{
    Dataset& dataset;
    Dataset workload_seed;
    char init_char = (char)127;

public:

    void prep_dataset_and_workload_seed(const string& file_path, const string& workload_difficulty, int impossible_depth = -1);

    DatasetAndWorkloadSeed(Dataset& _dataset, const string& file_path, const string& workload_difficulty): dataset(_dataset) {
        prep_dataset_and_workload_seed(file_path, workload_difficulty);
    }

    DatasetAndWorkloadSeed(Dataset& _dataset, const Dataset& _workload_seed): dataset(_dataset), workload_seed(_workload_seed)
    { }


    const Dataset &get_workload_seed() const {
        return workload_seed;
    }

    char get_init_char() const {
        return init_char;
    }

    const Dataset &get_dataset() const ;

    vector<DatasetAndWorkloadSeed> split_workload_seeds(int num_splits);

    void clear();
};

typedef vector<pair<string, string> > Workload;

class DatasetAndWorkload: public DatasetAndWorkloadMetaData
{
    Dataset dataset;
    Workload workload;

    RangeFilterTemplate* ground_truth_range_filter = nullptr;

    bool negative_workload_defined = false;
    Workload negative_workload;
    void translate_binary_to_to_base()
    {
        assert(_base <= 128);

        num_bits = 0;
        int tmp_base = _base;
        while(tmp_base >= 2)
        {
            assert(tmp_base%2 == 0);
            tmp_base/=2;
            num_bits+=1;
        }

        cout << "base " << _base << " num_bits " << num_bits << endl;

        for(size_t i = 0;i<dataset.size();i++)
        {
            dataset[i] = binary_string_to_string_in_base(dataset[i]);
        }

        for(size_t i = 0;i<workload.size();i++)
        {
            workload[i] = make_pair(
                    binary_string_to_string_in_base(workload[i].first),
                    binary_string_to_string_in_base(workload[i].second));
        }
    }

    void translate_to_base() {
        assert(id_to_char.empty());
        assert(char_to_id.empty());

        id_to_char.push_back(0);

        set<char> unique_chars = get_unique_chars();

        size_t id = 1;
        size_t prev = 0;
        for (auto c: unique_chars) {
            assert(c != 0);
            assert(prev <= char_to_id[c]);
            prev = char_to_id[c];
            assert(id_to_char.size() == id);
            id_to_char.push_back(c);
            char_to_id[c] = id++;
        }
        num_bits_per_char = 1;
        max_id = id - 1;
        int at = 1;
        while (at <= max_id) {
            at *= _base;
            num_bits_per_char++;
        }
        num_bits_per_char--;

//        cout << "max_id " << max_id << " " << num_bits_per_char << endl;

        for (auto c: unique_chars) {
            char_to_new_char[(int) c] = (char) char_to_id[c];
        }

        for (size_t i = 0; i < dataset.size(); i++) {
            dataset[i] = original_string_to_string_in_base(dataset[i]);
        }

        for (size_t i = 0; i < workload.size(); i++) {
            workload[i] = make_pair(
                    original_string_to_string_in_base(workload[i].first),
                    original_string_to_string_in_base(workload[i].second));
        }
    }

    set<char> get_unique_chars() {
        set<char> ret;
        for (int i = 0; i < CHAR_SIZE; i++) {
            if (count_num_char[i] != 0) {
                assert(count_num_char[i] >= 1);
                ret.insert((char) i);
            }
        }
        return ret;
    }

    void translate_to_binary()
    {
        assert(_base == 2);
        assert(id_to_char.empty());
        assert(char_to_id.empty());

        id_to_char.push_back(0);

        set<char> unique_chars = get_unique_chars();

        size_t id = 1;
        size_t prev = 0;
        for(auto c : unique_chars)
        {
            assert(c != 0);
            assert(prev <= char_to_id[c]);
            prev = char_to_id[c];
            assert(id_to_char.size() == id);
            id_to_char.push_back(c);
            char_to_id[c] = id++;
        }
        num_bits_per_char = 1;
        max_id = id-1;
        int at = 1;
        while(at <= max_id)
        {
            at*=_base;
            num_bits_per_char++;
        }
        num_bits_per_char--;

        cout << "max_id " << max_id <<" "<< num_bits_per_char << endl;

        for(auto c: unique_chars)
        {
            char_to_binary_string[(int)c] = to_binary_string(char_to_id[c], num_bits_per_char);
        }

        for(size_t i = 0;i<dataset.size();i++)
        {
            dataset[i] = to_binary_string(dataset[i]);
        }

        for(size_t i = 0;i<workload.size();i++)
        {
            workload[i] = make_pair(
                    to_binary_string(workload[i].first),
                    to_binary_string(workload[i].second));
        }
    }

    void process()
    {
        max_length = 0;
        min_char = (char) 127;
        max_char = (char) 0;
        memset(count_num_char, 0, sizeof(count_num_char));
        negative_workload_defined = false;
        negative_workload.clear();

        for(const auto& it: dataset) {
            process_str(it);
            max_length_of_key = max(max_length_of_key, it.size());
        }
        for(const auto& it: workload) {
            process_str(it.first);
            process_str(it.second);
        }

    }
public:

    void clear();

    void print(ostream& out) const
    {
        out << "dataset" << endl;
        for(const auto& it: dataset) {
            out << it << endl;
        }

        out << "workload" << endl;
        for(const auto& it: workload) {
            out << it.first <<" "<< it.second << endl;
        }
    }

//    DatasetAndWorkload(const DatasetAndWorkloadSeed& dataset_and_workload_seed, const string& workload_difficulty, bool do_translate_to_base)
//    {
//        dataset = dataset_and_workload_seed.get_dataset();
//
//        prep_dataset_and_workload(dataset_and_workload_seed, workload_difficulty);
//
//        process();
//
//        if(do_translate_to_base) {
//            translate_to_base();
//            process();
//        }
//    }

    DatasetAndWorkload(const string& file_path, const string& __workload_difficulty, const int __impossible_depth, bool do_translate_to_base):
            DatasetAndWorkloadMetaData(__workload_difficulty, __impossible_depth){

        DatasetAndWorkloadSeed dataset_and_workload_seed = DatasetAndWorkloadSeed(dataset, file_path, _workload_difficulty);

        prep_dataset_and_workload(dataset_and_workload_seed, _workload_difficulty, _impossible_depth);

        process();

        if(do_translate_to_base) {

            translate_to_base();
            process();

            if(false) {
//                DatasetAndWorkload to_compare(dataset_and_workload_seed, workload_difficulty, true);

//            size_t num_negative_workload = get_negative_workload().size();
//            DatasetAndWorkload original = DatasetAndWorkload(dataset, workload);

                translate_to_binary();

                process();

//            get_negative_workload(&original);
//            assert(num_negative_workload == negative_workload.size());

                assert(get_unique_chars().size() == _base);

                _base = final_base;

                translate_binary_to_to_base();

                process();

//            get_negative_workload(&original);
//            assert(num_negative_workload == negative_workload.size());

//                assert(to_compare == *this);
            }
        }


    }

    bool operator == (const DatasetAndWorkload& other) const
    {
        assert(dataset == other.dataset);
        assert(workload == other.workload);
        return dataset == other.dataset && workload == other.workload;
    }

//    DatasetAndWorkload(const vector<string>& _dataset, const Workload& _workload):
//        dataset(_dataset), workload(_workload){
//
//        for(const auto& it: dataset) {
//            process_str(it);
//        }
//        for(const auto& it: workload) {
//            process_str(it.first);
//            process_str(it.second);
//        }
//        assert(get_unique_chars().size() <= final_base);
//    }

    DatasetAndWorkload(const vector<string>& _dataset, const Workload& _workload, const DatasetAndWorkloadMetaData& to_copy_meta_data):
            DatasetAndWorkloadMetaData(to_copy_meta_data.original_meta_data, true), dataset(_dataset), workload(_workload){

        assert(get_unique_chars().size() <= final_base);
    }

    char get_max_char() const {
        return max_char;
    }
    char get_min_char() const {
        return min_char;
    }
    size_t get_max_length() const {
        return max_length;
    }

    const Dataset& get_dataset() const
    {
        return dataset;
    }
    const Workload& get_workload() const
    {
        return workload;
    }

    const RangeFilterScore * test_range_filter(RangeFilterTemplate* rf, bool do_print = false) const;

    const RangeFilterScore * eval_point_query(PointQuery* pq) const;

    void prep_dataset_and_workload(const DatasetAndWorkloadSeed& dataset_and_workload_seed, const string& workload_difficulty, const int impossible_depth = -1);

    const Workload& get_negative_workload(DatasetAndWorkload* prev= nullptr)
    {
        if(!negative_workload_defined) {
            for (size_t i = 0; i < workload.size(); i++) {
                string left_key = workload[i].first;
                string right_key = workload[i].second;

                bool ret = dataset.contains(left_key, right_key);
                if(prev != nullptr) {
                    bool prev_ret = prev->dataset.contains(prev->workload[i].first, prev->workload[i].second);
                    assert(ret == prev_ret);
                }

                if (!ret) {
                    negative_workload.push_back(workload[i]);
                }
            }
            negative_workload_defined = true;
        }
        return negative_workload;
    }

    int get_max_length_of_dataset() const;

    const Workload &get_negative_workload_assert_has() const;

    vector<DatasetAndWorkload> split_workload(int i);

    string stats_to_string();

    void build_ground_truth_range_filter();

    const RangeFilterTemplate &get_ground_truth_range_filter();
};

#endif //SURF_DATASETANDWORKLOAD_H
