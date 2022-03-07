//
// Created by kliment on 2/25/22.
//

#include "HybridPointQuery.h"
#include "DatasetAndWorkload.h"

string HybridPointQueryParams::to_string() const
{
    assert(invariant());
    string ret;
    int max_w = 20;
    ret += "splits:\t";
    for(size_t i = 0;i<splits.size();i++)
    {
        ret += meta_data.translate_back(splits[i])+"; ";
    }
    ret += "\t";
    max_w = max(max_w, (int)ret.size());
    ret += "MULTIBLOOM PARAMS: ";
    for(size_t i = 0;i<sub_point_query_params.size();i++)
    {
        string str = sub_point_query_params[i]->to_string() +" | ";
        assert(str.find('\n') == std::string::npos);
        ret += str;
        max_w = max(max_w, (int)str.size());
    }
    string line;
    for(int i = 0;i<max_w;i++)
    {
        line += "-";
    }
//        line +="\n";
//        return line+ret+line;
    return ret;
}

HybridPointQueryParams::HybridPointQueryParams(const DatasetAndWorkloadMetaData &_meta_data) : PointQueryParams(), meta_data(_meta_data) {
    assert(meta_data.num_bits_per_char == 6 || meta_data.num_bits_per_char == -1);
}

HybridPointQueryParams::HybridPointQueryParams(const HybridPointQueryParams &to_copy) : n(to_copy.n), splits(to_copy.splits), meta_data(to_copy.meta_data) {
    assert(meta_data.num_bits_per_char == 6 || meta_data.num_bits_per_char == -1);
    for(size_t i = 0;i<sub_point_query_params.size();i++)
    {
        sub_point_query_params.push_back(sub_point_query_params[i]->clone_params());
    }
}
HybridPointQuery::HybridPointQuery(const DatasetAndWorkload &dataset_and_workload, const string &midpoint,
                                   int left_cutoff, float left_fpr, int right_cutoff, float right_fpr, bool do_print) :
                                   HybridPointQueryParams(dataset_and_workload.original_meta_data)
{
    n = 2;
    splits.push_back(midpoint);
    vector<string> left_dataset;
    vector<string> right_dataset;
    const vector<string>& dataset = dataset_and_workload.get_dataset();
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