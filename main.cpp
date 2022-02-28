#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <random>
#include <cstring>

#include "RangeFilterOneBloom.h"
#include "RangeFilterMultiBloom.h"
#include "surf/surf_implementation.h"
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

void extract_dataset(string file_path, string dest_path, int num_keys)
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

int get_num_negatives(vector<string>& dataset, vector<pair<string, string> >& workload)
{
    int num_positive = 0;
    int num_negative = 0;
    for(size_t i = 0;i<workload.size();i++)
    {
        string left_key = workload[i].first;
        string right_key = workload[i].second;

        bool ground_truth = contains(dataset, left_key, right_key);

        if(ground_truth)
        {
            num_positive+=1;
        }
        else
        {
            num_negative+=1;
        }
    }
    return num_negative;
}

int main_test_surf(DatasetAndWorkload& dataset_and_workload)
{

    Frontier<int> frontier(2);
    for(int trie_size = 1; trie_size <= 55; trie_size+=1) {
        SurfStats surf_ret = test_surf(dataset_and_workload.get_dataset(), dataset_and_workload.get_workload(), trie_size);

        cout << surf_ret.to_string() << endl;
        frontier.insert(trie_size, surf_ret.get_score_as_vector());
    }

    ofstream surf_out("surf_frontier.out");
    frontier.print(surf_out);
    surf_out.close();

    return 0;
}
void grid_search(DatasetAndWorkload& dataset_and_workload, const string& range_filter_type, ofstream& output_file);
void simulated_annealing(DatasetAndWorkload& dataset_and_workload, ofstream& output_file);

#include <queue>

Frontier<PointQuery*>* optimize_base_case(DatasetAndWorkload& dataset_and_workload)
{
    cout << "ENTERED optimize_base_case " << dataset_and_workload.get_dataset().size() <<" "<< dataset_and_workload.get_workload().size() << endl;
    Frontier<PointQuery*>* ret = new Frontier<PointQuery*>(2);


    int id = 0;
    size_t space = dataset_and_workload.get_max_length()*(36);
    int samples = 100;
    float ratio = (float)samples/space;
    int samples_taken = 0;

    for(size_t cutoff = 1; cutoff < dataset_and_workload.get_max_length(); cutoff++) {
        for(float fpr = 0.0001; fpr < 1; fpr = min(fpr+0.05, fpr*1.5)) {
            id++;
            if(rand()%1000 < ratio*1000){
                samples_taken++;
            }
            else {
                continue;
            }
//            cout << "evaluating (sample " << samples_taken << ", id " << id << "): " << cutoff <<" " << fpr << endl;
            OneBloom *pq = new OneBloom(dataset_and_workload.get_dataset(), fpr, cutoff);
            RangeFilterStats rez = dataset_and_workload.eval_point_query(pq);
//            cout << "rez = " << rez.get_score_as_vector()[0] <<" " << rez.get_score_as_vector()[1] << endl;
            ret->insert(pq, rez.get_score_as_vector());
        }
    }
    cout << "TOTAL IDS: " << id << " TOTAL SAMPLES: " << samples_taken << endl;

    ret->print(cout, 10, true);

    cout << "DONE optimize_base_case" << endl << endl;

    return ret;
}

