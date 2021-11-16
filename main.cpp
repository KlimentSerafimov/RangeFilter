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

    RangeFilter(vector<string> dataset, int num_queries, double fpr)
    {
        last_char = (char)0;
        init_char = (char)127;
        max_length = 0;

        long long total_num_chars = 0;

        int char_count[127];
        memset(char_count, 0, sizeof(char_count));

        for(int i = 0;i<dataset.size();i++)
        {
            max_length = max(max_length, (int)dataset[i].size());
            total_num_chars+=(int)dataset[i].size();
            for(int j = 0;j<dataset[i].size();j++)
            {
                last_char = (char)max((int)last_char, (int)dataset[i][j]);
                init_char = (char)min((int)init_char, (int)dataset[i][j]);
                char_count[dataset[i][j]] += 1;
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

        cout << "num_strings " << dataset.size() << endl;
        cout << "num_chars " << total_num_chars << endl;
        cout << "init_char " << init_char << endl;
        cout << "last_char " << last_char << endl;
        cout << "last_char-init_char+1 " << last_char-init_char+1 << endl;
        cout << "|unique_chars| " << num_unique_chars << endl;

        is_leaf_char=init_char-1;
        cout << "is_leaf_char " << is_leaf_char << endl;

        bloom_parameters parameters;

        // How many elements roughly do we expect to insert?
        parameters.projected_element_count = total_num_chars + num_queries*(last_char-init_char)*max_length*2;
        cout << "expected total queries " << parameters.projected_element_count << endl;

        // Maximum tolerable false positive probability? (0,1)
        parameters.false_positive_probability = fpr; // 1 in 10000

        // Simple randomizer (optional)
        parameters.random_seed = 0xA5A5A5A5;

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
                for(int j = 0;j<dataset[i].size();j++) {
                    num_inserted += 1;
                    string substr = dataset[i].substr(0, j+1);
                    cout << substr << endl;
                    bf.insert(substr);
                    if (num_inserted % 100000000 == 0) {
                        cout << "inserted(chars) " << num_inserted << "/" << total_num_chars << endl;
                    }
                }
                string leaf = dataset[i]+is_leaf_char;
                num_inserted += 1;
                cout << leaf << endl;
                bf.insert(leaf);
                if (num_inserted % 100000000 == 0) {
                    cout << "inserted(chars) " << num_inserted << "/" << total_num_chars << endl;
                }
                if (i % 10000000 == 0) {
                    cout << "inserted(strings) " << i << "/" << dataset.size() << endl;
                }
            }
        }
    }

    bool str_invariant(string q)
    {
        bool ret = true;
        for(int i = 0;i<q.size();i++)
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
        while(s.size() < num)
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

        if(left.size()<max_n)
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
                while (left_id < left.size()) {
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
                while (right_id < right.size()) {
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

void small_example()
{
    vector<string> dataset;
    dataset.push_back("aaabbb");
    dataset.push_back("aaaqqq");
    dataset.push_back("aaazzz");

    RangeFilter rf(dataset, 10,0.01);

//    cout << rf.query("aaaa", "aaab") << endl;
//    cout << rf.query("aaaa", "aaac") << endl;
//    cout << rf.query("aaabbb", "aaabbb") << endl;
//    cout << rf.query("aaabbc", "aaaqqp") << endl;
//    cout << rf.query("aaaqqp", "aaaqqq") << endl;
//    cout << rf.query("aaaqqq", "aaaqqq") << endl;
//    cout << rf.query("aaaqqq", "aaaqqr") << endl;
//    cout << rf.query("aaaqqp", "aaaqqr") << endl;
//    cout << rf.query("aaaqqr", "aaaqqz") << endl;
//    cout << rf.query("aaaqqr", "aaaqqy") << endl;
//    cout << rf.query("aaaqqr", "aaazzz") << endl;
//    cout << rf.query("aaaqqr", "aaazzy") << endl;
//    cout << rf.query("aaaqqr", "aaazyz") << endl;
//    cout << rf.query("aaaqqr", "aaayzz") << endl;
//    cout << rf.query("aaaqqr", "aaayzz") << endl;
//    cout << rf.query("aa", "ab") << endl;
//    cout << rf.query("a", "aaabba") << endl;
//    cout << rf.query("a", "aaabbb") << endl;
//    cout << rf.query("a", "aaazzz") << endl;
//    cout << rf.query("aaabbc", "aaabbc") << endl;
//    cout << rf.query("aaaab", "aaaz") << endl;
//    cout << rf.query("aaac", "aaaz") << endl;
//    cout << rf.query("aac", "aacz") << endl;


}

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

int main() {

//    small_example();
//    eval_trie_vs_rf();
//    return 0;

//    string file_folder = "/home/kliment/Downloads/";
//    string file_name = "emails-validated-random-only-30-characters.txt.sorted";


    string file_folder = "";
    string file_name = "small_dataset.txt";

    string file_path = file_folder + file_name;

    vector<string> dataset;

    ifstream file(file_path);

    int id = 0;
    while (!file.eof())
    {
        string line;
        getline(file, line);
        dataset.push_back(line);
        if(id%10000000 == 0)
        cout << "reading " << id <<" "<< line << endl;
        id++;
    }
    cout << "done" << endl;

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
        for (int i = 0; i < subdataset.size(); i++) {
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

        if(i >= 1 && i < subdataset.size()-1)
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
        else if(i == subdataset.size()-1)
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
    for(int i = 0;i<subdataset.size();i++)
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

