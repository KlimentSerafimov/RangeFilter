//
// Created by kliment on 2/26/22.
//

#ifndef SURF_DATASETANDWORKLOAD_H
#define SURF_DATASETANDWORKLOAD_H

#include <string>
#include <vector>
#include "PointQuery.h"
#include <iostream>
#include "surf_implementation.h"
#include <set>
#include <map>
#include <algorithm>
#include "fstream"

using namespace std;

class RangeFilterTemplate;

vector<string> read_dataset(const string& file_path);

class DatasetAndWorkloadMetaData
{

    string to_binary_string(int x, size_t n)
    {
        assert(_base == 2);
        string ret = "";
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

protected:

    string to_binary_string(string str)
    {
        string new_str;
        for(auto c: str)
        {
            new_str += to_binary_string(char_to_id[c], num_bits_per_char);
        }
        return new_str;
    }


    size_t max_length = 0;
    char min_char = (char)127;
    char max_char = (char)0;

    int max_id = -1;
    set<char> unique_chars;
    map<char, int> char_to_id;
    vector<char> id_to_char;
    int num_bits = -1;
    size_t _base = 2;
    size_t final_base = 64;
public:
    int num_bits_per_char = -1;
    const DatasetAndWorkloadMetaData& original_meta_data;

    DatasetAndWorkloadMetaData(): original_meta_data(*this) {}

    DatasetAndWorkloadMetaData(const DatasetAndWorkloadMetaData& is_original, bool assert_true): original_meta_data(is_original) {
        max_length = is_original.max_length;
        min_char = is_original.min_char;
        max_char = is_original.max_char;
        assert(assert_true);
    }

    string to_string_base(const string& str) const
    {
        assert(num_bits != -1);
        string new_str;
        int num = 0;
        int p = 1<<(num_bits-1);
        int bit_id = 0;
//        cout << "str: " << str << endl;
        for(auto c: str)
        {
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

    void process_str(const string& str)
    {
        assert(!str.empty());
        max_length = max(max_length, str.size());
        for(auto c : str) {
            min_char = min(min_char, c);
            max_char = max(max_char, c);
            unique_chars.insert(c);
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

class DatasetAndWorkload: public DatasetAndWorkloadMetaData
{
    vector<string> dataset;
    vector<pair<string, string> > workload;

    bool negative_workload_defined = false;
    vector<pair<string, string> > negative_workload;
    void translate_to_base()
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
            dataset[i] = to_string_base(dataset[i]);
        }

        for(size_t i = 0;i<workload.size();i++)
        {
            workload[i] = make_pair(
                    to_string_base(workload[i].first),
                    to_string_base(workload[i].second));
        }


    }

    void translate_to_binary()
    {
        assert(_base == 2);
        assert(id_to_char.empty());
        assert(char_to_id.empty());

        id_to_char.push_back(0);

        size_t id = 1;
        char prev = 0;
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


public:

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

    DatasetAndWorkload(const string& file_path, const string& workload_difficulty, bool do_translate_to_base) {

        prep_dataset_and_workload(file_path, workload_difficulty);

        for(const auto& it: dataset) {
            process_str(it);
        }
        for(const auto& it: workload) {
            process_str(it.first);
            process_str(it.second);
        }

        if(do_translate_to_base) {

            size_t num_negative_workload = get_negative_workload().size();

            DatasetAndWorkload original = DatasetAndWorkload(dataset, workload);

            translate_to_binary();

            max_length = 0;
            min_char = (char) 127;
            max_char = (char) 0;
            unique_chars.clear();
            negative_workload_defined = false;
            negative_workload.clear();

            for (const auto &it: dataset) {
                process_str(it);
            }
            for (const auto &it: workload) {
                process_str(it.first);
                process_str(it.second);
            }

            get_negative_workload(&original);

            assert(num_negative_workload == negative_workload.size());

            assert(unique_chars.size() == _base);

            _base = final_base;

            translate_to_base();

            max_length = 0;
            min_char = (char) 127;
            max_char = (char) 0;
            unique_chars.clear();
            negative_workload_defined = false;
            negative_workload.clear();

            for (const auto &it: dataset) {
                process_str(it);
            }
            for (const auto &it: workload) {
                process_str(it.first);
                process_str(it.second);
            }

            get_negative_workload(&original);

            assert(num_negative_workload == negative_workload.size());
        }
    }

    DatasetAndWorkload(const vector<string>& _dataset, const vector<pair<string, string> >& _workload):
        dataset(_dataset), workload(_workload){

        for(const auto& it: dataset) {
            process_str(it);
        }
        for(const auto& it: workload) {
            process_str(it.first);
            process_str(it.second);
        }
        assert(unique_chars.size() <= final_base);
    }

    DatasetAndWorkload(const vector<string>& _dataset, const vector<pair<string, string> >& _workload, const DatasetAndWorkloadMetaData& to_copy_meta_data):
            DatasetAndWorkloadMetaData(to_copy_meta_data.original_meta_data, true), dataset(_dataset), workload(_workload){

        assert(unique_chars.size() <= final_base);
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

    const vector<string>& get_dataset() const
    {
        return dataset;
    }
    const vector<pair<string, string> >& get_workload() const
    {
        return workload;
    }



    RangeFilterStats test_range_filter(RangeFilterTemplate* rf, bool do_print = false) const;

    RangeFilterStats eval_point_query(PointQuery* pq) const;

    void prep_dataset_and_workload(const string& file_path, const string& workload_difficulty, int impossible_depth = -1);

    const vector<pair<string, string> >& get_negative_workload(DatasetAndWorkload* prev= nullptr)
    {
        if(!negative_workload_defined) {
            for (size_t i = 0; i < workload.size(); i++) {
                string left_key = workload[i].first;
                string right_key = workload[i].second;

                bool ret = contains(dataset, left_key, right_key);
                if(prev != nullptr) {
                    bool prev_ret = contains(prev->dataset, prev->workload[i].first, prev->workload[i].second);
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

    const vector<pair<string, string> > &get_negative_workload_assert_has() const;
};

#endif //SURF_DATASETANDWORKLOAD_H