Frontier<PointQuery*>* construct_hybrid_point_query(DatasetAndWorkload& dataset_and_workload, RangeFilterTemplate& ground_truth)
{
    assert(!dataset_and_workload.get_workload().empty());

    //base case

    Frontier<PointQuery*>* base_case_frontier = optimize_base_case(dataset_and_workload);

    if(dataset_and_workload.get_dataset().size() == 1)
    {
        return base_case_frontier;
    }

    assert(!dataset_and_workload.get_dataset().empty());

    //recursive case

    pair<double, string>* _ratio_and_best_split = ground_truth.analyze_negative_point_query_density_heatmap(dataset_and_workload);

    if(_ratio_and_best_split == nullptr)
    {
        //no need to split;
        //return base case;
        return base_case_frontier;
    }

    pair<double, string> ratio_and_best_split = *_ratio_and_best_split;

//        float ratio = ratio_and_best_split.first;
    string best_split = ratio_and_best_split.second;

    cout << endl;

    vector<pair<string, string> > left_workload;
    vector<pair<string, string> > right_workload;

    const vector<pair<string, string> >& local_workload = dataset_and_workload.get_workload();

    for (size_t i = 0; i < local_workload.size(); i++) {
        if (local_workload[i].second <= best_split) {
            left_workload.push_back(local_workload[i]);
        } else {
            right_workload.push_back(local_workload[i]);
        }
    }

    if(left_workload.empty() || right_workload.empty())
    {
        return base_case_frontier;
    }

    vector<string> left_dataset;
    vector<string> right_dataset;

    const vector<string>& local_dataset = dataset_and_workload.get_dataset();

    for (size_t i = 0; i < local_dataset.size(); i++) {
        if (local_dataset[i] <= best_split) {
            left_dataset.push_back(local_dataset[i]);
        } else {
            right_dataset.push_back(local_dataset[i]);
        }
    }

    cout << "LEFT subproblem: |workload| = " << left_workload.size() << " |dataset| = " << left_dataset.size() << endl;
    cout << "RIGHT subproblem: |workload| = " << right_workload.size() << " |dataset| = " << right_workload.size() << endl;

    DatasetAndWorkload left_dataset_and_workload(left_dataset, left_workload);
    Frontier<PointQuery*>* left_frontier = construct_hybrid_point_query(left_dataset_and_workload, ground_truth);

    DatasetAndWorkload right_dataset_and_workload(right_dataset, right_workload);
    Frontier<PointQuery*>* right_frontier = construct_hybrid_point_query(right_dataset_and_workload, ground_truth);

    Frontier<PointQuery*>* ret = base_case_frontier;

    cout << "TRYING ALL COMBINATIONS " << left_frontier->get_size() <<" x "<< right_frontier->get_size() << endl;

    int id = 0;
    int samples = 2000;
    size_t space = left_frontier->get_size() * right_frontier->get_size() ;
    float ratio = (float)samples/space;
    int samples_taken = 0;

    for(const auto& left_pq : left_frontier->get_frontier())
    {
        for(const auto& right_pq: right_frontier->get_frontier())
        {
            id++;
            if(rand()%1000 < ratio*1000){
                samples_taken++;
            }
            else {
                continue;
            }
//            cout << "evaluating (sample " << samples_taken << ", id " << id << ")" << endl;
            HybridPointQuery* pq = new HybridPointQuery(best_split, left_pq.get_params(), right_pq.get_params());
            RangeFilterStats rez = dataset_and_workload.eval_point_query(pq);
            ret->insert(pq, rez.get_score_as_vector());
        }
    }

    ret->print(cout, 10, true);

    cout << "DONE WITH RECURSIVE STEP" << endl;

    return ret;
}

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

int main() {

//    micro unit tests
//    eval_trie_vs_rf();
//    return 0;

    string file_folder;
    string file_name = "1k_dataset.txt";
    string workload_difficulty = "impossible"; //choose from "easy", "medium", "hard", "impossible", hybrid
    string range_filter_type = "multi_bloom"; // choose from "heatmap", "trie", "surf", "one_bloom", "multi_bloom", "hybrid"
    string parameter_search_style = "simulated_annealing"; // choose from "no_search", "grid_search", "simulated_annealing", "dt_style"

    string file_path = file_folder + file_name;

    bool do_extract_dataset = false;
    if (do_extract_dataset) {
        extract_dataset(file_path, "6_dataset.txt", 6);
        return 0;
    }

    DatasetAndWorkload dataset_and_workload(file_path, workload_difficulty, false);

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
                                                          dataset_and_workload.get_negative_workload());

            PointQuery *ground_truth_point_query = new GroundTruthPointQuery();
            RangeFilterTemplate ground_truth = RangeFilterTemplate(dataset_and_workload, ground_truth_point_query,
                                                                   false);

            Frontier<PointQuery *> *ret = construct_hybrid_point_query(local_dataset_and_workload, ground_truth);

            ofstream dt_out("tree_of_hybrid_d1_1k_impossible.out");

            ret->print(dt_out);

            dt_out.close();

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
        ofstream output_file = ofstream ("results__simulated_annealing.out");
        assert(range_filter_type == "multi_bloom");
        simulated_annealing(dataset_and_workload, output_file);
        output_file.close();
    }


    return 0;
}

