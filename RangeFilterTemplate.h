//
// Created by kliment on 12/6/21.
//

#ifndef SURF_RANGEFILTERTEMPLATE_H
#define SURF_RANGEFILTERTEMPLATE_H

#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <cassert>

using namespace std;

class RangeFilterTemplateTemplate
{
public:
    virtual bool query(string left, string right)
    {
        assert(false);
    }

    virtual unsigned long long get_memory() {
        assert(false);
    }

    virtual void clear()
    {
        assert(false);
    }
};

template<typename PointQuery>
class RangeFilterTemplate: public RangeFilterTemplateTemplate
{
    PointQuery pq;
    char last_char{};
    char init_char{};
    char is_leaf_char{};
    int max_length{};
    long long total_num_chars{};

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
            cout << "init_char " << init_char << endl;
            cout << "last_char " << last_char << endl;
            cout << "last_char-init_char+1 " << last_char - init_char + 1 << endl;
            cout << "|unique_chars| " << num_unique_chars << endl;
            cout << "is_leaf_char " << is_leaf_char << endl;
        }
    }

    bool contains(string s)
    {
        return pq.contains(s);
    }
    void insert_prefixes(const vector<string>& dataset){
        long long num_inserted = 0;
        for (size_t i = 0; i < dataset.size(); ++i) {
            string prefix;
            for(size_t j = 0;j<dataset[i].size();j++) {
                prefix+=dataset[i][j];
                pq.insert(prefix);

                num_inserted += 1;
                if ((num_inserted) % 100000000 == 0) {
                    cout << "inserted(chars) " << num_inserted << "/" << total_num_chars << endl;
                }
            }
            prefix+=is_leaf_char;
            pq.insert(prefix);

            num_inserted += 1;
            if ((num_inserted) % 100000000 == 0) {
                cout << "inserted(chars) " << num_inserted << "/" << total_num_chars << endl;
            }
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

    RangeFilterTemplate(const vector<string>& dataset, const vector<pair<string, string> >& workload, double fpr, bool do_print = false)
    {
        calc_metadata(dataset, workload, do_print);
        pq = PointQuery(dataset, workload, fpr, do_print);
        insert_prefixes(dataset);
    }

    RangeFilterTemplate(const vector<string>& dataset, const vector<pair<string, string> >& workload, vector<pair<int, double> > params, bool do_print = false)
    {
        calc_metadata(dataset, workload, do_print);
        pq = PointQuery(dataset, workload, params, do_print);
        insert_prefixes(dataset);
    }


    bool query(string left, string right) override
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


    unsigned long long get_memory() override {
        return pq.get_memory() + 3*sizeof(char) + sizeof(int) + sizeof(long long);
    }

    void clear() override
    {
        pq.clear();
    }
};


#endif //SURF_RANGEFILTERTEMPLATE_H
