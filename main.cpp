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

using namespace std;

void eval_trie_vs_rf()
{
    vector<string> dataset;
    dataset.emplace_back("aaabbb");
    dataset.emplace_back("aaaqqq");
    dataset.emplace_back("aaazzz");
    dataset.emplace_back("aaaff");
    dataset.emplace_back("ccc");


    Trie trie(dataset);

    vector<pair<int, double> > vec;
    for(int i = 0; i<8; i++)
    {
        vec.emplace_back(20, 0.0001);
    }


    vector<pair<string, string> > queries;
    queries.emplace_back(make_pair("aaaa", "aaab"));
    queries.emplace_back(make_pair("aaaa", "aaac"));
    queries.emplace_back(make_pair("aaabbb", "aaabbb"));
    queries.emplace_back(make_pair("aaabbc", "aaaqqp"));
    queries.emplace_back(make_pair("aaaqqp", "aaaqqq"));
    queries.emplace_back(make_pair("aaaqqq", "aaaqqq"));
    queries.emplace_back(make_pair("aaaqqq", "aaaqqr"));
    queries.emplace_back(make_pair("aaaqqp", "aaaqqr"));
    queries.emplace_back(make_pair("aaaqqr", "aaaqqz"));
    queries.emplace_back(make_pair("aaaqqr", "aaaqqy"));
    queries.emplace_back(make_pair("aaaqqr", "aaazzz"));
    queries.emplace_back(make_pair("aaaqqr", "aaazzy"));
    queries.emplace_back(make_pair("aaaqqr", "aaazyz"));
    queries.emplace_back(make_pair("aaaqqr", "aaayzz"));
    queries.emplace_back(make_pair("aaaqqr", "aaayzz"));
    queries.emplace_back(make_pair("aa", "ab"));
    queries.emplace_back(make_pair("a", "aaabba"));
    queries.emplace_back(make_pair("a", "aaabbb"));
    queries.emplace_back(make_pair("a", "aaazzz"));
    queries.emplace_back(make_pair("aaabbc", "aaabbc"));
    queries.emplace_back(make_pair("aaaab", "aaaz"));
    queries.emplace_back(make_pair("aaac", "aaaz"));
    queries.emplace_back(make_pair("aac", "aacz"));
    queries.emplace_back(make_pair("aaabbba", "aaaqqp"));
    queries.emplace_back(make_pair("aaaqqp", "aaaqqqa"));
    queries.emplace_back(make_pair("aaaff", "aaaff"));
    queries.emplace_back(make_pair("cbb", "ddd"));
    queries.emplace_back(make_pair("bbb", "cdd"));

    auto* multi_bloom = new MultiBloom(dataset, queries, 0.0001, -1, true);
    RangeFilterTemplate rf(dataset, queries, multi_bloom, true);

    cout << "start eval" << endl;
    for(auto q: queries)
    {
        cout << q.first <<" "<< q.second << endl;
        bool trie_out = trie.query(q.first, q.second);
        bool rf_out = rf.query(q.first, q.second);
        cout <<trie_out <<" "<< rf_out << endl;
        assert(trie_out == rf_out);
    }

    cout << "done" << endl;

}