#include "Frontier.h"

void simulated_annealing(DatasetAndWorkload& dataset_and_workload, ofstream& output_file)
{
    const vector<string>& dataset = dataset_and_workload.get_dataset();
    double seed_fpr = 0.0000001;

    int seed_cutoff = dataset_and_workload.get_max_length_of_dataset();
    bool do_print = true;


    vector<string> dim_names;
    dim_names.emplace_back("bpk");
    dim_names.emplace_back("fpr");

    int output_frontier_every = 1000;
    int hard_copy_every = 2500;

    Frontier<RichMultiBloomParams> frontier(2);

    RichMultiBloom *pq = new RichMultiBloom(dataset_and_workload, seed_fpr, seed_cutoff, do_print);
    RangeFilterTemplate *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);

    RangeFilterStats ret = dataset_and_workload.test_range_filter(rf, do_print);

    size_t annealing_epoch = 0;
    size_t iter = 1;
    RichMultiBloom& tmp_rmb_pq = *(RichMultiBloom*)ret.get_params();
    RichMultiBloomParams& tmp_params = *(tmp_rmb_pq.add_iter_and_epoch(iter, annealing_epoch));
    assert(frontier.insert(tmp_params, ret.get_score_as_vector()));

    cout << endl;
    cout << ret.to_string() << endl;
    output_file << ret.to_string() << endl;

    ofstream frontiers("frontiers.out");

    annealing_epoch+=1;

    size_t total_num_inserts = 1;

    size_t success_count = 0;
    size_t explore_more_success_count_threshold = 1;

    size_t stagnation_count = 0;

    const size_t stagnation_count_cutoff_for_annealing_epoch_transition = 4;
    const size_t max_reinitialization_count = 2;

    while(true)
    {
        iter+=1;

        int dim_id = rand() % seed_cutoff;
        double mult = pow(0.5, 1.0/(double)annealing_epoch);
        if(rand()%2 == 0)
        {
            mult = pow(2.0, 1.0/(double)annealing_epoch);
        }

        pq->perturb(dim_id, mult, rf->get_is_leaf_char());

        RangeFilterStats ret = dataset_and_workload.test_range_filter(rf, do_print);

        cout << endl;
        output_file << endl;
        cout << "ITER " << iter << endl;
        output_file << "ITER " << iter << endl;
        cout << "EPOCH " << annealing_epoch << endl;
        output_file << "EPOCH " << annealing_epoch << endl;

        RichMultiBloom& local_tmp_rmb_pq = *(RichMultiBloom*)ret.get_params();
        RichMultiBloomParams& local_tmp_params = *local_tmp_rmb_pq.add_iter_and_epoch(iter, annealing_epoch);
        if(frontier.insert(local_tmp_params, ret.get_score_as_vector()))
        {
            success_count+=1;
            total_num_inserts+=1;
            stagnation_count = 0;

            cout << "INSERT & CONTINUE" << endl;
            output_file << "INSERT & CONTINUE" << endl;

            cout << "|frontier| = " << frontier.get_size() << endl;
            output_file << "|frontier| = " << frontier.get_size() << endl;
            cout << "|inserts| = " << total_num_inserts << endl;
            output_file << "|inserts| = " << total_num_inserts << endl;
            cout << "|betterment| = " << success_count << endl;
            output_file << "|betterment| = " << success_count << endl;

        }
        else
        {
            stagnation_count+=1;
            success_count = 0;
            cout << "UNDO" << endl << "|stagnation| " << stagnation_count << endl;
            output_file << "UNDO" << endl << "|stagnation| " << stagnation_count << endl;
            pq->undo();

        }

        cout << ret.to_string() << endl;
        output_file << ret.to_string() << endl;

        if(success_count >= explore_more_success_count_threshold && annealing_epoch >= 2)
        {
            cout << endl;
            output_file << endl;
            cout << "EXPLORE MORE" << endl;
            output_file << "EXPLORE MORE" << endl;
            assert(annealing_epoch >= 2);
//            annealing_epoch = annealing_epoch-1;
            annealing_epoch = max((size_t)1, annealing_epoch/2);
            cout << "NEW EPOCH " << annealing_epoch << endl;
            output_file << "NEW EPOCH " << annealing_epoch << endl;
            success_count = 0;
        }

        bool break_asap = false;
        if(stagnation_count >= stagnation_count_cutoff_for_annealing_epoch_transition)
        {
            cout << endl;
            output_file << endl;

            vector<FrontierPoint<RichMultiBloomParams> >& vec = frontier.get_frontier_to_modify();
            pair<size_t, int> oldest_params = make_pair(iter, -1);

            for(size_t reinitialization_count = 0;
                reinitialization_count <= max_reinitialization_count && oldest_params.second == -1;
                reinitialization_count++) {
                if(reinitialization_count >= 1)
                {
                    cout << "REINITIALIZATION_COUNT " << reinitialization_count << endl;
                    output_file << "REINITIALIZATION_COUNT " << reinitialization_count << endl;
                    cout << "MAX_REINITIALIZATION_COUNT " << max_reinitialization_count << endl;
                    output_file << "MAX_REINITIALIZATION_COUNT " << max_reinitialization_count << endl;
                }
                for (size_t i = 0; i < vec.size(); i++) {
                    if (vec[i].get_params().get_used_for_reinit_count() <= reinitialization_count) {
                        oldest_params = min(oldest_params, make_pair(vec[i].get_params().get_iter_id(), (int) i));
                    }
                }
            }

            if(oldest_params.second == -1)
            {
                cout << "BREAK ASAP" << endl;
                output_file << "BREAK ASAP" << endl;
                break_asap = true;
            }
            else {

                assert(oldest_params.second != -1);

                cout << "REINITIALIZE TO" << endl;
                output_file << "REINITIALIZE TO" << endl;

                cout << vec[oldest_params.second].to_string(dim_names) << endl;
                output_file << vec[oldest_params.second].to_string(dim_names) << endl;

                pq->reinitialize(vec[oldest_params.second].get_params());
                rf->insert_prefixes(dataset);

                RichMultiBloomParams &params = vec[oldest_params.second].get_params_to_modify();
                params.used_for_reinit();

                annealing_epoch = params.get_epoch();

                cout << "NEW EPOCH " << annealing_epoch << endl;
                output_file << "NEW EPOCH " << annealing_epoch << endl;

                stagnation_count = 0;
                success_count = 0;
            }

        }

//        assert(stagnation_count_cutoff_for_annealing_epoch_transition == 7);
//        assert(stagnation_count <= stagnation_count_cutoff_for_annealing_epoch_transition);

        if(iter % output_frontier_every == 0 || break_asap)
        {
            const vector<FrontierPoint<RichMultiBloomParams> >& vec = frontier.get_frontier();

//            frontiers << "ITER " << iter << endl;
//            frontiers << "EPOCH " << annealing_epoch << endl;
//            frontiers << "|inserts| = " << total_num_inserts << endl;
//            frontiers << "|betterment| = " << success_count << endl;
//            frontiers << "|stagnation| = " << stagnation_count << endl;
//            frontiers << "|frontier| = " << frontier.get_size() << endl;

            vector<size_t> reinit_counts;
            for(size_t i = 0; i <= max_reinitialization_count+1; i++)
            {
                reinit_counts.push_back(0);
            }

            for(size_t i = 0;i<vec.size();i++)
            {
                size_t reinit_count = vec[i].get_params().get_used_for_reinit_count();
                assert(reinit_count <= reinit_counts.size());
                reinit_counts[reinit_count]+=1;
            }

            for(size_t i = 0; i <= max_reinitialization_count+1; i++){
                if(reinit_counts[i] >= 1) {
                    frontiers << "|reinit_counts = " << i << "| = " << reinit_counts[i] << endl;
                }
            }
            for(size_t i = 0;i<vec.size();i++){
                frontiers << vec[i].to_string(dim_names) << endl;
            }
            frontier.print(frontiers, -1, false);
            frontiers << endl;

            if(break_asap)
            {
                cout << endl;
                output_file << endl;
                cout << "BREAK" << endl;
                output_file << "BREAK" << endl;
                break;
            }
        }

        if(iter % hard_copy_every == 0) {
            ofstream latest_frontier("latest_frontier_impossible1k_base128_init0.0000001_stagcountcutoff6_iter" + std::to_string(iter) + ".out");
            frontier.print(latest_frontier, -1, false);
            latest_frontier.close();
        }
    }

    cout << "DONE" << endl;
    output_file << "DONE" << endl;
}

