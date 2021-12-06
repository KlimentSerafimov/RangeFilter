//
// Created by kliment on 12/4/21.
//

#ifndef SURF_RANGEFILTERONEBLOOM_H
#define SURF_RANGEFILTERONEBLOOM_H

#include "bloom/bloom_filter.hpp"
#include <cassert>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>

using namespace std;

class RangeFilterOneBloom
{
    bloom_filter bf;
    char last_char;
    char init_char;
    char is_leaf_char;
    int max_length;
    long long total_num_chars;

public:

    void calc_metadata(const vector<string>& dataset, const vector<pair<string, string> >& workload, bool do_print)
    {
        last_char = (char)0;
        init_char = (char)127;
        max_length = 0;

        total_num_chars = 0;

        int char_count[127];
        memset(char_count, 0, sizeof(char_count));

        for(int i = 0;i<(int)dataset.size();i++)
        {
            max_length = max(max_length, (int)dataset[i].size());
            total_num_chars+=(int)dataset[i].size();
            for(int j = 0;j<(int)dataset[i].size();j++)
            {
                last_char = (char)max((int)last_char, (int)dataset[i][j]);
                init_char = (char)min((int)init_char, (int)dataset[i][j]);
                char_count[(int)dataset[i][j]] += 1;
            }
        }

        for(size_t i = 0;i<workload.size();i++)
        {
            for(size_t j = 0;j<workload[i].first.size();j++)
            {
                last_char = (char)max((int)last_char, (int)workload[i].first[j]);
                init_char = (char)min((int)init_char, (int)workload[i].first[j]);
            }
            for(size_t j = 0;j<workload[i].second.size();j++)
            {
                last_char = (char)max((int)last_char, (int)workload[i].second[j]);
                init_char = (char)min((int)init_char, (int)workload[i].second[j]);
            }
        }

        int num_unique_chars = 0;
        for(int i = 0;i<127;i++)
        {
            if(char_count[i] != 0)
            {
                num_unique_chars+=1;
            }
        }

        is_leaf_char = (char)((int)init_char-1);

        if(do_print) {
            cout << "RANGE FILTER STATS" << endl;
            cout << "num_strings " << (int) dataset.size() << endl;
            cout << "num_chars " << total_num_chars << endl;
            cout << "init_char " << init_char << endl;
            cout << "last_char " << last_char << endl;
            cout << "last_char-init_char+1 " << last_char - init_char + 1 << endl;
            cout << "|unique_chars| " << num_unique_chars << endl;
            cout << "is_leaf_char " << is_leaf_char << endl;
        }
    }

    bloom_parameters get_bloom_parameters(int size, double fpr)
    {

        bloom_parameters parameters;

        parameters.projected_element_count = size;

        parameters.false_positive_probability = fpr;

        parameters.random_seed = 0xA5A5A5A5;

        if (!parameters)
        {
            cout << "Error - Invalid set of bloom filter parameters!" << endl;
            assert(false);
            return parameters;
        }

        parameters.compute_optimal_parameters();

        return parameters;
    }

    RangeFilterOneBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, double fpr, bool do_print = false)
    {

//        const int max_depth = 4;
//        vector<string> dataset;
//
//        set<string> unique_prefixes;
//
//        for(size_t i = 0;i<init_dataset.size();i++)
//        {
//            dataset.emplace_back(init_dataset[i].substr(0, min(max_depth, (int)init_dataset[i].size())));
//        }
//
//        for(size_t i = 0;i<dataset.size();i++)
//        {
//            for(size_t j = 0;j<dataset[i].size();j++)
//            {
//                unique_prefixes.insert(dataset[i].substr(0, j+1));
//            }
//        }

        calc_metadata(dataset, workload, do_print);

        bloom_parameters parameters = get_bloom_parameters((int)total_num_chars, fpr);

        if(do_print)
        {
            cout << "expected total queries " << parameters.projected_element_count << endl;
        }


        //Instantiate Bloom Filter
        bf = bloom_filter(parameters);

        // Insert into Bloom Filter
        // Insert some strings
        long long num_inserted = 0;
        for (size_t i = 0; i < dataset.size(); ++i) {
            for(size_t j = 0;j<dataset[i].size();j++) {
                num_inserted += 1;
                string substr = dataset[i].substr(0, j+1);
//                    cout << substr << endl;
                bf.insert(substr);
                if ((num_inserted+1) % 100000000 == 0) {
                    cout << "inserted(chars) " << num_inserted+1 << "/" << total_num_chars << endl;
                }
            }
            string leaf = dataset[i]+is_leaf_char;
            num_inserted += 1;
//                cout << leaf << endl;
            bf.insert(leaf);
            if ((i+1) % 10000000 == 0) {
                cout << "inserted(strings) " << i+1 << "/" << dataset.size() << endl;
            }
        }
    }

    bool str_invariant(string q)
    {
        bool ret = true;
        for(int i = 0;i<(int)q.size();i++)
        {
            ret &= (init_char <= q[i]);
            ret &= (q[i] <= last_char);
            if(!ret)
            {
                assert(false);
                break;
            }
        }
        return ret;
    }

    string pad(string s, int num, char c) const
    {
        while((int) s.size() < num)
        {
            s+=c;
        }
        return s;
    }

    void breakpoint(bool ret)
    {
//        cout << "ret" << endl;
    }

    bool query(string left, string right)
    {
        assert(str_invariant(left));
        assert(str_invariant(right));

        int max_n = max(max_length, (int)max(left.size(), right.size()));

        if((int)left.size()<max_n)
            left[left.size()-1]-=1;
        left = pad(left, max_n, last_char);
        right = pad(right, max_n, init_char);

        assert(left.size() == right.size());
        assert((int)left.size() == max_n);

        assert(left <= right);
        int n = left.size();
        int id = 0;
        string prefix;

        //check common substring
        //e.g left = aaabbb, right = aaaqqq
        //then this checks up to 'aaa'
        while(left[id] == right[id] && id < n) {
            prefix += left[id];
            if (!bf.contains(prefix)) {
                breakpoint(false);
                return false;
            }
            id++;
        }
        assert(id <= n);
        if(id == n)
        {
            breakpoint(true);
            return true;
        }

        //check first non-boundary divergent character.
        //e.g. aaac, aaad, ... aaap (without aaab, and aaaq)
        for(char c = (char)((int)left[id]+1); c <= (char)((int)right[id]-1); c++)
        {
            string local_prefix = prefix+c;
            if(bf.contains(local_prefix))
            {
                breakpoint(true);
                return true;
            }
        }


        //left boundary
        {
            //check boundary divergent character on the left
            //e.g. aaab
            string left_prefix = prefix + left[id];
            bool continue_left = false;
            if (bf.contains(left_prefix)) {
                continue_left = true;
            }
            if (continue_left) {
                int left_id = id + 1;
                //check boundary prefixes on the left; after divergent character
                //eg. aaabb .. aaabz
                //aaabba, aaabab ... aaabaz
                while (left_id < (int)left.size()) {
                    //check non-boundary characters
                    //aaabc .. aaabz
                    for (char c = (char) ((int) left[left_id] + 1); c <= (char) ((int) last_char); c++) {
                        string local_prefix = left_prefix + c;
                        if (bf.contains(local_prefix)) {
                            breakpoint(true);
                            return true;
                        }
                    }
                    //check boundary character
                    //aaabb
                    left_prefix += left[left_id];
                    if (bf.contains(left_prefix)) {
                        continue_left = true;
                        //continue checking
                    } else {
                        continue_left = false;
                        //if it reached here, means that left is empty.
                        break;
                    }
                    left_id++;
                }
                if(continue_left)
                {
                    breakpoint(true);
                    return true;
                }
                //if it reached here, means that left is empty.
            }
        }


        //right boundary
        {
            //check boundary divergent character on the right
            //e.g. aaaq
            string right_prefix = prefix + right[id];
            bool continue_right = false;
            if (bf.contains(right_prefix)) {
                if(bf.contains(right_prefix+is_leaf_char))
                {
                    //right_prefix is a leaf;
                    breakpoint(true);
                    return true;
                }
                continue_right = true;
            }


            if (continue_right) {
                int right_id = id + 1;
                //check boundary prefixes on the left; after divergent character
                //eg. aaaqa .. aaaqq
                //aaaqqa, aaaqqb ... aaaqqz
                while (right_id < (int)right.size()) {
                    //check non-boundary characters
                    //aaaqa .. aaaqp
                    for (char c = (char) ((int) init_char); c <= (char) ((int) right[right_id] - 1); c++) {
                        string local_prefix = right_prefix + c;
                        if (bf.contains(local_prefix)) {
                            breakpoint(true);
                            return true;
                        }
                    }
                    //check boundary character
                    //aaabb
                    right_prefix += right[right_id];
                    if (bf.contains(right_prefix)) {
                        continue_right = true;
                        if(bf.contains(right_prefix+is_leaf_char))
                        {
                            //right_prefix is a leaf;
                            breakpoint(true);
                            return true;
                        }
                        //continue checking
                    } else {
                        continue_right = false;
                        //if it reached here, means that right is empty.
                        break;
                    }
                    right_id++;
                }
                breakpoint(continue_right);
                return continue_right;
            }
            else
            {

                breakpoint(false);
                return false;
            }
        }

    }

    string get_init_char() {
        string ret;
        ret+=init_char;
        return ret;
    }

    string get_last_char() {
        string ret;
        ret+=last_char;
        return ret;
    }


    unsigned long long get_memory() {
        return bf.get_memory() + sizeof(bloom_filter);
    }

    void clear()
    {
        bf.clear_memory();
    }
};


#endif //SURF_RANGEFILTERONEBLOOM_H