vector<string> read_dataset(const string& file_path)
{
    vector<string> dataset;

    ifstream file(file_path);

    cout << "INIT reading file " << file_path << endl;
    int id = 0;
    while (!file.eof())
    {
        string line;
        getline(file, line);
        if(line != "") {
            dataset.push_back(line);
        }
        if((id+1)%10000000 == 0)
            cout << "reading line #" << id+1 <<" line=\""<< line << "\""<< endl;
        id++;
    }
    file.close();
    cout << "END reading file " << file_path << endl;
    cout << "READ " << dataset.size() << " rows" << endl;
    return dataset;
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


RangeFilterStats test_range_filter(const vector<string>& dataset, const vector<pair<string, string> >& workload, RangeFilterTemplate* rf, bool do_print)
{
    int num_positive = 0;
    int num_negative = 0;
    int num_false_positives = 0;
    int num_false_negatives = 0;

    int true_positives = 0;
    int true_negatives = 0;

    for(size_t i = 0;i<workload.size();i++)
    {
        string left_key = workload[i].first;
        string right_key = workload[i].second;

        bool ground_truth = contains(dataset, left_key, right_key);
        bool prediction = rf->query(left_key, right_key);

        if(ground_truth)
        {
            num_positive+=1;
            if(!prediction)
            {
                num_false_negatives+=1;
                assert(false);
            }
            else
            {
                true_positives+=1;
            }
        }
        else
        {
            num_negative+=1;
            if(prediction)
            {
                num_false_positives +=1;
            }
            else
            {
                true_negatives += 1;
            }
        }

        if((i+1)%1000000 == 0)
        {
            cout << "tested " << i+1 << "/" << workload.size() << endl;
        }
    }

    assert(num_false_negatives == 0);

    assert(true_negatives + true_positives + num_false_positives + num_false_negatives == (int) workload.size());
    if(do_print) {
        cout << rf->get_memory() << " bytes" << endl;
        cout << "true_positives " << true_positives << endl;
        cout << "true_negatives " << true_negatives << endl;
        cout << "false_positives " << num_false_positives << endl;
        cout << "false_negatives " << num_false_negatives << endl;
        cout << "sum " << true_negatives + true_positives + num_false_positives + num_false_negatives
             << " workload.size() " << workload.size() << endl;

        cout << "assert(true_negatives+true_positives+num_false_positives+num_false_negatives == workload.size()); passed" << endl;
    }


    RangeFilterStats ret(
            rf->get_params(),
            (int)dataset.size(),
            (int)workload.size(),
            num_false_positives,
            num_negative,
            (int)rf->get_memory()*8);


    return ret;
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

vector<string> workload_seed;
vector<string> dataset;
vector<string> workload_seed_and_dataset;

int test_bloom_filter()
{

    string file_folder = "";
    string file_name = "5M_dataset.txt";
    string file_path = file_folder + file_name;

    bloom_parameters parameters;
    workload_seed_and_dataset = read_dataset(file_path);

    cout << "workload_seed_and_dataset.size() " << workload_seed_and_dataset.size() << endl;

    cout <<"shuffling" << endl;
    shuffle(workload_seed_and_dataset.begin(), workload_seed_and_dataset.end(), std::default_random_engine(0));
    cout <<"done shuffling" << endl;
    cout << "splitting" << endl;

    int num_bf_inserts = 0;
    for (size_t i = 0; i < workload_seed_and_dataset.size(); i++) {
        if (i < workload_seed_and_dataset.size() / 2) {
            workload_seed.push_back(workload_seed_and_dataset[i]);
        } else {
            num_bf_inserts += (int)workload_seed_and_dataset[i].size()+1;
            dataset.push_back(workload_seed_and_dataset[i]);
        }
        if((i+1)%10000000 == 0)
        {
            cout << "num_split " << i+1 << endl;
        }
    }

    cout << "done splitting" << endl;

    workload_seed_and_dataset.clear();

    cout << "sorting" << endl;

    sort(workload_seed.begin(), workload_seed.end());
    sort(dataset.begin(), dataset.end());

    cout << "done sorting"<< endl;

    // How many elements roughly do we expect to insert?
    parameters.projected_element_count = dataset.size();

    // Maximum tolerable false positive probability? (0,1)
    parameters.false_positive_probability = 0.08;

    // Simple randomizer (optional)
    parameters.random_seed = 0xA5A5A5A5;

    parameters.compute_optimal_parameters();

    //Instantiate Bloom Filter
    bloom_filter bf = bloom_filter(parameters);

    for(size_t i = 0;i<dataset.size();i++)
    {
        bf.insert(dataset[i]);
    }


    for(size_t i = 0;i<dataset.size();i++)
    {
        if(bf.contains(dataset[i]))
        {
            assert(contains(dataset, dataset[i], dataset[i]));
        }
        else
        {
            assert(false);
        }
    }

    int true_negatives = 0;
    int num_false_positives = 0;
    for(size_t i = 0;i<workload_seed.size();i++)
    {
        if(bf.contains(workload_seed[i]))
        {
            assert(!contains(dataset, workload_seed[i], workload_seed[i]));
            num_false_positives += 1;
        }
        else
        {
            true_negatives += 1;
        }
    }

    cout << "num_false_positives " << num_false_positives << endl;
    cout << "workload_size " << workload_seed.size() << endl;
    cout << "fpr " << (float)num_false_positives/workload_seed.size() * 100.0 << endl;
    cout << "memory " << bf.get_memory_in_bits() << endl;
    cout << "bits per key " << bf.get_memory_in_bits()/dataset.size() << endl;
    cout << "true_negatives " << true_negatives << endl;

    return 0;

}

vector<pair<string, string> > workload;

int prep_dataset_and_workload(const string& file_path, string workload_difficulty, int impossible_depth = -1)
{

    workload_seed_and_dataset = read_dataset(file_path);

    cout << "workload_seed_and_dataset.size() " << workload_seed_and_dataset.size() << endl;

    cout <<"shuffling" << endl;
    shuffle(workload_seed_and_dataset.begin(), workload_seed_and_dataset.end(), std::default_random_engine(0));
    cout <<"done shuffling" << endl;
    cout << "splitting" << endl;

    int num_bf_inserts = 0;
    char init_char = (char)127;
    for (size_t i = 0; i < workload_seed_and_dataset.size(); i++) {
        if (i < workload_seed_and_dataset.size() / 2) {
            workload_seed.push_back(workload_seed_and_dataset[i]);
        } else {
            num_bf_inserts+=(int)workload_seed_and_dataset[i].size()+1;
            dataset.push_back(workload_seed_and_dataset[i]);
            for(size_t j = 0; j < workload_seed_and_dataset[i].size();j++)
            {
                init_char = min(init_char, (char)workload_seed_and_dataset[i][j]);
            }
        }
        if((i+1)%10000000 == 0)
        {
            cout << "num_split " << i+1 << endl;
        }
    }

    cout << "done splitting" << endl;

    workload_seed_and_dataset.clear();

    cout << "sorting" << endl;

    sort(workload_seed.begin(), workload_seed.end());
    sort(dataset.begin(), dataset.end());

    cout << "done sorting"<< endl;


    if(workload_difficulty == "easy" || workload_difficulty == "medium" || workload_difficulty == "hard") {
        for (size_t i = 0; i < workload_seed.size() - 1; i++) {
            if (workload_difficulty == "easy") {
                //eeasy workload
                string left_key = workload_seed[i];
                string right_key = left_key;
                right_key[(int) right_key.size() - 1] += (char) 1;
                workload.emplace_back(make_pair(left_key, right_key));
            } else if (workload_difficulty == "medium") {
                //medium workload
                string left_key = workload_seed[i];
                string right_key = left_key;
                right_key[(int) right_key.size() / 4] += (char) 1;

                workload.emplace_back(make_pair(left_key, right_key));
            } else if (workload_difficulty == "hard") {
                //hard workload
                string left_key = workload_seed[i];
                string right_key = workload_seed[i + 1];

                workload.emplace_back(make_pair(left_key, right_key));
            }else {
                assert(false);
            }
            if ((i + 1) % 10000000 == 0) {
                cout << "num_workloads converted " << i + 1 << endl;
            }
        }
    }
    else if (workload_difficulty == "impossible")
    {
        for (size_t i = 0; i < dataset.size() - 1; i++) {
            if (workload_difficulty == "impossible") {
                //hard workload
                string left_key = dataset[i];
                string right_key = dataset[i + 1];

                if(left_key >= right_key)
                {
                    continue;
                }
                if(impossible_depth == -1) {
                    left_key += init_char;
                    right_key[right_key.size() - 1] -= 1;
                    assert(left_key < right_key);
                }
                else
                {
                    assert(impossible_depth >= 1);
                    assert(false);
//                    if(impossible_depth > left_key)
                }
                workload.emplace_back(make_pair(left_key, right_key));
            } else {
                assert(false);
            }
            if ((i + 1) % 10000000 == 0) {
                cout << "num_workloads converted " << i + 1 << endl;
            }
        }
    }
    else
    {
        assert(false);
    }

    cout << "workload size " << workload_seed.size() << endl;
    cout << "dataset size " << dataset.size() << endl;
    cout << "num_bf_inserts " << num_bf_inserts << endl;

    int num_negatives = get_num_negatives(dataset, workload);

    cout << "num_negatives " << num_negatives << endl;
    cout << "num_positives " << workload.size() - num_negatives << endl;
    cout << "num_negatives(%) " << (float)num_negatives/(float)workload.size()*100.0 << endl;

    long long sum_memory_in_bits = sizeof(dataset);
    for(size_t i = 0; i<dataset.size();i++)
    {
        for(size_t j = 0;j<dataset[i].size();j++)
        {
            assert(sizeof(dataset[i][j]) == 1);
            sum_memory_in_bits+=8*sizeof(dataset[i][j]);
        }
    }
    cout << "num_bits_in_dataset " << sum_memory_in_bits << endl;
    cout << "bits_per_key  " << (float)sum_memory_in_bits/dataset.size() << endl;

    return num_bf_inserts;
}

int main_test_surf()
{
    for(int trie_size = 1; trie_size <= 120; trie_size+=1) {
        SurfStats surf_ret = test_surf(dataset, workload, trie_size);

        cout << surf_ret.to_string() << endl;
    }
    return 0;
}
void grid_search(const string& range_filter_type, ofstream& output_file);
void simulated_annealing(ofstream& output_file);

void eval_trie_heatmap()
{
    Trie trie = Trie(dataset);

    cout << endl;
    cout << "trie bpk " << (double)(trie.get_memory()*8)/(double)dataset.size() << endl;
    int num_positive = 0;
    int num_negative = 0;
    int num_false_positives = 0;
    int num_false_negatives = 0;

    int true_positives = 0;
    int true_negatives = 0;

    for(size_t i = 0;i<workload.size();i++)
    {
        string left_key = workload[i].first;
        string right_key = workload[i].second;

        bool ground_truth = contains(dataset, left_key, right_key);

        if(ground_truth)
        {
            continue;
        }

        bool prediction = (&trie)->query(left_key, right_key);

        if(ground_truth)
        {
            num_positive+=1;
            if(!prediction)
            {
                num_false_negatives+=1;
                assert(false);
            }
            else
            {
                true_positives+=1;
            }
        }
        else
        {
            num_negative+=1;
            if(prediction)
            {
                num_false_positives +=1;
            }
            else
            {
                true_negatives += 1;
            }
        }

        if((i+1)%1000000 == 0)
        {
            cout << "tested " << i+1 << "/" << workload.size() << endl;
        }
    }

    assert(num_false_positives == 0);
    assert(num_false_negatives == 0);

    cout << "all correct" << endl;

    cout << "num ret_false " << trie.sum_count_ret_false() << " num_negatives " << num_negative << endl;
    cout << "num negative_point_queries/num_negatives " << (double) trie.sum_count_ret_false() / num_negative << endl;

//    easy
//    num ret_false 24999 num_negatives 24999
//    num negative_point_queries/num_negatives 1

//    medium
//    num ret_false 23318 num_negatives 21552
//    num negative_point_queries/num_negatives 1.08194

//    hard
//    num ret_false 16268 num_negatives 12467
//    num negative_point_queries/num_negatives 1.30488

//    impossible
//    num ret_false 461930 num_negatives 24999
//    num negative_point_queries/num_negatives 18.4779





}

void eval_rf_heatmap()
{
    RichMultiBloom *pq;
    bool do_print = true;
    pq = new RichMultiBloom(dataset, workload, 0.0000000001, -1, do_print);
    auto *rf = new RangeFilterTemplate(dataset, workload, pq, do_print);


//    cout << endl;
//    cout << "trie bpk " << (double)(trie.get_memory()*8)/(double)dataset.size() << endl;

    int num_positive = 0;
    int num_negative = 0;
    int num_false_positives = 0;
    int num_false_negatives = 0;

    int true_positives = 0;
    int true_negatives = 0;

    for(size_t i = 0;i<workload.size();i++)
    {
        string left_key = workload[i].first;
        string right_key = workload[i].second;

        bool ground_truth = contains(dataset, left_key, right_key);

        if(ground_truth)
        {
            continue;
        }

        bool prediction = rf->query(left_key, right_key);

        if(ground_truth)
        {
            num_positive+=1;
            if(!prediction)
            {
                num_false_negatives+=1;
                assert(false);
            }
            else
            {
                true_positives+=1;
            }
        }
        else
        {
            num_negative+=1;
            if(prediction)
            {
                num_false_positives +=1;
            }
            else
            {
                true_negatives += 1;
            }
        }

        if((i+1)%1000000 == 0)
        {
            cout << "tested " << i+1 << "/" << workload.size() << endl;
        }
    }

    assert(num_false_positives == 0);
    assert(num_false_negatives == 0);

    cout << "all correct" << endl;

    cout << "num negative_point_queries " << rf->get_negative_point_queries() << " num_negatives " << num_negative << endl;
    cout << "num negative_point_queries/num_negatives " << (double) rf->get_negative_point_queries() / num_negative << endl;

    //TRIE
//50k
//    easy
//    num ret_false 24999 num_negatives 24999
//    num negative_point_queries/num_negatives 1

//    medium
//    num ret_false 23318 num_negatives 21552
//    num negative_point_queries/num_negatives 1.08194

//    hard
//    num ret_false 16268 num_negatives 12467
//    num negative_point_queries/num_negatives 1.30488

//    impossible
//    num ret_false 461930 num_negatives 24999
//    num negative_point_queries/num_negatives 18.4779

//RF
//50k
//easy
//    num negative_point_queries 24999 num_negatives 24999
//    num negative_point_queries/num_negatives 1

//medium
//    num negative_point_queries 68378 num_negatives 21552
//    num negative_point_queries/num_negatives 3.1727

//hard
//    num negative_point_queries 331456 num_negatives 12467
//    num negative_point_queries/num_negatives 26.5867

//impossible
//    num negative_point_queries 39505877 num_negatives 24999
//    num negative_point_queries/num_negatives 1580.3
// 0.9  = x^1580


//1M
//    easy
//    num negative_point_queries 499999 num_negatives 499995
//    num negative_point_queries/num_negatives 1.00001

//    medium

//    num negative_point_queries 2118338 num_negatives 307724
//    num negative_point_queries/num_negatives 6.88389

//1k impossible

//    num negative_point_queries 752820 num_negatives 499
//    num negative_point_queries/num_negatives 1508.66



}

int main() {


    string file_folder;
    string file_name = "10k_dataset.txt";
    string workload_difficulty = "medium"; //choose from "easy", "medium", "hard", "impossible"
    string range_filter_type = "hybrid"; // choose from "heatmap", "trie", "surf", "one_bloom", "multi_bloom", "hybrid"
    string parameter_search_style = "grid_search"; // choose from "no_search", "grid_search", "simulated_annealing"

    string file_path = file_folder + file_name;

    bool do_extract_dataset = false;
    if (do_extract_dataset) {
        extract_dataset(file_path, "1k_dataset.txt", 1000);
        return 0;
    }

    prep_dataset_and_workload(file_path, workload_difficulty);

    if (range_filter_type == "surf") {
        assert(parameter_search_style == "grid_search");
        main_test_surf();
        return 0;
    }

    if(range_filter_type == "heatmap")
    {
        assert(parameter_search_style == "no_search");
        eval_rf_heatmap();
        return 0;
    }

    if(range_filter_type == "trie")
    {
        assert(parameter_search_style == "no_search");
        eval_trie_heatmap();
        return 0;
    }
//    micro unit tests
//    eval_trie_vs_rf();
//    return 0;

//    test_bloom_filter();
//    return 0;

//    string file_folder = "/home/kliment/Downloads/";
//    string file_name = "emails-validated-random-only-30-characters.txt.sorted";



    prep_dataset_and_workload(file_path, workload_difficulty);

    ofstream output_file = ofstream ("results.out");

    if(parameter_search_style == "grid_search") {
        grid_search(range_filter_type, output_file);
    }
    else {
        assert(range_filter_type == "multi_bloom");
        simulated_annealing(output_file);
    }

    output_file.close();

    return 0;
}

#include "Frontier.h"

void simulated_annealing(ofstream& output_file)
{
    double seed_fpr = 0.1;

    int seed_cutoff = 7;
    bool do_print = true;

    vector<string> dim_names;
    dim_names.emplace_back("bpk");
    dim_names.emplace_back("fpr");

    int output_frontier_every = 20;

    Frontier<RichMultiBloomParams> frontier(2);

    RichMultiBloom *pq;
    pq = new RichMultiBloom(dataset, workload, seed_fpr, seed_cutoff, do_print);
    RangeFilterTemplate *rf = new RangeFilterTemplate(dataset, workload, pq, do_print);

    RangeFilterStats ret = test_range_filter(dataset, workload, rf, do_print);

    size_t annealing_epoch = 0;
    size_t iter = 1;
    assert(frontier.insert(*(((RichMultiBloomParams *) ret.get_params())->add_iter_and_epoch(iter, annealing_epoch)), ret.to_vector()));

    cout << endl;
    cout << ret.to_string() << endl;
    output_file << ret.to_string() << endl;

    ofstream frontiers("frontiers.out");

    annealing_epoch+=1;

    size_t total_num_inserts = 1;

    size_t success_count = 0;
    size_t explore_more_success_count_threshold = 3;

    size_t stagnation_count = 0;

    const size_t stagnation_count_cutoff_for_annealing_epoch_transition = seed_cutoff; //seed_cutoff*4;
    const size_t max_reinitialization_count = 1;

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

        RangeFilterStats ret = test_range_filter(dataset, workload, rf, do_print);

        cout << endl;
        output_file << endl;
        cout << "ITER " << iter << endl;
        output_file << "ITER " << iter << endl;
        cout << "EPOCH " << annealing_epoch << endl;
        output_file << "EPOCH " << annealing_epoch << endl;
        if(frontier.insert(*(((RichMultiBloomParams *) ret.get_params())->add_iter_and_epoch(iter, annealing_epoch)), ret.to_vector()))
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
            annealing_epoch = annealing_epoch-1;
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

        if(iter % output_frontier_every == 0 || break_asap)
        {
            const vector<FrontierPoint<RichMultiBloomParams> >& vec = frontier.get_frontier();

            frontiers << "ITER " << iter << endl;
            frontiers << "EPOCH " << annealing_epoch << endl;
            frontiers << "|inserts| = " << total_num_inserts << endl;
            frontiers << "|betterment| = " << success_count << endl;
            frontiers << "|stagnation| = " << stagnation_count << endl;
            frontiers << "|frontier| = " << frontier.get_size() << endl;

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

            for(size_t i = 0; i <= max_reinitialization_count+1; i++)
            {
                if(reinit_counts[i] >= 1) {
                    frontiers << "|reinit_counts = " << i << "| = " << reinit_counts[i] << endl;
                }
            }


            for(size_t i = 0;i<vec.size();i++)
            {
                frontiers << vec[i].to_string(dim_names) << endl;
            }

            frontiers << endl;

            if(break_asap)
            {
                cout << endl;
                output_file << endl;
                cout << "BREAK" << endl;
                output_file << "BREAK" << endl;
            }
        }
    }

    cout << "DONE" << endl;
    output_file << "DONE" << endl;
}

void grid_search(const string& range_filter_type, ofstream& output_file){

    for(int cutoff = 0; cutoff < 20; cutoff++) {

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
            bool do_print = true;
            if (range_filter_type == "one_bloom") {
                pq = new OneBloom(dataset, workload, seed_fprs[seed_fpr_id], cutoff, do_print);
            } else if (range_filter_type == "multi_bloom") {
                pq = new MultiBloom(dataset, workload, seed_fprs[seed_fpr_id], cutoff, do_print);
            } else {
                assert(false);
            }
            auto *rf = new RangeFilterTemplate(dataset, workload, pq, do_print);
            RangeFilterStats ret = test_range_filter(dataset, workload, rf, do_print);
            rf->clear();
            output_file << ret.to_string() << endl;
            cout << ret.to_string() << endl;
            if(ret.false_positive_rate() > 0.5)
            {
                output_file << "fpr too large; break;" << endl;
                cout << "fpr too large; break;" << endl;
                break;
            }
        }
        output_file << endl;
        cout << endl;
    }

}
