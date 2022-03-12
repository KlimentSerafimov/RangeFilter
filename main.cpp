#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <random>
#include <cstring>

#include "RangeFilterOneBloom.h"
#include "RangeFilterMultiBloom.h"
#include "SuRF/surf_implementation.h"
#include "Trie.h"
#include "HybridPointQuery.h"
#include "DatasetAndWorkload.h"

using namespace std;

void eval_trie_vs_rf()
{
    vector<string> dataset;
    dataset.emplace_back("aaabbb");
    dataset.emplace_back("aaaqqq");
    dataset.emplace_back("aaazzz");
    dataset.emplace_back("aaaff");
    dataset.emplace_back("ccc");

    sort(dataset.begin(), dataset.end());


    Trie trie(dataset);

    vector<pair<int, double> > vec;
    vec.reserve(8);
    for(int i = 0; i<8; i++)
    {
        vec.emplace_back(20, 0.0001);
    }


    vector<pair<string, string> > workload;
    workload.emplace_back(make_pair("aaaqqr", "aaaqqz"));
    workload.emplace_back(make_pair("aaaa", "aaab"));
    workload.emplace_back(make_pair("aaaa", "aaac"));
    workload.emplace_back(make_pair("aaabbb", "aaabbb"));
    workload.emplace_back(make_pair("aaabbc", "aaaqqp"));
    workload.emplace_back(make_pair("aaaqqp", "aaaqqq"));
    workload.emplace_back(make_pair("aaaqqq", "aaaqqq"));
    workload.emplace_back(make_pair("aaaqqq", "aaaqqr"));
    workload.emplace_back(make_pair("aaaqqp", "aaaqqr"));
    workload.emplace_back(make_pair("aaaqqr", "aaaqqy"));
    workload.emplace_back(make_pair("aaaqqr", "aaazzz"));
    workload.emplace_back(make_pair("aaaqqr", "aaazzy"));
    workload.emplace_back(make_pair("aaaqqr", "aaazyz"));
    workload.emplace_back(make_pair("aaaqqr", "aaayzz"));
    workload.emplace_back(make_pair("aaaqqr", "aaayzz"));
    workload.emplace_back(make_pair("aa", "ab"));
    workload.emplace_back(make_pair("a", "aaabba"));
    workload.emplace_back(make_pair("a", "aaabbb"));
    workload.emplace_back(make_pair("a", "aaazzz"));
    workload.emplace_back(make_pair("aaazzz", "aaazzz"));
    workload.emplace_back(make_pair("aaabbc", "aaabbc"));
    workload.emplace_back(make_pair("aaaab", "aaaz"));
    workload.emplace_back(make_pair("aaac", "aaaz"));
    workload.emplace_back(make_pair("aac", "aacz"));
    workload.emplace_back(make_pair("aaabbba", "aaaqqp"));
    workload.emplace_back(make_pair("aaaqqp", "aaaqqqa"));
    workload.emplace_back(make_pair("aaaff", "aaaff"));
    workload.emplace_back(make_pair("cbb", "ddd"));
    workload.emplace_back(make_pair("bbb", "cdd"));

    bool do_print = false;

//    auto* multi_bloom = new MultiBloom(dataset, 0.0001, -1, do_print);
//    RangeFilterTemplate* rf = new RangeFilterTemplate(dataset_and_workload, multi_bloom, do_print);

//    auto* pq = new HybridPointQuery(dataset, "aaazzz", -1, 0.001, -1, 0.001, do_print);
//    auto *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);

    DatasetAndWorkload dataset_and_workload(dataset, workload);

    PointQuery *ground_truth_point_query = new GroundTruthPointQuery();
    RangeFilterTemplate* rf = new RangeFilterTemplate(dataset_and_workload, ground_truth_point_query, do_print);

    vector<pair<string, string> > negative_workload;

    for(size_t i = 0;i<workload.size();i++) {
        string left_key = workload[i].first;
        string right_key = workload[i].second;

        bool ret = rf->query(left_key, right_key);
        assert(contains(dataset, left_key, right_key) == ret);

        if (!ret) {
            negative_workload.push_back(workload[i]);
        }
    }

    cout << "|negative_workload| = " << negative_workload.size() << endl;

    DatasetAndWorkload local_dataset_and_workload(dataset_and_workload.get_dataset(), dataset_and_workload.get_negative_workload());

    string best_split = rf->analyze_negative_point_query_density_heatmap(local_dataset_and_workload)->second;


    cout << "start eval" << endl;
    for(const auto& q: workload)
    {
        cout << q.first <<" "<< q.second << endl;
        bool trie_out = trie.query(q.first, q.second);
        bool rf_out = rf->query(q.first, q.second);
        cout <<trie_out <<" "<< rf_out << endl;
        if(trie_out == rf_out)
        {
            cout << "OK" << endl;
        }
        else
        {
//            cout << "WRONG" << endl;
            if(trie_out)
            {
                cout << "FALSE NEGATIVE" << endl;
            }
            else
            {
                cout << "FALSE POSITIVE" << endl;
            }
            assert(false);
        }
    }

    cout << "done" << endl;

}

