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

    bool negative_workload_defined = false;
    vector<pair<string, string> > negative_workload;

public:

    void process_str(const string& str)
    {
        max_length = max(max_length, dataset.size());
        for(auto c : str) {
            min_char = min(min_char, c);
            max_char = max(max_char, c);
        }
    }

    DatasetAndWorkload(const string& file_path, const string& workload_difficulty){

        prep_dataset_and_workload(file_path, workload_difficulty);

        for(const auto& it: dataset) {
            process_str(it);
        }
        for(const auto& it: workload) {
            process_str(it.first);
            process_str(it.second);
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



    RangeFilterStats test_range_filter(RangeFilterTemplate* rf, bool do_print);

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

};

#endif //SURF_DATASETANDWORKLOAD_H
