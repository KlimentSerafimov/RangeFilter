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



class TriePointQuery: public PointQuery
{
    Trie* trie;
public:
    TriePointQuery(): trie(new Trie()) {}

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

    void _clear() override
    {
        assert(false);
        trie->clear();
    }
};

#include "DatasetAndWorkload.h"

class MemorizedPointQuery: public PointQuery
{
    bool is_sorted = true;
    vector<Dataset> datasets;
    bool should_be_sorted = false;
    size_t local_cutoff = 0;
public:
    
    MemorizedPointQuery() { }

    int get_num_prefixes(size_t cutoff) const {
        assert(cutoff == local_cutoff);
        int ret = 0;
        for(size_t i = 0; i<cutoff;i++) {
            ret+=datasets[i].size();
        }
        return ret;
    }

    bool contains(const string& s) override {
        assert(!s.empty());
        if(!is_sorted) {
            assert(!should_be_sorted);
            for(auto dataset:datasets) {
                sort(dataset.begin(), dataset.end());
                dataset.remove_duplicates();
            }
            is_sorted = true;
            should_be_sorted = true;
        }
        local_cutoff = max(local_cutoff, s.size());
        size_t dataset_id = s.size()-1;
        assert(dataset_id < datasets.size());
        Dataset& dataset = datasets[dataset_id];
        return dataset.contains(s, s);
    }

//    bool range_query(const string& left_key, const string& right_key) override {
//        if(!is_sorted) {
//            assert(!should_be_sorted);
//            sort(dataset.begin(), dataset.end());
//            is_sorted = true;
//            should_be_sorted = true;
//        }
//        return DatasetAndWorkload::contains(dataset, left_key, right_key);
//    }

    void insert(const string& s) override
    {
        assert(!s.empty());
        size_t dataset_id = s.size()-1;
        if(dataset_id >= datasets.size()) {
            assert(dataset_id == datasets.size());
            datasets.emplace_back();
        }
        assert(dataset_id < datasets.size());
        Dataset& dataset = datasets[dataset_id];
        if(!dataset.empty()) {
            if(*dataset.rbegin() > s) {
                assert(!should_be_sorted);
                is_sorted = false;
            }
            else if(*dataset.rbegin() == s) {
                return;
            }
        }
        dataset.push_back(s);
    }

    unsigned long long get_memory() override {
        unsigned long long ret = 0;
        for(size_t dataset_id = 0; datasets.size();dataset_id++) {
            const Dataset &dataset = datasets[dataset_id];
            for (size_t i = 0; i < dataset.size(); i++) {
                ret += dataset[i].size() * sizeof(char);
            }
        }
        return ret;
    }

    void _clear() override {
        for(auto dataset:datasets) {
            dataset.clear();
        }
        datasets.clear();
    }
};

class GroundTruthPointQuery: public MemorizedPointQuery {};

class RangeFilterTemplate
{
    PointQuery* pq;
    char max_char{};
    char min_char{};
    char no_char{};
    char is_leaf_char{};
    size_t max_length{};

    mutable int num_negative_point_queries = 0;

    mutable bool has_prev_num_negative_point_queries = false;
    mutable int prev_num_negative_point_queries = 0;

    void calc_metadata(const DatasetAndWorkload& dataset_and_workload, bool do_print);

private:
    mutable bool track_negative_point_queries = false;
    mutable map<string, int> prefix_to_negative_point_queries;
//    mutable map<string, int> negative_point_queries;

    void reset() const
    {
        track_negative_point_queries = false;
        prefix_to_negative_point_queries.clear();
//        negative_point_queries.clear();
        has_prev_num_negative_point_queries = true;
        prev_num_negative_point_queries = num_negative_point_queries;
        num_negative_point_queries = 0;
    }

    bool contains(const string& s, const string& left_str, const string& right_str) const
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

                //---
                if (prefix_to_negative_point_queries.find(record_s) == prefix_to_negative_point_queries.end()) {
                    prefix_to_negative_point_queries[record_s] = 0;
                }
                prefix_to_negative_point_queries[record_s] += 1;

                //---
//                if (negative_point_queries.find(s) == negative_point_queries.end()) {
//                    negative_point_queries[s] = 0;
//                }
//                negative_point_queries[s] += 1;

                //---
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

    void breakpoint(bool ret) const
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

    bool query(const string& _left, const string& _right) const
    {
        string left = _left;
        string right = _right;
        assert(!left.empty() && !right.empty());
        assert(str_invariant(left));
        assert(str_invariant(right));
        assert(left <= right);

        if(pq->get_has_range_query()) {
            return pq->range_query(left, right);
        }

        size_t max_n = max(max_length, max(left.size(), right.size()));

        if(left.size()<max_n) {
            assert(!left.empty());
            size_t at = left.size()-1;
            if(left[at] == 0) {
                assert(false);
                left.pop_back();
            }
            else {
                left[at] -= 1;
            }
        }

        left = pad(left, max_n, max_char);
        assert(no_char == (char)((int)min_char-1) && no_char >= 0);
        right = pad(right, max_n, no_char);

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
                    bool out_of_scope = false;
                    if(left_prefix.size() == _left.size()) {
                        out_of_scope = true;
                        assert(left_prefix < _left && _left.substr(0, left_prefix.size()) != left_prefix);
                    }
                    else {
                        assert(!(left_prefix < _left && _left.substr(0, left_prefix.size()) != left_prefix));
                    }
                    if(left_prefix < _left && _left.substr(0, left_prefix.size()) != left_prefix) {
                        assert(out_of_scope);
                    }

                    if (!out_of_scope && contains(left_prefix, left, right)) {
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
                        assert(right[right_id] == no_char);
                    }
                    //check boundary character
                    //aaabb
                    right_prefix += right[right_id];

                    bool out_of_scope = false;
                    if(right_prefix.size() == _right.size()+1) {
                        out_of_scope = true;
                        assert(right_prefix > _right && _right.substr(0, right_prefix.size()) != right_prefix);
                    }
                    else {
                        assert(!(right_prefix > _right && _right.substr(0, right_prefix.size()) != right_prefix));
                    }

                    if(right_prefix > _right && _right.substr(0, right_prefix.size()) != right_prefix) {
                        assert(out_of_scope);
                    }

                    if (!out_of_scope && contains(right_prefix, left, right)) {
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

    int get_negative_point_queries() const {
        if(track_negative_point_queries) {
            return num_negative_point_queries;
        }
        else {
            assert(has_prev_num_negative_point_queries);
            return prev_num_negative_point_queries;
        }
    }

    pair<double, string>* analyze_negative_point_query_density_heatmap(const DatasetAndWorkload& dataset_and_workload, bool do_print = false) const;

    void build_heatmap(const DatasetAndWorkload& dataset_and_workload) const;

    void set_point_query(PointQuery *new_pq);

    size_t get_cutoff();

    size_t get_unique_negative_point_queries();

    bool is_cold() const;
};


#endif //SURF_RANGEFILTERTEMPLATE_H