void extract_dataset(const string& file_path, const string& dest_path, int num_keys)
{
    vector<string> dataset = read_dataset(file_path);

    shuffle(dataset.begin(), dataset.end(), std::default_random_engine(0));

    vector<string> subdataset;
    subdataset.reserve((int)num_keys);
    for (int i = 0; i < (int)num_keys; i++) {
        subdataset.push_back(dataset[i]);
    }

    sort(subdataset.begin(), subdataset.end());

    ofstream out_file(dest_path);
    for (int i = 0; i < (int)subdataset.size(); i++) {
        out_file << subdataset[i] << endl;
    }

    out_file.close();
}

class MyInt
{
    int x;
public:
    explicit MyInt(int _x): x(_x)
    {
    }
    string to_string() const
    {
        return std::to_string(x);
    }
    void clear()
    {
    }
};

#include "SurfPointQuery.h"
#include "HybridRangeFilterSynthesizer.h"

int main_test_surf(DatasetAndWorkload& dataset_and_workload)
{
    Frontier<MyInt> frontier(2);
    for(int trie_size = 1; trie_size <= 64; trie_size+=1) {

        SurfPointQuery* surf_pq = new SurfPointQuery(dataset_and_workload.get_dataset(), trie_size);
        RangeFilterTemplate* surf_rf = new RangeFilterTemplate(dataset_and_workload, surf_pq);
        const RangeFilterScore& rez = *dataset_and_workload.test_range_filter(surf_rf);
        cout << rez.to_string() << endl;
        frontier.insert(MyInt(trie_size), rez.get_score_as_vector());
    }

    ofstream surf_out("surf_frontier.out");
    frontier.print(surf_out);
    surf_out.close();

    return 0;
}

void grid_search(DatasetAndWorkload& dataset_and_workload, const string& range_filter_type, ofstream& output_file);

#include <queue>

void eval_rf_heatmap(DatasetAndWorkload& dataset_and_workload)
{
    PointQuery *ground_truth_point_query = new GroundTruthPointQuery();
    RangeFilterTemplate ground_truth = RangeFilterTemplate(dataset_and_workload, ground_truth_point_query, false);

    priority_queue<pair<size_t, vector<pair<string, string> > > > workloads;

    vector<pair<string, string> > negative_workload = dataset_and_workload.get_negative_workload();

    workloads.push(make_pair(negative_workload.size(), negative_workload));

    while(!workloads.empty()) {
        size_t sz = workloads.top().first;
        assert(sz >= 2);
        cout << "SZ: " << sz << endl;
        vector<pair<string, string> > local_workload = workloads.top().second;
        assert(local_workload.size() == sz);
        workloads.pop();

        DatasetAndWorkload local_dataset_and_workload(dataset_and_workload.get_dataset(), local_workload);

        pair<double, string>* _ratio_and_best_split = ground_truth.analyze_negative_point_query_density_heatmap(local_dataset_and_workload);
        if(_ratio_and_best_split == nullptr)
        {
            cout << "UNIFORM size = " << local_workload.size() << endl;
            continue;
        }

        pair<double, string> ratio_and_best_split = *_ratio_and_best_split;

//        float ratio = ratio_and_best_split.first;
        string best_split = ratio_and_best_split.second;

        cout << endl;

        assert(local_workload.size() > 1);

        vector<pair<string, string> > left_workload;
        vector<pair<string, string> > right_workload;

        for (size_t i = 0; i < local_workload.size(); i++) {
            if (local_workload[i].second <= best_split) {
                left_workload.push_back(local_workload[i]);
            } else {
                right_workload.push_back(local_workload[i]);
            }
        }

        cout << left_workload.size() << endl;
        cout << right_workload.size() << endl;

        if(left_workload.size() >= 2) {
            workloads.push(make_pair(left_workload.size(), left_workload));
        }
        if(right_workload.size() >= 2) {
            workloads.push(make_pair(right_workload.size(), right_workload));
        }
    }
}

