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
                if (num_inserted % 10000000 == 0) {
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

    string pad(string s, char c) const
    {
        while(s.size() < max_length)
        {
            s+=c;
        }
        s = s.substr(0, max_length);
        return s;
    }

    void breakpoint(bool ret)
    {
        cout << "ret" << endl;
    }

    bool query(string left, string right)
    {
        cout << left << " " << right << " ";
        assert(str_invariant(left));
        assert(str_invariant(right));

        left = pad(left, init_char);
        right = pad(right, init_char);

        assert(left.size() == right.size());
        assert(left.size() == max_length);

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

int main() {

//    small_example();
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
    int num_empty_point_queries = (num_elements+1)*2;
    int num_one_element_queries = (num_elements)*2;
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

    RangeFilter rf(subdataset, total_queries*10, 0.000001);

    assert(num_empty_range_queries == subdataset.size()-1);
    int false_positive_empty_range_q = 0;
    for(int i = 0;i<num_empty_range_queries; i++)
    {
        string left = subdataset[i];
        string right = subdataset[i+1];
        left+='z';
        right[right.size()-1]-=1;
        bool ret = rf.query(left,right);
        cout << i <<" "<< ret << endl;
        false_positive_empty_range_q += ret;

    }
    cout << false_positive_empty_range_q <<" / " << num_empty_range_queries << endl;

    return 0;
}

