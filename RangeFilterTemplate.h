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

class DatasetAndWorkload;

class GroundTruthPointQuery: public PointQuery
{
public:
    Trie* trie;
    GroundTruthPointQuery(): trie(new Trie()) {}

    bool contains(string s) override
    {
        return trie->contains(s);
    }

    void insert(string s) override
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
    ~RangeFilterTemplate(){
        pq->clear();
    }
private:
    bool track_negative_point_queries = false;
    map<string, int> prefix_to_negative_point_queries;

    void reset()
    {
        track_negative_point_queries = false;
        prefix_to_negative_point_queries.clear();
        num_negative_point_queries = 0;
    }

    bool contains(const string& s)
    {
        if(s.empty())
        {
            return true;
        }
        bool ret = pq->contains(s);
        if(track_negative_point_queries) {
            if (!ret) {
                string sub_s = s.substr(0, s.size() - 1);
                assert(contains(sub_s));

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
//        cout << "ret" << endl;
    }

public:

    void insert_prefixes(const vector<string>& dataset){
        for (size_t i = 0; i < dataset.size(); ++i) {
            string prefix;
            for(size_t j = 0;j<dataset[i].size();j++) {
                prefix+=dataset[i][j];
                pq->insert(prefix);
            }
            prefix+=is_leaf_char;
            pq->insert(prefix);
        }
    }

    RangeFilterTemplate(const DatasetAndWorkload& dataset_and_workload, PointQuery* _pq, bool do_print = false);

    bool query(string left, string right)
    {
        assert(str_invariant(left));
        assert(str_invariant(right));

        size_t max_n = max(max_length, max(left.size(), right.size()));

        if(left.size()<max_n)
            left[left.size()-1]-=1;
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
            prefix += left[id];
            if (!contains(prefix)) {
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
            if(contains(local_prefix))
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
            if (contains(left_prefix)) {
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
                        if (contains(local_prefix)) {
                            breakpoint(true);
                            return true;
                        }
                    }
                    //check boundary character
                    //aaabb
                    left_prefix += left[left_id];
                    if (contains(left_prefix)) {
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
            if (contains(right_prefix)) {
                string next_right_prefix = right_prefix+is_leaf_char;
                if(contains(next_right_prefix))
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
                    for (char c = (char) ((int) min_char); c <= (char) ((int) right[right_id] - 1); c++) {
                        string local_prefix = right_prefix + c;
                        if (contains(local_prefix)) {
                            breakpoint(true);
                            return true;
                        }
                    }
                    //check boundary character
                    //aaabb
                    right_prefix += right[right_id];
                    if (contains(right_prefix)) {
                        continue_right = true;
                        if(contains(right_prefix+is_leaf_char))
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
        return pq->get_memory() + 3*sizeof(char) + sizeof(int) + sizeof(long long);
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

    pair<double, string>* analyze_negative_point_query_density_heatmap(const vector<string>& dataset, const vector<pair<string, string> >& negative_workload) {
        assert(!track_negative_point_queries);

        track_negative_point_queries = true;

        for(const auto& it: negative_workload) {
            assert(query(it.first, it.second) == false);
        }

        assert(track_negative_point_queries);

        vector<pair<string, string> > inits;
        vector<pair<string, string> > ends;

        for(size_t i = 0;i<negative_workload.size();i++)
        {
            assert(negative_workload[i].first <= negative_workload[i].second);
            inits.push_back(negative_workload[i]);
        }

        sort(inits.begin(), inits.end());

        vector<string> arr;
        vector<int> negative_count;
        int sum = 0;
        for(const auto& it: prefix_to_negative_point_queries) {
            arr.emplace_back(it.first);
            negative_count.push_back(it.second);
            sum+=it.second;
        }
        sort(arr.begin(), arr.end());
        int prefix_sum = 0;
        size_t at_init = 0;
        vector<pair<float, float> > densities;

        ofstream out("densities.out");

        pair<float, string> best_split = make_pair(-1, "");

//        for(size_t at_arr = 0; at_arr < arr.size(); at_arr++)

        size_t at_arr = 0;

        Frontier<string> split_size_vs_density_ratio_frontier(2);

        size_t init_row_id = 0;
        size_t end_row_id = dataset.size()-1;

        assert(!inits.empty());

        bool enter = false;
        for(size_t row_id = 0; row_id < dataset.size();row_id++)
        {
            if(dataset[row_id] > inits[0].first) {
                init_row_id = row_id;
                enter = true;
                break;
            }
        }

        assert(enter);

        enter = false;

        for(int row_id = dataset.size()-1; row_id >= 0;row_id--)
        {
            if(dataset[row_id] < inits[inits.size()-1].second) {
                end_row_id = row_id;
                enter = true;
                break;
            }
        }

        assert(enter);

        for(size_t row_id = init_row_id; row_id <= end_row_id; row_id++) {
            while(at_init < inits.size() && inits[at_init].first < dataset[row_id]) {
                assert(inits[at_init].second < dataset[row_id]);
                at_init++;
            }
            if(at_init < inits.size())
                assert(inits[at_init].first > dataset[row_id]);

            while(at_arr < arr.size() && arr[at_arr] < dataset[row_id]) {
                prefix_sum += negative_count[at_arr];
                at_arr++;
            }
            if(at_arr < arr.size())
                assert(arr[at_arr] > dataset[row_id]);
            else{
                assert(prefix_sum == sum);
                assert(at_init == inits.size()-1);
            }

            if(at_init == 0 || inits.size() == at_init)
            {
                assert(false);
                //no split
                continue;
            }

            if(prefix_sum == 0 || prefix_sum == sum)
            {
                if(prefix_sum == sum)
                {
                    assert(at_arr == arr.size());
                }
                assert(false);
                continue;
            }

            if(at_arr >= arr.size())
            {
                assert(false);
            }

            assert(inits[at_init-1].second < dataset[row_id]);
            assert(inits[at_init].first > dataset[row_id]);

            float left_density = (float)prefix_sum/(at_init);
            float right_density = (float)(sum-prefix_sum)/(inits.size()-at_init);

            densities.emplace_back(left_density,right_density);
//            cout <<at_init <<" "<< left_density <<" "<< right_density << endl;

            float score = max(right_density/left_density, left_density/right_density);
            out << row_id << " "<< score <<  " left_density = "  << prefix_sum <<" / "<< at_init << " = " << left_density << " | right_density = " << (sum-prefix_sum) << "/" <<(inits.size() - at_init) << " = " << right_density <<" | ratios: " << left_density/right_density <<" "<< right_density/left_density << endl;

            best_split = max(best_split, make_pair(score, dataset[row_id]));


            vector<double> multi_d_score;
            multi_d_score.push_back(-score);

//            if(false) {
//                //size_ratio based on dataset.
//                int dataset_size = end_row_id-init_row_id+1;
//                int offset_row_id = row_id-init_row_id;
//                double size_ratio = max((float)offset_row_id/dataset_size, (float)(dataset_size-offset_row_id)/dataset_size);
//            }

            double size_ratio = max((float)at_init/inits.size(), (float)(inits.size()-at_init)/inits.size());
//            cout << size_ratio << endl;
            multi_d_score.push_back(size_ratio);

            split_size_vs_density_ratio_frontier.insert(dataset[row_id], multi_d_score);
        }
        out.close();

        cout << "best based on density ratio: " << best_split.first <<" "<< best_split.second << endl;

        split_size_vs_density_ratio_frontier.print(cout, 10, true);

        int constraint_relaxation_id = 1;

        pair<double, string>* ret = nullptr;

        while(ret == nullptr && constraint_relaxation_id <= 1) {
            vector<double> constraint;
            constraint.push_back(-2);
            constraint.push_back(1.0 - 0.1/constraint_relaxation_id);

            auto optimization_function = [](vector<double> in) {
                    return - in[0] * in[0] * (1 - in[1]);
            };

            pair<vector<double>, string> *almost_ret = split_size_vs_density_ratio_frontier.get_best_better_than(
                    constraint, optimization_function);

            if (almost_ret != nullptr) {
                ret = new pair<double, string>(-almost_ret->first[0], almost_ret->second);
            }
            cout << "constrain_relaxation_id " << constraint_relaxation_id << endl;

            if(ret != nullptr) {
                cout << "ret: " << ret->first << " " << ret->second << " size_ratio: " << almost_ret->first[1] << endl;
            }
            else
            {
                cout << "ret = nullptr" << endl;
            }

            constraint_relaxation_id ++;
        }

        reset();

        return ret;
    }
};


#endif //SURF_RANGEFILTERTEMPLATE_H