void meta_dataset_construction()
{
    string file_folder;
    string file_name = "1M_dataset.txt";
    string workload_difficulty = "medium"; //choose from "easy", "medium", "hard", "impossible", hybrid

    string file_path = file_folder + file_name;

    DatasetAndWorkload dataset_and_workload(file_path, workload_difficulty, true);
    dataset_and_workload.get_negative_workload();

    GroundTruthPointQuery *original_ground_truth_point_query = new GroundTruthPointQuery();
    RangeFilterTemplate original_ground_truth =
            RangeFilterTemplate(dataset_and_workload, original_ground_truth_point_query, false);

    original_ground_truth.analyze_negative_point_query_density_heatmap(dataset_and_workload);

    cout << original_ground_truth.get_negative_point_queries()/dataset_and_workload.get_negative_workload_assert_has().size() << endl;

}

int main() {

    srand(0);

//    micro unit tests
//    eval_trie_vs_rf();
//    return 0;

//    meta_dataset_construction();
//
//    return 0;

    string file_folder;
    string file_name = "50k_dataset.txt";
    string workload_difficulty = "hybrid"; //choose from "easy", "medium", "hard", "impossible", hybrid
    string range_filter_type = "hybrid"; // choose from "heatmap", "trie", "surf", "one_bloom", "multi_bloom", "hybrid"
    string parameter_search_style = "dt_style"; // choose from "no_search", "grid_search", "simulated_annealing", "dt_style"

    string file_path = file_folder + file_name;

    bool do_extract_dataset = false;
    if (do_extract_dataset) {
        extract_dataset(file_path, "6_dataset.txt", 6);
        return 0;
    }

    DatasetAndWorkload dataset_and_workload(file_path, workload_difficulty, true);

    bool debug = false;
    if(debug) {
        DatasetAndWorkload original_dataset_and_workload(file_path, workload_difficulty, false);

        assert(dataset_and_workload.get_workload().size() == original_dataset_and_workload.get_workload().size());
        assert(dataset_and_workload.get_dataset().size() == original_dataset_and_workload.get_dataset().size());


        GroundTruthPointQuery *original_ground_truth_point_query = new GroundTruthPointQuery();
        RangeFilterTemplate original_ground_truth =
                RangeFilterTemplate(original_dataset_and_workload, original_ground_truth_point_query, false);

        GroundTruthPointQuery *ground_truth_point_query = new GroundTruthPointQuery();
        RangeFilterTemplate ground_truth = RangeFilterTemplate(dataset_and_workload, ground_truth_point_query, false);

        int workload_size = dataset_and_workload.get_workload().size();

        for (int i = 0; i < workload_size; i++) {
            assert(!original_dataset_and_workload.get_workload()[i].first.empty() &&
                   !original_dataset_and_workload.get_workload()[i].second.empty());
            bool original_rez = contains(
                    original_dataset_and_workload.get_dataset(),
                    original_dataset_and_workload.get_workload()[i].first,
                    original_dataset_and_workload.get_workload()[i].second);

            bool translated_rez = contains(
                    dataset_and_workload.get_dataset(),
                    dataset_and_workload.get_workload()[i].first,
                    dataset_and_workload.get_workload()[i].second);

            assert(original_rez == translated_rez);


            bool original_rf_rez = original_ground_truth.query(original_dataset_and_workload.get_workload()[i].first,
                                                               original_dataset_and_workload.get_workload()[i].second);

            assert(original_rf_rez == original_rez);

            bool rf_rez = ground_truth.query(dataset_and_workload.get_workload()[i].first,
                                             dataset_and_workload.get_workload()[i].second);


            if (rf_rez != original_rez) {
                ground_truth_point_query->trie->do_breakpoint = true;

                ground_truth.query(dataset_and_workload.get_workload()[i].first,
                                   dataset_and_workload.get_workload()[i].second);
            }

            assert(rf_rez == original_rez);
        }
    }

    if(parameter_search_style == "dt_style") {

        assert (range_filter_type == "hybrid");
        {

            DatasetAndWorkload local_dataset_and_workload(dataset_and_workload.get_dataset(),
                                                          dataset_and_workload.get_negative_workload(),
                                                          dataset_and_workload);

            PointQuery *ground_truth_point_query = new GroundTruthPointQuery();
            RangeFilterTemplate ground_truth = RangeFilterTemplate(dataset_and_workload, ground_truth_point_query,
                                                                   false);

            string base_case_filter = "surf;one_bloom" ; // choose from "surf;one_bloom", "surf", "multi_bloom", "one_bloom";

//            for(size_t base_case_size = 20; base_case_size <= 248;
//                base_case_size =
//                        min(
//                                min(
//                                        base_case_size*2,
//                                        base_case_size+dataset_and_workload.get_workload().size()/20),
//                                dataset_and_workload.get_workload().size())
//                )
            int base_case_size = 50; {

                local_dataset_and_workload.get_negative_workload();
                Frontier<HybridRangeFilterSynthesizer::PointQueryPointer> *ret =
                        HybridRangeFilterSynthesizer::construct_hybrid_point_query(
                                local_dataset_and_workload,
                                ground_truth, base_case_filter,
                                base_case_size,
                                vector<string>(1, "root")).first;

                ofstream dt_out("tree_of_hybrid_hybrid50k_base" + std::to_string(base_case_size) + "_second__" + base_case_filter + ".out");

                ret->print(dt_out);

                dt_out.close();
            }

            return 0;
        }
    }

    if (range_filter_type == "surf") {
        assert(parameter_search_style == "grid_search");
        main_test_surf(dataset_and_workload);
        return 0;
    }

    if(range_filter_type == "heatmap")
    {
        assert(parameter_search_style == "no_search");
        eval_rf_heatmap(dataset_and_workload);
        return 0;
    }
//    string file_folder = "/home/kliment/Downloads/";
//    string file_name = "emails-validated-random-only-30-characters.txt.sorted";

    if(parameter_search_style == "grid_search") {
        ofstream output_file = ofstream ("results__grid_search.out");
        grid_search(dataset_and_workload, range_filter_type, output_file);
        output_file.close();
    }
    else {
        assert(range_filter_type == "multi_bloom");
        HybridRangeFilterSynthesizer::optimize_base_case_with_multi_bloom(dataset_and_workload);
    }


    return 0;
}

