//
// Created by kliment on 2/25/22.
//

#ifndef SURF_HYBRIDPOINTQUERY_H
#define SURF_HYBRIDPOINTQUERY_H

#include <string>
#include <cassert>
#include "RangeFilterMultiBloom.h"

using namespace std;

class HybridPointQueryParams: public PointQueryParams
{
protected:
    size_t n = 0;
    vector<string> splits;
    vector<PointQueryParams*> sub_point_query_params;

    bool invariant() const {
        assert(n >= 1);
        assert(splits.size() == n-1);
        assert(sub_point_query_params.size() == n);
        for(size_t i = 1;i<splits.size();i++) {
            assert(splits[i] >= splits[i-1]);
        }
        return true;
    }

public:
    HybridPointQueryParams(): PointQueryParams() {}
    HybridPointQueryParams(const HybridPointQueryParams& to_copy): n(to_copy.n), splits(to_copy.splits) {
        for(size_t i = 0;i<sub_point_query_params.size();i++)
        {
            sub_point_query_params.push_back(sub_point_query_params[i]->clone());
        }
    }
    virtual HybridPointQueryParams* clone() const override
    {
        return new HybridPointQueryParams(*this);
    }
    string to_string() const override
    {
        assert(invariant());
        string ret;
        int max_w = 20;
        ret += "splits:\t";
        for(size_t i = 0;i<splits.size();i++)
        {
            ret += splits[i]+"; ";
        }
        ret += "\n";
        max_w = max(max_w, (int)ret.size());
        ret += "MULTIBLOOM PARAMS: \n";
        for(size_t i = 0;i<sub_point_query_params.size();i++)
        {
            string str = sub_point_query_params[i]->to_string() +"\n";
            ret += str;
            max_w = max(max_w, (int)str.size());
        }
        string line;
        for(int i = 0;i<max_w;i++)
        {
            line += "-";
        }
        line +="\n";
        return line+ret+line;
    }
};

class HybridPointQuery: public HybridPointQueryParams, virtual public PointQuery {

    vector<PointQuery*> sub_point_query;
public:
    HybridPointQuery(const vector<string>& dataset, const string& midpoint, int left_cutoff, float left_fpr, int right_cutoff, float right_fpr, bool do_print):
           HybridPointQueryParams()
    {
        n = 2;
        splits.push_back(midpoint);
        vector<string> left_dataset;
        vector<string> right_dataset;
        for(size_t i = 0;i<dataset.size();i++)
        {
            if(dataset[i] < midpoint) {
                left_dataset.push_back(dataset[i]);
            }
            else {
                for(size_t j = dataset[i].size()-1;j>=1;j--)
                {
                    if(dataset[i].substr(0, j) < midpoint)
                    {
                        left_dataset.push_back(dataset[i].substr(0, j));
                        break;
                    }
                }
                right_dataset.push_back(dataset[i]);
            }
        }
        if(do_print) {
            cout << "LEFT MULTI-BLOOM:" << endl;
        }
        sub_point_query.push_back(new MultiBloom(left_dataset, left_fpr, left_cutoff, do_print));
        sub_point_query_params.push_back(*sub_point_query.rbegin());
        if(do_print) {
            cout << "RIGHT MULTI-BLOOM" << endl;
        }
        sub_point_query.push_back(new MultiBloom(right_dataset, right_fpr, right_cutoff, do_print));
        sub_point_query_params.push_back(*sub_point_query.rbegin());
        if(do_print) {
            cout << "DONE HYBRID CONSTRUCTION." << endl;
        }
        assert(invariant());
    }

    HybridPointQueryParams* clone() const override{
        return HybridPointQueryParams::clone();
    }

    bool contains(string s) override
    {
        assert(invariant());
        bool found = false;
        bool ret = false;
        for(size_t i = 0;i<splits.size();i++) {
            if(splits[i] > s) {
                ret = sub_point_query[i]->contains(s);
                found = true;
                break;
            }
        }
        if(!found)
        {
            ret = sub_point_query[splits.size()]->contains(s);
            found = true;
        }
        assert(found);
        return ret;
    }

    void insert(string s) override
    {
        assert(invariant());
        for(size_t i = 0;i<splits.size();i++) {
            if(splits[i] > s) {
                sub_point_query[i]->insert(s);
                return;
            }
        }
        sub_point_query[splits.size()]->insert(s);
    }

    unsigned long long get_memory() override {
        assert(invariant());
        unsigned long long ret = 0;
        for(size_t i = 0;i<sub_point_query_params.size();i++)
        {
            ret+=sub_point_query[i]->get_memory();
        }
        for(size_t i =0;i<splits.size();i++)
        {
            ret+=splits[i].size()*sizeof(char);
        }
        return ret;
    }

    void clear() override {
        for(size_t i = 0;i<sub_point_query_params.size();i++) {
            sub_point_query[i]->clear();
        }
        sub_point_query_params.clear();
        for(size_t i =0;i<splits.size();i++) {
            splits[i].clear();
        }
        splits.clear();
    }

    string to_string() const override
    {
        return HybridPointQueryParams::to_string();
    }
};


#endif //SURF_HYBRIDPOINTQUERY_H
