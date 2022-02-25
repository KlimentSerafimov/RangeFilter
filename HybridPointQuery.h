//
// Created by kliment on 2/25/22.
//

#ifndef SURF_HYBRIDPOINTQUERY_H
#define SURF_HYBRIDPOINTQUERY_H

#include <string>
#include <assert.h>
#include "RangeFilterMultiBloom.h"

using namespace std;

class HybridPointQueryParams: public PointQueryParams
{
protected:
    size_t n;
    vector<string> splits;
    vector<PointQuery*> sub_point_query_params;

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
    virtual string to_string() const
    {
        assert(invariant());
        string ret;
        int max_w = 20;
        ret += "splits:\t";
        for(size_t i = 0;i<splits.size();i++)
        {
            cout << splits[i] <<"; ";
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

class HybridPointQuery: public PointQuery, public HybridPointQueryParams {
    virtual bool contains(string s)
    {
        assert(invariant());
        for(size_t i = 0;i<splits.size();i++) {
            if(splits[i] > s) {
                return sub_point_query_params[i]->contains(s);
            }
        }
        return sub_point_query_params[splits.size()]->contains(s);
    }

    virtual void insert(string s)
    {
        assert(invariant());
        for(size_t i = 0;i<splits.size();i++) {
            if(splits[i] > s) {
                sub_point_query_params[i]->insert(s);
            }
        }
        sub_point_query_params[splits.size()]->contains(s);
    }

    virtual unsigned long long get_memory() {
        assert(invariant());
        unsigned long long ret = 0;
        for(size_t i = 0;i<sub_point_query_params.size();i++)
        {
            ret+=sub_point_query_params[i]->get_memory();
        }
        for(size_t i =0;i<splits.size();i++)
        {
            ret+=splits[i].size()*sizeof(char);
        }
        return ret;
    }

    virtual void clear() {
        for(size_t i = 0;i<sub_point_query_params.size();i++) {
            sub_point_query_params[i]->clear();
        }
        sub_point_query_params.clear();
        for(size_t i =0;i<splits.size();i++) {
            splits[i].clear();
        }
        splits.clear();
    }
};


#endif //SURF_HYBRIDPOINTQUERY_H