#include "Frontier.h"

void grid_search(DatasetAndWorkload& dataset_and_workload, const string& _range_filter_type, ofstream& output_file){

    const vector<string>& dataset = dataset_and_workload.get_dataset();

    string range_filter_type = _range_filter_type;

    range_filter_type = "hybrid";

    if(range_filter_type == "hybrid")
    {
        int _sample_id = 0;
        int samples = 100 ;
        int space = 20*20*24*24;
        float ratio = (float)samples/(float)space;
        int samples_taken = 0;
        Frontier<HybridPointQueryParams> frontier(2);
        Frontier<MultiBloomParams> multi_bloom_frontier(2);
        for(int left_cutoff = 1; left_cutoff <= 20; left_cutoff++) {//20
            for(float left_fpr = 0.001; left_fpr < 0.6; left_fpr = min(left_fpr+0.05, left_fpr*1.5)) {//6

                bool do_print = false;

                for(int right_cutoff = 1; right_cutoff <= 20; right_cutoff++) {//20
                    for (float right_fpr = 0.001; right_fpr < 0.6; right_fpr = min(right_fpr+0.05, right_fpr * 1.5)) { //6
//                        cout << "ID: " << id++ << endl;
                        _sample_id++;
//                        cout << left_cutoff << " "<< right_cutoff <<" "<< left_fpr <<" "<< right_fpr << endl;
                        if(rand()%1000 <= ratio*1000)
                        {
                            cout << "#SAMPLES: " << samples_taken++ << endl;
                        }
                        else
                        {
                            continue;
                        }
                        PointQuery *pq;
                        pq = new HybridPointQuery(dataset_and_workload, "kirk", left_cutoff, left_fpr, right_cutoff, right_fpr, do_print);
                        auto *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);
                        const RangeFilterScore& ret = *dataset_and_workload.test_range_filter(rf, do_print);
//                        HybridPointQueryParams *params = (HybridPointQueryParams *) ret.get_params();
                        HybridPointQuery& local_tmp_rmb_pq = *((HybridPointQuery*)ret.get_params());
                        frontier.insert(local_tmp_rmb_pq, ret.get_score_as_vector());
                        output_file << ret.to_string() << endl;
                        cout << ret.to_string() << endl;
                        cout << "DONE" << endl;

                        {
                            PointQuery *mb_pq = new MultiBloom(dataset, right_fpr, right_cutoff, do_print);
                            auto *multi_bloom_rf = new RangeFilterTemplate(dataset_and_workload, mb_pq, do_print);
                            const RangeFilterScore& mb_ret = *dataset_and_workload.test_range_filter(multi_bloom_rf, do_print);
                            MultiBloomParams *mb_params = (MultiBloomParams *) mb_ret.get_params();
                            multi_bloom_frontier.insert(*mb_params, mb_ret.get_score_as_vector());
                        }
                        {
                            PointQuery *mb_pq = new MultiBloom(dataset, left_fpr, left_cutoff, do_print);
                            auto *multi_bloom_rf = new RangeFilterTemplate(dataset_and_workload, mb_pq, do_print);
                            const RangeFilterScore& mb_ret = *dataset_and_workload.test_range_filter(multi_bloom_rf, do_print);
                            MultiBloomParams *mb_params = (MultiBloomParams *) mb_ret.get_params();
                            multi_bloom_frontier.insert(*mb_params, mb_ret.get_score_as_vector());
                        }
                    }
                }
            }
        }
        ofstream frontier_output = ofstream ("hybrid_on_hybrid.txt");
        frontier.print(frontier_output);
        frontier_output.close();

        ofstream multi_bloom_frontier_output = ofstream ("multi_bloom_on_hybrid.txt");
        multi_bloom_frontier.print(multi_bloom_frontier_output);
        multi_bloom_frontier_output.close();
    }
    else {
        Frontier<MultiBloomParams> frontier(2);
        for (int cutoff = 0; cutoff < 20; cutoff++) {

            const int num_seed_fprs = 10;
            float seed_fprs[num_seed_fprs] = {
//                0.0000001,
//                0.000001,
//                0.00001,
//                0.0001,
                    0.001,
                    0.005,
                    0.01,
                    0.05,
                    0.1,
                    0.2,
                    0.3,
                    0.4,
                    0.5,
                    0.6,
//                0.7,
//                0.8,
//                0.9,
            };
            for (int seed_fpr_id = 0; seed_fpr_id < num_seed_fprs; seed_fpr_id++) {
                PointQuery *pq;
                bool do_print = false;
                if (range_filter_type == "one_bloom") {
                    pq = new OneBloom(dataset, seed_fprs[seed_fpr_id], cutoff, do_print);
                } else if (range_filter_type == "multi_bloom") {
                    pq = new MultiBloom(dataset, seed_fprs[seed_fpr_id], cutoff, do_print);
                } else {
                    assert(false);
                }
                auto *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);
                const RangeFilterScore& ret = *dataset_and_workload.test_range_filter(rf, do_print);

                if(range_filter_type == "multi_bloom") {
                    MultiBloomParams *params = (MultiBloomParams *)ret.get_params();
                    frontier.insert(*params, ret.get_score_as_vector());
                }
                delete rf;
                pq->_clear();
                output_file << ret.to_string() << endl;
                cout << ret.to_string() << endl;
                if (ret.false_positive_rate() > 0.5) {
                    output_file << "fpr too large; break;" << endl;
                    cout << "fpr too large; break;" << endl;
                    break;
                }
            }
            output_file << endl;
            cout << endl;
        }

        ofstream frontier_output = ofstream ("multi_bloom_on_hybrid.txt");
        frontier.print(frontier_output);
        frontier_output.close();
    }

}
