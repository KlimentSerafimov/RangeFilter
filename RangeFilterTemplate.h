//
// Created by kliment on 12/6/21.
//

#ifndef SURF_RANGEFILTERTEMPLATE_H
#define SURF_RANGEFILTERTEMPLATE_H

#include <string>
#include <utility>
#include <vector>
#include <cstring>
#include <iostream>
#include <cassert>
#include "PointQuery.h"
#include "Trie.h"
#include "Frontier.h"
#include <map>
#include <fstream>
#include <algorithm>

using namespace std;

class DatasetAndWorkloadMetaData;

class DatasetAndWorkload;

class GroundTruthPointQuery: public PointQuery
{
public:
    Trie* trie;
    GroundTruthPointQuery(): trie(new Trie()) {}

    bool contains(const string& s) override
    {
        return trie->contains(s);
    }

    void insert(const string& s) override
    {
        trie->insert(s);
    }

    unsigned long long get_memory() override {
        return trie->get_memory();
    }

    void clear() override
    {
        trie->clear();
    }

};

class RangeFilterTemplate
{
    PointQuery* pq;
    char max_char{};
    char min_char{};
    char is_leaf_char{};
    size_t max_length{};

    int num_negative_point_queries = 0;

    void calc_metadata(const DatasetAndWorkload& dataset_and_workload, bool do_print);

public:
    ~RangeFilterTemplate(){ }
private:
    bool track_negative_point_queries = false;
    map<string, int> prefix_to_negative_point_queries;

    void reset()
    {
        track_negative_point_queries = false;
        prefix_to_negative_point_queries.clear();
        num_negative_point_queries = 0;
    }

    bool contains(const string& s, const string& left_str, const string& right_str)
    {
        if(s.empty())
        {
            return true;
        }
        bool ret = pq->contains(s, left_str, right_str);
        if(track_negative_point_queries) {
            if (!ret) {
                string sub_s = s.substr(0, s.size() - 1);
                assert(contains(sub_s, left_str, right_str));

                const string& record_s = s;

                if (prefix_to_negative_point_queries.find(record_s) == prefix_to_negative_point_queries.end()) {
                    prefix_to_negative_point_queries[record_s] = 0;
                }
                prefix_to_negative_point_queries[record_s] += 1;
                num_negative_point_queries += 1;
            }
        }
        return ret;
    }
    bool str_invariant(string q) const
    {
        bool ret = true;
        for(int i = 0;i<(int)q.size();i++)
        {
            ret &= (min_char <= q[i]);
            ret &= (q[i] <= max_char);
            if(!ret)
            {
                break;
            }
        }
        return ret;
    }

    static string pad(string s, size_t num, char c)
    {
        while(s.size() < num)
        {
            s+=c;
        }
        return s;
    }

    void breakpoint(bool ret)
    {
        if(do_breakpoint && ret)
        cout << "ret" << endl;
    }

public:

    bool do_breakpoint = false;

    void insert_prefixes(const vector<string>& dataset){
        for (size_t i = 0; i < dataset.size(); ++i) {
            string prefix;
            for(size_t j = 0;j<dataset[i].size();j++) {
                prefix+=dataset[i][j];
                pq->insert(prefix, false);
            }
            prefix+=is_leaf_char;
            pq->insert(prefix, true);
        }
    }

    RangeFilterTemplate(const DatasetAndWorkload& dataset_and_workload, PointQuery* _pq, bool do_print = false);

    bool query(string left, string right)
    {
        assert(!left.empty() && !right.empty());
        assert(str_invariant(left));
        assert(str_invariant(right));

        if(pq->get_has_range_query()) {
            return pq->range_query(left, right);
        }

        size_t max_n = max(max_length, max(left.size(), right.size()));

        if(left.size()<max_n) {
            assert(left.size() >= 1);
            size_t at = left.size()-1;
            if(left[at] == 0) {
                left.pop_back();
            }
            else {
                left[at] -= 1;
            }
        }

        left = pad(left, max_n, max_char);
        right = pad(right, max_n, min_char);

        assert(left.size() == right.size());
        assert(left.size() == max_n);

        assert(left <= right);


        size_t n = left.size();
        size_t id = 0;
        string prefix;

        //check common substring
        //e.g left = aaabbb, right = aaaqqq
        //then this checks up to 'aaa'
        while(left[id] == right[id] && id < n) {
            prefix += (char)left[id];
            assert(!prefix.empty());
            assert(prefix.size() >= 1);
            if (!contains(prefix, left, right)) {
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
        if(right[id] >= 1) {
            for (char c = (char) ((int) left[id] + 1); c <= (char) ((int) right[id] - 1); c++) {
                string local_prefix = prefix + c;
                if (contains(local_prefix, left, right)) {
                    breakpoint(true);
                    return true;
                }
            }
        }
        else
        {
            assert(right[id] == 0);
            assert(right[id] == min_char);
        }


        //left boundary
        {
            //check boundary divergent character on the left
            //e.g. aaab
            string left_prefix = prefix + left[id];
            bool continue_left = false;
            if (contains(left_prefix, left, right)) {
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
                    for (char c = (char) ((int) left[left_id] + 1); c <= (char) ((int) max_char); c++) {
                        string local_prefix = left_prefix + c;
                        if (contains(local_prefix, left, right)) {
                            breakpoint(true);
                            return true;
                        }
                    }
                    //check boundary character
                    //aaabb
                    left_prefix += left[left_id];
                    if (contains(left_prefix, left, right)) {
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
            if (contains(right_prefix, left, right)) {
                string next_right_prefix = right_prefix+is_leaf_char;
                if(contains(next_right_prefix, left, right))
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
                    if( right[right_id] >= 1) {
                        for (char c = (char) ((int) min_char); c <= (char) ((int) right[right_id] - 1); c++) {
                            string local_prefix = right_prefix + c;
                            if (contains(local_prefix, left, right)) {
                                breakpoint(true);
                                return true;
                            }
                        }
                    }
                    else {
                        assert(right[right_id] == 0);
                        assert(right[right_id] == min_char);
                    }
                    //check boundary character
                    //aaabb
                    right_prefix += right[right_id];
                    if (contains(right_prefix, left, right)) {
                        continue_right = true;
                        if(contains(right_prefix+is_leaf_char, left, right))
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


    unsigned long long get_memory() {
        return pq->get_memory() + 0*((3*sizeof(char) + sizeof(int) + sizeof(long long)));
    }

    PointQuery* get_point_query()
    {
        return pq;
    }

    char get_is_leaf_char() const {
        return is_leaf_char;
    }

    int get_negative_point_queries() {
        return num_negative_point_queries;
    }

    pair<double, string>* analyze_negative_point_query_density_heatmap(const DatasetAndWorkload& dataset_and_workload);

    void set_point_query(PointQuery *new_pq);
};


#endif //SURF_RANGEFILTERTEMPLATE_H
