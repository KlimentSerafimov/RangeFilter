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

int prep_dataset_and_workload(const string& file_path, string workload_difficulty)
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


    for(size_t i = 0;i<workload_seed.size()-1;i++)
    {
        if(workload_difficulty == "easy") {
            //eeasy workload
            string left_key = workload_seed[i];
            string right_key = left_key;
            right_key[(int)right_key.size()-1]+=(char)1;
            workload.emplace_back(make_pair(left_key, right_key));
        }
        else if (workload_difficulty == "medium")
        {
            //medium workload
            string left_key = workload_seed[i];
            string right_key = left_key;
            right_key[(int)right_key.size()/4]+=(char)1;

            workload.emplace_back(make_pair(left_key, right_key));
        }
        else if(workload_difficulty == "hard") {
            //hard workload
            string left_key = workload_seed[i];
            string right_key = workload_seed[i + 1];
            left_key += init_char;
            right_key[right_key.size() - 1] -= 1;

            workload.emplace_back(make_pair(left_key, right_key));
        }
        else
        {
    assert(false);
        }


        if((i+1)%10000000 == 0)
        {
            cout << "num_workloads converted " << i+1 << endl;
        }

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

int main_test_surf(const string& file_path)
{
    prep_dataset_and_workload(file_path, "easy");

    for(int trie_size = 2; trie_size <= 30; trie_size+=2) {
        SurfStats surf_ret = test_surf(dataset, workload, trie_size);

        cout << surf_ret.to_string() << endl;
    }
    return 0;
}
void grid_search(const string& range_filter_type, ofstream& output_file);
void simulated_annealing(ofstream& output_file);



int main() {


    string file_folder = "";
    string file_name = "1M_dataset.txt";
    string workload_difficulty = "medium";
    string range_filter_type = "multi_bloom"; // choose from "surf", "one_bloom", "multi_bloom"
    string parameter_search_style = "simulated_annealing"; // choose from "grid_search", "simulated_annealing"

    if (range_filter_type == "surf") {
        assert(parameter_search_style == "grid_search");
        main_test_surf(file_folder + file_name);
        return 0;
    }

//    micro unit tests
//    eval_trie_vs_rf();
//    return 0;

//    test_bloom_filter();
//    return 0;

//    string file_folder = "/home/kliment/Downloads/";
//    string file_name = "emails-validated-random-only-30-characters.txt.sorted";

    string file_path = file_folder + file_name;

    bool do_extract_dataset = false;
    if (do_extract_dataset) {
        extract_dataset(file_path, "1M_dataset.txt", 1000000);
        return 0;
    }

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
    double seed_fpr = 0.01;
    int seed_cutoff = 12;
    bool do_print = false;

    int output_frontier_every = 10;

    Frontier<RichMultiBloomParams> frontier(2);

    RichMultiBloom *pq;
    pq = new RichMultiBloom(dataset, workload, seed_fpr, seed_cutoff, true);
    auto *rf = new RangeFilterTemplate(dataset, workload, pq, do_print);

    RangeFilterStats ret = test_range_filter(dataset, workload, rf, do_print);

    assert(frontier.insert(*((RichMultiBloomParams*)ret.get_params()), ret.to_vector()));

    cout << ret.to_string() << endl;

    ofstream frontiers("frontiers.out");

    int iter = 1;
    int total_num_inserts = 1;

    int stagnation_count = 0;
    int max_stagnation = 0;

    while(true)
    {
        iter+=1;

        int dim_id = rand() % seed_cutoff;
        double mult = 0.5;
        if(rand()%2 == 0)
        {
            mult = 2.0;
        }

        pq->perturb(dim_id, mult, rf->get_is_leaf_char());

        RangeFilterStats ret = test_range_filter(dataset, workload, rf, do_print);

        cout << endl;
        cout << "ITER " << iter << endl;
        output_file << "ITER " << iter << endl;
        if(frontier.insert(*((RichMultiBloomParams*)ret.get_params()), ret.to_vector()))
        {
            total_num_inserts+=1;
            stagnation_count = 0;

            cout << "INSERT & CONTINUE" << endl;
            output_file << "INSERT & CONTINUE" << endl;

            cout << "|frontier| = " << frontier.get_size() << endl;
            output_file << "|frontier| = " << frontier.get_size() << endl;
            cout << "|inserts| = " << total_num_inserts << endl;
            output_file << "|inserts| = " << total_num_inserts << endl;
        }
        else
        {
            stagnation_count+=1;
            max_stagnation = max(max_stagnation, stagnation_count);
            cout << "UNDO" << endl << "|stagnation| " << stagnation_count << endl << "|max_stagnation| " << max_stagnation << endl;
            output_file << "UNDO" << endl << "|stagnation| " << stagnation_count << endl << "|max_stagnation| " << max_stagnation << endl;
            pq->undo();
        }

        cout << ret.to_string() << endl << endl;
        output_file << ret.to_string() << endl << endl;

        if(iter % output_frontier_every == 0)
        {
            const vector<FrontierPoint<RichMultiBloomParams> >& vec = frontier.get_frontier();

            frontiers << "ITER " << iter << endl;
            frontiers << "|inserts| = " << total_num_inserts << endl;
            frontiers << "|stagnation| " << stagnation_count << endl;
            frontiers << "|max_stagnation| " << max_stagnation << endl;
            frontiers << "|frontier| = " << frontier.get_size() << endl;
            for(size_t i = 0;i<vec.size();i++)
            {
                frontiers << vec[i].to_string() << endl;
            }

            frontiers << endl;
        }
    }

}

void grid_search(const string& range_filter_type, ofstream& output_file){

    for(int cutoff = 0; cutoff < 20; cutoff++) {

        const int num_seed_fprs = 10;
        float seed_fprs[num_seed_fprs] = {
//                0.00001,
                0.0001,
                0.001,
                0.005,
                0.01,
                0.05,
                0.1,
                0.2,
                0.3,
                0.4,
                0.5,
//                0.6,
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
            if(ret.false_positive_rate() > 50)
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
