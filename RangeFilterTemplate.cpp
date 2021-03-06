//
// Created by kliment on 2/26/22.
//

#include "RangeFilterTemplate.h"

#include <utility>
#include "DatasetAndWorkload.h"


void RangeFilterTemplate::calc_metadata(const DatasetAndWorkload &dataset_and_workload, bool do_print) {

    max_char = dataset_and_workload.get_max_char();
    min_char = dataset_and_workload.get_min_char();
    no_char = (char)((int)min_char-1);
    max_length = dataset_and_workload.get_max_length()+1;//+1 for the is_leaf_char at the end.

    is_leaf_char = (char) ((int) max_char + 1);
    assert((int) is_leaf_char <= 127);

    if (do_print) {
        cout << "RANGE FILTER STATS" << endl;
        cout << "num_strings " << (int) dataset_and_workload.get_dataset().size() << endl;
        cout << "max_length " << max_length << endl;
        cout << "min_char " << min_char << endl;
        cout << "max_char " << max_char << endl;
        cout << "max_char-min_char+1 " << max_char - min_char + 1 << endl;
        cout << "is_leaf_char " << is_leaf_char << endl;
    }
}

RangeFilterTemplate::RangeFilterTemplate(const DatasetAndWorkload &dataset_and_workload, PointQuery *_pq,
                                         bool do_print):
        pq(_pq)
{
    calc_metadata(dataset_and_workload, do_print);
    insert_prefixes(dataset_and_workload.get_dataset());
}

class MyString: public string
{
    bool cleared = false;
public:
    MyString(string str): string(std::move(str)) {}
    string to_string() const {
        return *this;
    }
    void clear() {
        assert(!cleared);
        string::clear();
        cleared = true;
    }
};

pair<double, string> *
RangeFilterTemplate::analyze_negative_point_query_density_heatmap(const DatasetAndWorkload &dataset_and_workload, bool do_print) const {
    build_heatmap(dataset_and_workload);
    assert(track_negative_point_queries);

    const Dataset& dataset = dataset_and_workload.get_dataset();
    const Workload& negative_workload = dataset_and_workload.get_negative_workload_assert_has();

    Workload inits;

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

    ofstream out;
    if(do_print)
    {
        out = ofstream("densities.out");
    }

    pair<float, string> best_split = make_pair(-1, "");

    size_t at_arr = 0;

    Frontier<MyString> split_size_vs_density_ratio_frontier(2);

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

    if(!enter){
        reset();
        return nullptr;
    }

    enter = false;

    for(int row_id = dataset.size()-1; row_id >= 0;row_id--)
    {
        if(dataset[row_id] < inits[inits.size()-1].second) {
            end_row_id = row_id;
            enter = true;
            break;
        }
    }

    if(!enter) {
        reset();
        return nullptr;
    }

    for(size_t row_id = init_row_id; row_id <= end_row_id; row_id++) {
        while(at_init < inits.size() && inits[at_init].first < dataset[row_id]) {
            assert(inits[at_init].first <= inits[at_init].second);
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

        float score = max(right_density/left_density, left_density/right_density);
        if(do_print) {
            out << row_id << " " << score << " left_density = " << prefix_sum << " / " << at_init << " = "
                << left_density << " | right_density = " << (sum - prefix_sum) << "/" << (inits.size() - at_init)
                << " = " << right_density << " | ratios: " << left_density / right_density << " "
                << right_density / left_density << endl;
        }

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

        split_size_vs_density_ratio_frontier.insert(MyString(dataset[row_id]), multi_d_score);
//        assert(split_size_vs_density_ratio_frontier.is_sorted());
    }
    if(do_print) {
        out.close();
    }

    if(do_print) {
        cout << "best based on density ratio: " << best_split.first << " " << best_split.second << endl;
        split_size_vs_density_ratio_frontier.print(cout, 10, true);
    }

//    assert(split_size_vs_density_ratio_frontier.is_sorted());

    int constraint_relaxation_id = 1;

    pair<double, string>* ret = nullptr;

    while(ret == nullptr && constraint_relaxation_id <= 3) {
        vector<double> constraint;
        constraint.push_back(-1 - 0.06*constraint_relaxation_id);
        constraint.push_back(1.0 - 0.4/(1<<constraint_relaxation_id));
//        constraint.push_back(-1 - 0.06*constraint_relaxation_id);
//        constraint.push_back(1.0 - 0.02/(1<<constraint_relaxation_id));

        auto optimization_function = [](vector<double> in) {
            return - in[0] * in[0] * (1 - in[1]);
        };

        pair<vector<double>, MyString> *almost_ret = split_size_vs_density_ratio_frontier.get_best_better_than(
                constraint, optimization_function);

        if (almost_ret != nullptr) {
            ret = new pair<double, string>(-almost_ret->first[0], almost_ret->second);
        }
        if(do_print) {
            cout << "constrain_relaxation_id " << constraint_relaxation_id << endl;
            if (ret != nullptr) {
                cout << "ret: " << ret->first << " " << ret->second << " size_ratio: " << almost_ret->first[1] << endl;
            } else {
                cout << "ret = nullptr" << endl;
            }
        }
        constraint_relaxation_id ++;
    }

    reset();

    return ret;
}

void RangeFilterTemplate::build_heatmap(const DatasetAndWorkload &dataset_and_workload) const {
    assert(!track_negative_point_queries);
    assert(num_negative_point_queries == 0);
    assert(prefix_to_negative_point_queries.empty());
//    assert(negative_point_queries.empty());

    const Workload& negative_workload = dataset_and_workload.get_negative_workload_assert_has();

    track_negative_point_queries = true;

    for(size_t i = 0;i<negative_workload.size();i++) {
        auto it = negative_workload[i];
        assert(query(it.first, it.second) == false);
    }

    assert(track_negative_point_queries);
}


void RangeFilterTemplate::set_point_query(PointQuery *new_pq) {
    pq = new_pq;
}

size_t RangeFilterTemplate::get_cutoff() {
    assert(!is_cold());
    assert(track_negative_point_queries);
    assert(num_negative_point_queries >= 1);
    assert(!prefix_to_negative_point_queries.empty());
    //assert(!negative_point_queries.empty());

    size_t ret = 0;

    for(const auto& it: prefix_to_negative_point_queries) {
        ret = max(ret, it.first.size());
    }

    return ret;
}

size_t RangeFilterTemplate::get_unique_negative_point_queries() {
    assert(track_negative_point_queries);
    assert(num_negative_point_queries >= 1);
    assert(!prefix_to_negative_point_queries.empty());
    //assert(!negative_point_queries.empty());

    return prefix_to_negative_point_queries.size();
}

bool RangeFilterTemplate::is_cold() const {
    if(!track_negative_point_queries) {
        assert(num_negative_point_queries == 0);
        assert(prefix_to_negative_point_queries.empty());
//        assert(negative_point_queries.empty());
    }
    return !track_negative_point_queries;
}
