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

using namespace std;

class RangeFilterTemplate;

vector<string> read_dataset(const string& file_path);

class DatasetAndWorkload
{
    vector<string> dataset;
    vector<pair<string, string> > workload;

    size_t max_length = 0;
    char min_char = (char)127;
    char max_char = (char)0;
    set<char> unique_chars;

    bool negative_workload_defined = false;
    vector<pair<string, string> > negative_workload;

    map<char, int> char_to_id;
    int max_id = -1;
    int num_bits_per_char = -1;

    size_t _base = 2;
    size_t final_base = 128;

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

    string to_binary_string(string str)
    {
        string new_str;
        for(auto c: str)
        {
            new_str += to_binary_string(char_to_id[c], num_bits_per_char);
        }
        return new_str;
    }

    int num_bits = -1;

    string to_string_base(string str) const
    {
        assert(num_bits != -1);
        string new_str;
        int num = 0;
        int p = 1<<(num_bits-1);
        int bit_id = 0;
        for(auto c: str)
        {
            assert(p>=1);
//            cout << c;
            if(c == '1')
            {
                num+=p;
            }
            else
            {
                assert(c == '0');
            }
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
        if(bit_id >= 1)
        new_str+=(char)num;
        return new_str;
    }


    void translate_to_binary()
    {
        assert(_base == 2);
        int id = 0;
        char prev = 0;
        for(auto c : unique_chars)
        {
            assert(prev <= char_to_id[c]);
            prev = char_to_id[c];
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
public:

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

    DatasetAndWorkload(const string& file_path, const string& workload_difficulty, bool do_translate_to_base){

        prep_dataset_and_workload(file_path, workload_difficulty);

        for(const auto& it: dataset) {
            process_str(it);
        }
        for(const auto& it: workload) {
            process_str(it.first);
            process_str(it.second);
        }

        if(do_translate_to_base) {

            size_t num_negative_workload = negative_workload.size();

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

            get_negative_workload();

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

            get_negative_workload();

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



    RangeFilterStats test_range_filter(RangeFilterTemplate* rf, bool do_print = false);

    RangeFilterStats eval_point_query(PointQuery* pq);

    int prep_dataset_and_workload(const string& file_path, const string& workload_difficulty, int impossible_depth = -1);

    const vector<pair<string, string> >& get_negative_workload()
    {
        if(!negative_workload_defined) {
            for (size_t i = 0; i < workload.size(); i++) {
                string left_key = workload[i].first;
                string right_key = workload[i].second;

                bool ret = contains(dataset, left_key, right_key);

                if (!ret) {
                    negative_workload.push_back(workload[i]);
                }
            }
            negative_workload_defined = true;
        }
        return negative_workload;
    }

    int get_max_length_of_dataset();
};

#endif //SURF_DATASETANDWORKLOAD_H
