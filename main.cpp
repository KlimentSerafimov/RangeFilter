#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <assert.h>
#include <set>
#include <random>
#include <cstring>

#include "bloom/bloom_filter.hpp"


using namespace std;

class RangeFilter
{
    bloom_filter bf;
    char last_char;
    char init_char;
    char is_leaf_char;
    int max_length;

public:

    RangeFilter(vector<string> dataset, int size, double fpr, bool do_print = false)
    {
        last_char = (char)0;
        init_char = (char)127;
        max_length = 0;

        long long total_num_chars = 0;

        int char_count[127];
        memset(char_count, 0, sizeof(char_count));

        for(int i = 0;i<(int)dataset.size();i++)
        {
            max_length = max(max_length, (int)dataset[i].size());
            total_num_chars+=(int)dataset[i].size();
            for(int j = 0;j<(int)dataset[i].size();j++)
            {
                last_char = (char)max((int)last_char, (int)dataset[i][j]);
                init_char = (char)min((int)init_char, (int)dataset[i][j]);
                char_count[(int)dataset[i][j]] += 1;
            }
        }

        int num_unique_chars = 0;
        for(int i = 0;i<127;i++)
        {
            if(char_count[i] != 0)
            {
                num_unique_chars+=1;
            }
        }

        is_leaf_char = (char)((int)init_char-1);

        bloom_parameters parameters;

        // How many elements roughly do we expect to insert?
        parameters.projected_element_count = size;

        // Maximum tolerable false positive probability? (0,1)
        parameters.false_positive_probability = fpr; // 1 in 10000

        // Simple randomizer (optional)
        parameters.random_seed = 0xA5A5A5A5;


        if(do_print) {
            cout << "RANGE FILTER STATS" << endl;
            cout << "num_strings " << (int) dataset.size() << endl;
            cout << "num_chars " << total_num_chars << endl;
            cout << "init_char " << init_char << endl;
            cout << "last_char " << last_char << endl;
            cout << "last_char-init_char+1 " << last_char - init_char + 1 << endl;
            cout << "|unique_chars| " << num_unique_chars << endl;
            cout << "is_leaf_char " << is_leaf_char << endl;
            cout << "expected total queries " << parameters.projected_element_count << endl;
        }


        if (!parameters)
        {
            std::cout << "Error - Invalid set of bloom filter parameters!" << std::endl;
            assert(false);
            return ;
        }


        parameters.compute_optimal_parameters();

        //Instantiate Bloom Filter
        bf = bloom_filter(parameters);

        // Insert into Bloom Filter
        if(true){
            // Insert some strings
            long long num_inserted = 0;
            for (std::size_t i = 0; i < dataset.size(); ++i) {
                for(size_t j = 0;j<dataset[i].size();j++) {
                    num_inserted += 1;
                    string substr = dataset[i].substr(0, j+1);
//                    cout << substr << endl;
                    bf.insert(substr);
                    if ((num_inserted+1) % 100000000 == 0) {
                        cout << "inserted(chars) " << num_inserted+1 << "/" << total_num_chars << endl;
                    }
                }
                string leaf = dataset[i]+is_leaf_char;
                num_inserted += 1;
//                cout << leaf << endl;
                bf.insert(leaf);
                if ((i+1) % 10000000 == 0) {
                    cout << "inserted(strings) " << i+1 << "/" << dataset.size() << endl;
                }
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

    bool query(string left, string right)
    {
        assert(str_invariant(left));
        assert(str_invariant(right));

        int max_n = max(max_length, (int)max(left.size(), right.size()));

        if((int)left.size()<max_n)
            left[left.size()-1]-=1;
        left = pad(left, max_n, last_char);
        right = pad(right, max_n, init_char);

        assert(left.size() == right.size());
        assert(left.size() == max_n);

        assert(left <= right);
        int n = left.size();
        int id = 0;
        string prefix;

        //check common substring
        //e.g left = aaabbb, right = aaaqqq
        //then this checks up to 'aaa'
        while(left[id] == right[id] && id < n) {
            prefix += left[id];
            if (!bf.contains(prefix)) {
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
            if(bf.contains(local_prefix))
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
            if (bf.contains(left_prefix)) {
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
                        if (bf.contains(local_prefix)) {
                            breakpoint(true);
                            return true;
                        }
                    }
                    //check boundary character
                    //aaabb
                    left_prefix += left[left_id];
                    if (bf.contains(left_prefix)) {
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
            if (bf.contains(right_prefix)) {
                if(bf.contains(right_prefix+is_leaf_char))
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
                        if (bf.contains(local_prefix)) {
                            breakpoint(true);
                            return true;
                        }
                    }
                    //check boundary character
                    //aaabb
                    right_prefix += right[right_id];
                    if (bf.contains(right_prefix)) {
                        continue_right = true;
                        if(bf.contains(right_prefix+is_leaf_char))
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


    int get_memory() {
        return bf.get_memory();
    }
};

#include "Trie.h"

void eval_trie_vs_rf()
{
    vector<string> dataset;
    dataset.emplace_back("aaabbb");
    dataset.emplace_back("aaaqqq");
    dataset.emplace_back("aaazzz");
    dataset.emplace_back("aaaff");
    dataset.emplace_back("ccc");



    Trie trie(dataset);
    RangeFilter rf(dataset, 20, 0.00001);

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

#include "surf/surf_implementation.h"

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

class RangeFilterStats
{
public:
    int input_size;
    float seed_fpr;
    int num_keys;
    int num_queries;
    int num_false_positives;
    int num_negatives;
    int memory;
    RangeFilterStats(
            int _input_size,
            float _seed_fpr,
            int _num_keys,
            int _num_queries,
            int _num_false_positives,
            int _num_negatives,
            int _memory):
            input_size(_input_size),
            seed_fpr(_seed_fpr),
            num_keys(_num_keys),
            num_queries(_num_queries),
            num_false_positives(_num_false_positives),
            num_negatives(_num_negatives),
            memory(_memory){}

    string to_string() {
        string ret;
        ret += "size\t"+std::to_string(input_size)+"\tseed_fpr\t"+std::to_string(seed_fpr)+"\tbits_per_key\t"+std::to_string(bits_per_key())+"\tfalse_positive_rate\t"+std::to_string(false_positive_rate());
        return ret;
    }

    double false_positive_rate() const
    {
        return (double)num_false_positives/num_negatives*100.0;
    }

    double bits_per_key()
    {
        return (double)memory/num_keys*100.0;
    }
};

bool contains(const vector<string>& dataset, string left, string right)
{
    auto at = lower_bound(dataset.begin(), dataset.end(), left);
    if(at == dataset.end())
    {
        return false;
    }
    if(*at > right)
    {
        return false;
    }

    return true;

}

RangeFilterStats test_range_filter(const vector<string>& dataset, const vector<pair<string, string> >& workload, int size, float seed_fpr, Trie* trie)
{

    RangeFilter rf = RangeFilter(dataset, size, seed_fpr);

    int num_positive = 0;
    int num_negative = 0;
    int num_false_positives = 0;
    int num_false_negatives = 0;

    for(size_t i = 0;i<workload.size();i++)
    {
        string left_key = workload[i].first;
        string right_key = workload[i].second;

        bool ground_truth = contains(dataset, left_key, right_key);
        if(trie != nullptr)
        {
            assert(trie->query(left_key, right_key) == ground_truth);
        }
        bool prediction = rf.query(left_key, right_key);

        if(ground_truth)
        {
            num_positive+=1;
            if(!prediction)
            {
                num_false_negatives+=1;
            }
        }
        else
        {
            num_negative+=1;
            if(prediction)
            {
                num_false_positives +=1;
            }
        }
    }
//
//    cout << "num_positive " << num_positive << endl;
//    cout << "num_negative " << num_negative << endl;
//    cout << "fpr " << (float)num_false_positives/num_negative*100.0 << "%" << endl;
    assert(num_false_negatives == 0);
//
//    cout << rf.get_memory() << " bytes" << endl;
//    cout << rf.get_memory()*8/dataset.size()*100.0 << " bits/key" << endl;

    RangeFilterStats ret(
            size,
            seed_fpr,
            (int)dataset.size(),
            (int)workload.size(),
            num_false_positives,
            num_negative,
            (int)rf.get_memory());

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

void prep_dataset_and_workload(const string& file_path)
{

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

    for(size_t i = 0;i<workload_seed.size();i++)
    {
        string left_key = workload_seed[i];
        string right_key = left_key;
        right_key[right_key.length() - 1]++;

        workload.emplace_back(make_pair(left_key, right_key));

        if((i+1)%10000000 == 0)
        {
            cout << "num_workloads converted " << i+1 << endl;
        }

    }


    cout << "workload size " << workload_seed.size() << endl;
    cout << "dataset size " << dataset.size() << endl;
    cout << "num_bf_inserts" << num_bf_inserts << endl;

    int num_negatives = get_num_negatives(dataset, workload);

    cout << "num_negatives " << num_negatives << endl;
    cout << "num_positives " << workload.size() - num_negatives << endl;
    cout << "num_negatives(%) " << (float)num_negatives/(float)workload.size()*100.0 << endl;
}

int test_surf()
{

    string file_folder = "";
    string file_name = "5M_dataset.txt";

    prep_dataset_and_workload(file_folder+file_name);

    //todo

}

int main() {
    string file_folder = "";
    string file_name = "80M_dataset.txt";

//    string file_folder = "/home/kliment/Downloads/";
//    string file_name = "emails-validated-random-only-30-characters.txt.sorted";

    string file_path = file_folder + file_name;

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

    for(size_t i = 0;i<workload_seed.size();i++)
    {
        string left_key = workload_seed[i];
        string right_key = left_key;
        right_key[right_key.length() - 1]++;

        workload.emplace_back(make_pair(left_key, right_key));

        if((i+1)%10000000 == 0)
        {
            cout << "num_workloads converted " << i+1 << endl;
        }

    }


    cout << "workload size " << workload_seed.size() << endl;
    cout << "dataset size " << dataset.size() << endl;
    cout << "num_bf_inserts" << num_bf_inserts << endl;

    ofstream output_file("results.out");

//    Trie trie = Trie(dataset);

    Trie* trie_p = nullptr;

    int num_negatives = get_num_negatives(dataset, workload);

    cout << "num_negatives " << num_negatives << endl;
    cout << "num_positives " << workload.size() - num_negatives << endl;
    cout << "num_negatives(%) " << (float)num_negatives/(float)workload.size()*100.0 << endl;


//    int init_size = (int) dataset.size()*3;
//    for(int size = init_size; size<init_size*30;size*=sqrt(2))
//    {
        const int num_seed_fprs = 11;
        float seed_fprs[num_seed_fprs] = {
//                0.00001, 0.0001,
//                0.001, 0.005,
                0.01,
                0.05,
                0.1,
                0.2,
                0.3,
                0.4,
                0.5,
                0.6,
                0.7,
                0.8,
                0.9,
                //0.95, 0.99
        };
        for(int seed_fpr_id = 0; seed_fpr_id < num_seed_fprs; seed_fpr_id++) {
            RangeFilterStats ret = test_range_filter(dataset, workload, num_bf_inserts, seed_fprs[seed_fpr_id], trie_p);
            output_file << ret.to_string() << endl;
        }
        output_file << endl;
//    }

    output_file.close();

    return 0;

}

int old_main() {

//    eval_trie_vs_rf();
//    return 0;

//    string file_folder = "/home/kliment/Downloads/";
//    string file_name = "emails-validated-random-only-30-characters.txt.sorted";

    string file_folder = "";
    string file_name = "small_dataset.txt";

    string file_path = file_folder + file_name;

    vector<string> dataset = read_dataset(file_path);

    shuffle(dataset.begin(), dataset.end(), std::default_random_engine(0));

    int num_elements = 1000;
    int num_empty_range_queries = (num_elements-1);
    int num_empty_point_queries = (num_elements)*2;
    int num_one_element_queries = num_elements*3;
    int num_prefix_queries = num_elements;
    int total_queries = num_empty_range_queries+num_empty_point_queries+num_one_element_queries+num_prefix_queries;

    assert(dataset.size() >= num_elements);

    vector<string> subdataset;

    for(int i = 0;i<num_elements; i++)
    {
        subdataset.push_back(dataset[i]);
    }

    sort(subdataset.begin(), subdataset.end());

    bool output_subdataset = false;
    if(output_subdataset) {
        ofstream out_file("small_dataset.txt");
        for (int i = 0; i < (int)subdataset.size(); i++) {
            out_file << subdataset[i] << endl;
        }
        out_file.close();
        return 0;
    }

    cout << "building rf" << endl;

//    936483776  total_queries*10, 0.00001
//    93705348  total_queries, 0.00001
//    74963057  total_queries, 0.0001
//    56214185  total_queries, 0.001
//    37506878  total_queries, 0.01
//    3768696  total_queries/10, 0.01
//    394878  total_queries/100, 0.01
//    207443  total_queries/200, 0.01
//    148535  total_queries/300, 0.01
//    126224  total_queries/300, 0.02 3/999
//    113022  total_queries/300, 0.03 20/999
//    300450  total_queries/100, 0.03
//    216977  total_queries/100, 0.08 7/999

    RangeFilter rf(subdataset, total_queries/300, 0.02);
    Trie trie(subdataset);

    bool assert_exact = false;

    assert(num_empty_range_queries == subdataset.size()-1);
    int false_positive_empty_range_q = 0;
    if(true)
    for(int i = 0;i<num_empty_range_queries; i++)
    {
        string left = subdataset[i];
        string right = subdataset[i+1];
        left+='a';
        right[right.size()-1]-=1;
        bool ret = rf.query(left,right);
//        cout << i <<" "<< ret << endl;
        false_positive_empty_range_q += ret;
        if(assert_exact)assert(ret == trie.query(left, right));

    }
    cout << "empty_ranges" << endl;
    cout << false_positive_empty_range_q <<" / " << num_empty_range_queries << endl;

    assert(num_empty_point_queries == subdataset.size()*2);
    int false_positive_empty_point_queries = 0;
    if(true)
    for(int i = 0;i<num_empty_point_queries/2; i++)
    {
        string left = subdataset[i];
        string right = subdataset[i];
        left+='a';
        right[right.size()-1]-=1;
        bool ret = rf.query(left,left);
//        cout << 2*i <<" "<< ret << endl;
        false_positive_empty_point_queries += ret;
        if(assert_exact)assert(ret == trie.query(left, left));

        ret = rf.query(right,right);
//        cout << 2*i+1 <<" "<< ret << endl;
        false_positive_empty_point_queries += ret;
        if(assert_exact)assert(ret == trie.query(right, right));

    }

    cout << "empty_points" << endl;
    cout << false_positive_empty_point_queries <<" / " << num_empty_point_queries << endl;


    assert(num_one_element_queries == 3*subdataset.size());
    int true_positive_one_element_queries = 0;
    for(int i = 0; i<num_one_element_queries/3; i++)
    {
        string left = subdataset[i];
        string right = subdataset[i];
        right+='a';
        left[left.size()-1]-=1;
        bool ret = rf.query(left,right);
//        cout << 3*i <<" "<< ret << endl;
        true_positive_one_element_queries += ret;
        if(true)assert(ret == trie.query(left, right));

        ret = rf.query(subdataset[i],subdataset[i]);
//        cout << 3*i+1 <<" "<< ret << endl;
        true_positive_one_element_queries += ret;
        if(true)assert(ret == trie.query(subdataset[i], subdataset[i]));

        if(i >= 1 && i < (int)subdataset.size()-1)
        {
            left = subdataset[i-1];
            right = subdataset[i+1];
            left+='a';
            right[right.size()-1]-=1;
            ret = rf.query(left, right);
//            cout << 3*i+2 << ret << endl;
            true_positive_one_element_queries+=1;
            assert(ret == trie.query(left, right));
        }
        else if(i == 0)
        {

            left = rf.get_init_char();
            right = subdataset[i+1];
            right[right.size()-1]-=1;
            ret = rf.query(left, right);
//            cout << 3*i+2 << ret << endl;
            true_positive_one_element_queries+=1;
            assert(ret == trie.query(left, right));
        }
        else if(i == (int)subdataset.size()-1)
        {

            left = subdataset[i-1];
            right = rf.get_last_char();
            while(right.size() < left.size())
            {
                right += rf.get_last_char();
            }
            left+='a';
            ret = rf.query(left, right);
//            cout << 3*i+2 << ret << endl;
            true_positive_one_element_queries+=1;
            assert(ret == trie.query(left, right));
        }

    }

    cout << "one_element_queries" << endl;
    cout << true_positive_one_element_queries <<" / " << num_one_element_queries << endl;

    int positive_prefixes = 0;
    for(int i = 0;i<(int)subdataset.size();i++)
    {
        bool ret = rf.query(subdataset[0], subdataset[i]);
        positive_prefixes+=ret;
        assert(trie.query(subdataset[0], subdataset[i]) == ret );
    }

    cout << "positive_prefixes " << positive_prefixes << endl;

    cout << rf.get_memory() << endl;
    cout << "sizeof(unsigned char) " << sizeof(unsigned char) << endl;


    return 0;
}