void grid_search(DatasetAndWorkload& dataset_and_workload, const string& _range_filter_type, ofstream& output_file){

    const vector<string>& dataset = dataset_and_workload.get_dataset();

    string range_filter_type = _range_filter_type;

    range_filter_type = "hybrid";

    if(range_filter_type == "hybrid")
    {
        int id = 0;
        int samples = 100 ;
        int space = 20*20*24*24;
        float ratio = (float)samples/space;
        int samples_taken = 0;
        Frontier<HybridPointQueryParams> frontier(2);
        Frontier<MultiBloomParams> multi_bloom_frontier(2);
        for(int left_cutoff = 1; left_cutoff <= 20; left_cutoff++) {//20
            for(float left_fpr = 0.001; left_fpr < 0.6; left_fpr = min(left_fpr+0.05, left_fpr*1.5)) {//6

                bool do_print = false;

                for(int right_cutoff = 1; right_cutoff <= 20; right_cutoff++) {//20
                    for (float right_fpr = 0.001; right_fpr < 0.6; right_fpr = min(right_fpr+0.05, right_fpr * 1.5)) { //6
//                        cout << "ID: " << id++ << endl;
                        id++;
//                        cout << left_cutoff << " "<< right_cutoff <<" "<< left_fpr <<" "<< right_fpr << endl;
                        if(rand()%1000 < ratio*1000)
                        {
                            cout << "#SAMPLES: " << samples_taken++ << endl;
                        }
                        else
                        {
                            continue;
                        }
                        PointQuery *pq;
                        pq = new HybridPointQuery(dataset, "kirk", left_cutoff, left_fpr, right_cutoff, right_fpr, do_print);
                        auto *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);
                        RangeFilterStats ret = dataset_and_workload.test_range_filter(rf, do_print);
                        HybridPointQueryParams *params = (HybridPointQueryParams *) ret.get_params();
                        frontier.insert(*(params->clone()), ret.get_score_as_vector());
                        output_file << ret.to_string() << endl;
                        cout << ret.to_string() << endl;
                        cout << "DONE" << endl;

                        {
                            PointQuery *mb_pq = new MultiBloom(dataset, right_fpr, right_cutoff, do_print);
                            auto *multi_bloom_rf = new RangeFilterTemplate(dataset_and_workload, mb_pq, do_print);
                            RangeFilterStats mb_ret = dataset_and_workload.test_range_filter(multi_bloom_rf, do_print);
                            MultiBloomParams *mb_params = (MultiBloomParams *) mb_ret.get_params();
                            multi_bloom_frontier.insert(*(mb_params->clone()), mb_ret.get_score_as_vector());
                        }
                        {
                            PointQuery *mb_pq = new MultiBloom(dataset, left_fpr, left_cutoff, do_print);
                            auto *multi_bloom_rf = new RangeFilterTemplate(dataset_and_workload, mb_pq, do_print);
                            RangeFilterStats mb_ret = dataset_and_workload.test_range_filter(multi_bloom_rf, do_print);
                            MultiBloomParams *mb_params = (MultiBloomParams *) mb_ret.get_params();
                            multi_bloom_frontier.insert(*(mb_params->clone()), mb_ret.get_score_as_vector());
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
                RangeFilterStats ret = dataset_and_workload.test_range_filter(rf, do_print);

                if(range_filter_type == "multi_bloom") {
                    MultiBloomParams *params = (MultiBloomParams *)ret.get_params();
                    frontier.insert(*(params->clone()), ret.get_score_as_vector());
                }
                delete rf;
                pq->clear();
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
