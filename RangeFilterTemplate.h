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
#include <map>
#include <fstream>

using namespace std;

class GroundTruthPointQuery: public PointQuery
{
public:
    Trie* trie;
    GroundTruthPointQuery(): trie(new Trie()) {}

    bool contains(string s) override
    {
        return trie->contains(s);
    }

    virtual void insert(string s)
    {
        trie->insert(s);
    }

    virtual unsigned long long get_memory() {
        return trie->get_memory();
    }

    virtual void clear()
    {
        trie->clear();
    }

};

class RangeFilterTemplate
{
    PointQuery* pq;
    char last_char{};
    char init_char{};
    char is_leaf_char{};
    int max_length{};
    long long total_num_chars{};

    int negative_point_queries = 0;

    void calc_metadata(const vector<string>& dataset, const vector<pair<string, string> >& workload, bool do_print)
    {
        last_char = (char)0;
        init_char = (char)127;
        max_length = 0;

        total_num_chars = 0;

        int char_count[127];
        memset(char_count, 0, sizeof(char_count));

        for(size_t i = 0;i<dataset.size();i++)
        {
            max_length = max(max_length, (int)dataset[i].size());
            total_num_chars+=(int)dataset[i].size();
            string prefix = "";
            for(size_t j = 0;j<dataset[i].size();j++)
            {
                prefix+=dataset[i][j];
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

        assert(init_char <= last_char);

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
            cout << "max_length " << max_length << endl;
            cout << "init_char " << init_char << endl;
            cout << "last_char " << last_char << endl;
            cout << "last_char-init_char+1 " << last_char - init_char + 1 << endl;
            cout << "|unique_chars| " << num_unique_chars << endl;
            cout << "is_leaf_char " << is_leaf_char << endl;
        }
    }

public:
    ~RangeFilterTemplate(){
        pq->clear();
        delete pq;
    }
    bool track_negative_point_queries = false;
private:
    map<string, int> prefix_to_negative_point_queries;

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
                if (prefix_to_negative_point_queries.find(sub_s) == prefix_to_negative_point_queries.end()) {
                    prefix_to_negative_point_queries[sub_s] = 0;
                }
                prefix_to_negative_point_queries[sub_s] += 1;
                negative_point_queries += 1;
            }
        }
        return ret;
    }
    bool str_invariant(string q) const
    {
        bool ret = true;
        for(int i = 0;i<(int)q.size();i++)
        {
            ret &= (init_char <= q[i]);
            ret &= (q[i] <= last_char);
            if(!ret)
            {
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

public:

    void insert_prefixes(const vector<string>& dataset){
        long long num_inserted = 0;
        for (size_t i = 0; i < dataset.size(); ++i) {
            string prefix;
            for(size_t j = 0;j<dataset[i].size();j++) {
                prefix+=dataset[i][j];
                pq->insert(prefix);

                num_inserted += 1;
                if ((num_inserted) % 1000000 == 0) {
                    cout << "inserted(chars) " << num_inserted << "/" << total_num_chars << endl;
                }
            }
            prefix+=is_leaf_char;
            pq->insert(prefix);

            num_inserted += 1;
            if ((num_inserted) % 1000000 == 0) {
                cout << "inserted(chars) " << num_inserted << "/" << total_num_chars << endl;
            }
            if ((i+1) % 100000 == 0) {
                cout << "inserted(strings) " << i+1 << "/" << dataset.size() << endl;
            }
        }
    }

    RangeFilterTemplate(const vector<string>& dataset, const vector<pair<string, string> >& workload, PointQuery* _pq, bool do_print = false):
    pq(_pq)
    {
        calc_metadata(dataset, workload, do_print);
        insert_prefixes(dataset);
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
                    for (char c = (char) ((int) left[left_id] + 1); c <= (char) ((int) last_char); c++) {
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
                    for (char c = (char) ((int) init_char); c <= (char) ((int) right[right_id] - 1); c++) {
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

    void clear()
    {
        pq->clear();
    }

    PointQuery* get_point_query()
    {
        return pq;
    }

    char get_is_leaf_char() const {
        return is_leaf_char;
    }

    int get_negative_point_queries() {
        return negative_point_queries;
    }

    static bool static_contains(const vector<string>& dataset, const string& left, const string& right)
    {
        auto at = lower_bound(dataset.begin(), dataset.end(), left);
        if(at == dataset.end())
        {
            return false;
        }
        if(*at > right)
        {
            return false;
        }

        return true;
    }


    vector<pair<string, string> > negative_workload;
    string analyze_negative_point_query_density_heatmap(const vector<string>& dataset, const vector<pair<string, string> >& workload) {
        assert(!track_negative_point_queries);

        for(size_t i = 0;i<workload.size();i++) {
            string left_key = workload[i].first;
            string right_key = workload[i].second;

            bool ret = query(left_key, right_key);
            assert(static_contains(dataset, left_key, right_key) == ret);

            if (!ret) {
                negative_workload.push_back(workload[i]);
            }
        }

        cout << "|negative_workload| = " << negative_workload.size() << endl;

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
            ends.emplace_back(negative_workload[i].second, negative_workload[i].first);
        }

        sort(inits.begin(), inits.end());
        sort(ends.begin(), ends.end());

        vector<pair<string, int> > array;
        int sum = 0;
        for(const auto& it: prefix_to_negative_point_queries) {
            array.emplace_back(it.first, it.second);
            sum+=it.second;
        }
        sort(array.begin(), array.end());
        int prefix_sum = 0;
        int at_init = 0;
        int at_end = 0;
        vector<pair<float, float> > densities;

        ofstream out("densities.out");

        pair<float, string> best_split = make_pair(-1, "");

        for(size_t i = 0;i<array.size();i++)
        {
            prefix_sum+=array[i].second;
            while(at_init < inits.size() && inits[at_init].first < array[i].first)
            {
                assert(inits[at_init].first != array[i].first);
                if(inits[at_init].second > array[i].first) {
                    assert(inits[at_init].second.size() > array[i].first.size());
                    assert(inits[at_init].second.substr(0, array[i].first.size()) == array[i].first);
                }
                at_init+=1;
            }
            while(at_end < ends.size() && ((ends[at_end].first < array[i].first) ||
                    (ends[at_end].first > array[i].first && (
                       ends[at_end].first.size() > array[i].first.size() &&
                       ends[at_end].first.substr(0, array[i].first.size()) == array[i].first))))
            {
                at_end+=1;
            }
            if(at_end < ends.size()) {
                assert(inits[at_end].first != array[i].first);
                assert(ends[at_end].second > array[i].first);
            }

            if((inits.size()-at_init-1) == 0)
            {
                continue;
            }

            float left_density = (float)prefix_sum/(at_init+1);
            float right_density = (float)(sum-prefix_sum)/(inits.size()-at_init-1);


            densities.emplace_back(left_density,right_density);
//            cout <<at_init <<" "<< left_density <<" "<< right_density << endl;


            if(right_density == 0 || left_density == 0)
            {
                continue;
            }

            float score = max(right_density/left_density, left_density/right_density);
            out << at_init << " "<< score << endl;

            if(at_init < inits.size())
            best_split = max(best_split, make_pair(score, inits[at_init].second));



        }
        out.close();

        cout << best_split.first <<" "<< best_split.second << endl;

        return best_split.second;
    }
};


#endif //SURF_RANGEFILTERTEMPLATE_H
