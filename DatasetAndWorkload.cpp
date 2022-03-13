//
// Created by kliment on 2/26/22.
//

#include <random>
#include "DatasetAndWorkload.h"
#include "RangeFilterTemplate.h"

const RangeFilterScore *
DatasetAndWorkload::test_range_filter(RangeFilterTemplate *rf, bool do_print) const
{
    int num_positive = 0;
    int num_negative = 0;
    int num_false_positives = 0;
    int num_false_negatives = 0;

    int true_positives = 0;
    int true_negatives = 0;

    for(size_t i = 0;i<workload.size();i++) {
        string left_key = workload[i].first;
        string right_key = workload[i].second;

        bool ground_truth = dataset.contains(left_key, right_key);
        bool prediction = rf->query(left_key, right_key);

        if (ground_truth) {
            num_positive += 1;
            if (!prediction) {
                num_false_negatives += 1;
            } else {
                true_positives += 1;
            }
        } else {
            num_negative += 1;
            if (prediction) {
                num_false_positives += 1;
            } else {
                true_negatives += 1;
            }
        }
    }


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

    if(num_false_negatives >= 1) {
        cout << "ERROR: NUM FALSE POSITIVES >= 1!!!" << endl;
        num_false_positives = num_negative;
    }

    const RangeFilterScore* ret = new RangeFilterScore(
            rf->get_point_query(),
            (int)dataset.size(),
            (int)workload.size(),
            num_false_positives,
            num_negative,
            num_false_negatives,
            (int)rf->get_memory()*8);

    return ret;
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

void DatasetAndWorkload::prep_dataset_and_workload(
        const DatasetAndWorkloadSeed& dataset_and_workload_seed,
        const string& workload_difficulty,
        const int impossible_depth) {

    assert(impossible_depth >= 2);

    const Dataset& workload_seed = dataset_and_workload_seed.get_workload_seed();
    char init_char = dataset_and_workload_seed.get_init_char();

    string midpoint1 = "";
    string midpoint2 = "";

    if(workload_difficulty == "hybrid")
    {
//        midpoint1 = dataset[2*dataset.size()/5];
//        midpoint2 = dataset[4*dataset.size()/5];
        midpoint1 = dataset[dataset.size()/3];
        midpoint2 = dataset[2*dataset.size()/3];
        cout << "midpoint1: " << midpoint1 << endl;
        cout << "midpoint2: " << midpoint2 << endl;
    }

    if(workload_difficulty == "easy" || workload_difficulty == "medium" || workload_difficulty == "hard" || workload_difficulty == "hybrid") {
        for (size_t i = 0; i < workload_seed.size() - 1; i++) {
            string local_workload_difficulty = workload_difficulty;

            if(local_workload_difficulty == "hybrid") {
                if (workload_seed[i] < midpoint1) {
                    local_workload_difficulty = "hard";
                } else if (workload_seed[i] < midpoint2) {
                    local_workload_difficulty = "easy";
                } else {
                    local_workload_difficulty = "hard";
                }
            }

            if (local_workload_difficulty == "easy") {
                //easy workload
                string left_key = workload_seed[i];
                string right_key = left_key;
                right_key[(int) right_key.size() - 1] += (char) 1;
                workload.emplace_back(make_pair(left_key, right_key));
            } else if (local_workload_difficulty == "medium") {
                //medium workload
                string left_key = workload_seed[i];
                string right_key = left_key;
                right_key[(int) right_key.size() / 4] += (char) 1;

                workload.emplace_back(make_pair(left_key, right_key));
            } else if (local_workload_difficulty == "hard") {
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
                    assert(left_key == right_key);
                    continue;
                }

                if(impossible_depth == -1) {
                    assert(false);
                    left_key += init_char;
                    assert(right_key[right_key.size() - 1] >= 1);
                    right_key[right_key.size() - 1] -= 1;
                    assert(left_key < right_key);
                }
                else
                {
                    assert(impossible_depth >= 1);
                    const size_t impossible_id = impossible_depth-1;
                    bool enter = false;
                    if(impossible_id >= left_key.size()) {
                        left_key+=init_char;
                        enter = true;
                    }
                    else {
                        left_key[impossible_id]++;
                        enter = true;
                    }
                    assert(enter);
                    enter = false;
                    if(impossible_id >= right_key.size()) {
                        right_key[right_key.size() - 1] -= 1;
                        enter = true;
                    }
                    else {
                        right_key[impossible_id]--;
                        enter = true;
                    }
                    assert(enter);
                    enter = false;
                }
                if(left_key <= right_key) {
                    workload.emplace_back(make_pair(left_key, right_key));
                }
            } else {
                assert(false);
            }
        }
    }
    else {
        assert(false);
    }

    for(size_t i = 1;i<dataset.size();i++)
    {
        assert(dataset[i-1] <= dataset[i]);
    }

}

int Dataset::reads_from_cache = 0;
int Dataset::total_reads = 0;
int Dataset::cache_read_attempts = 0;

void Dataset::remove_duplicates() {
    size_t at_left = 0;
    size_t at_right = 0;
    while(at_left < this->size()) {
        this->at(at_right) = this->at(at_left);
        at_left++;
        for (; at_left < this->size() && this->at(at_left) == this->at(at_right); at_left++);
        at_right++;
    }

    while(size() > at_right) {
        //NOT TESTED
        pop_back();
    }

    for(size_t i = 1;i<size();i++) {
        assert(at(i-1) != at(i));
        assert(at(i-1) < at(i));
    }
}

int Dataset::get_num_inserts_in_point_query(size_type cutoff) const {
    assert(ground_truth_point_query != nullptr);
    return ground_truth_point_query->get_num_prefixes(cutoff);
}

void Dataset::set_ground_truth_point_query(const MemorizedPointQuery *_ground_truth_point_query) {
    assert(ground_truth_point_query == nullptr);
    ground_truth_point_query =_ground_truth_point_query;
}

size_t Dataset::get_max_length() {
    size_t max_length = 0;
    for(auto it: *this) {
        max_length = max(max_length, it.size());
    }
    return max_length;
}

const RangeFilterScore * DatasetAndWorkload::eval_point_query(PointQuery *pq) const {
    RangeFilterTemplate* rf = new RangeFilterTemplate(*this, pq);
    return test_range_filter(rf);
}

int DatasetAndWorkload::get_max_length_of_dataset() const{
    size_t ret = 0;
    for(size_t i = 0;i<dataset.size();i++) {
        ret = max(ret, dataset[i].size());
    }
    assert(ret == max_length_of_key);
    return ret;
}

const Workload &DatasetAndWorkload::get_negative_workload_assert_has() const {
    assert(negative_workload_defined);
    return negative_workload;
}


void DatasetAndWorkloadSeed::prep_dataset_and_workload_seed(const string &file_path, const string &workload_difficulty,
                                                            int impossible_depth){
    vector<string> workload_seed_and_dataset;

    workload_seed_and_dataset = read_dataset(file_path);

//    workload_seed_and_dataset.emplace_back("aaabbb");
//    workload_seed_and_dataset.emplace_back("aaaqqq");
//    workload_seed_and_dataset.emplace_back("aaazzz");
//    workload_seed_and_dataset.emplace_back("aaaff");
//    workload_seed_and_dataset.emplace_back("ccc");

    cout << "workload_seed_and_dataset.size() " << workload_seed_and_dataset.size() << endl;

//    cout <<"shuffling" << endl;
    shuffle(workload_seed_and_dataset.begin(), workload_seed_and_dataset.end(), std::default_random_engine(0));
//    cout <<"done shuffling" << endl;
//    cout << "splitting" << endl;

    for (size_t i = 0; i < workload_seed_and_dataset.size(); i++) {
        if (i < workload_seed_and_dataset.size() / 2 && workload_difficulty != "impossible") {
            workload_seed.push_back(workload_seed_and_dataset[i]);
        } else {
            dataset.push_back(workload_seed_and_dataset[i]);
            assert(!dataset.rbegin()->empty());
            for(size_t j = 0; j < workload_seed_and_dataset[i].size();j++){
                init_char = min(init_char, (char)workload_seed_and_dataset[i][j]);
            }
        }
    }

//    cout << "done splitting" << endl;

    workload_seed_and_dataset.clear();

//    cout << "sorting" << endl;

    sort(workload_seed.begin(), workload_seed.end());
    sort(dataset.begin(), dataset.end());

//    cout << "done sorting"<< endl;

    for(size_t i = 1;i<dataset.size();i++)
    {
        assert(dataset[i-1] <= dataset[i]);
    }
}

const Dataset &DatasetAndWorkloadSeed::get_dataset() const {
    return dataset;
}

vector<DatasetAndWorkloadSeed> DatasetAndWorkloadSeed::split_workload_seeds(int num_splits) {
    assert(false);//WHY USE THIS?
    vector<DatasetAndWorkloadSeed> ret;
    Dataset local_workload_seed;

    shuffle(workload_seed.begin(), workload_seed.end(), std::default_random_engine(0));

    size_t size_of_bucket = workload_seed.size()/num_splits;

    for(size_t i = 0;i<workload_seed.size();i++)
    {
        local_workload_seed.push_back(workload_seed[i]);
        if(local_workload_seed.size() == size_of_bucket && workload_seed.size()-i-1 >= size_of_bucket) {
            sort(local_workload_seed.begin(), local_workload_seed.end());
            ret.emplace_back(dataset, local_workload_seed);
            local_workload_seed.clear();
        }
    }

    if(!local_workload_seed.empty()) {
        assert(local_workload_seed.size() >= size_of_bucket);
        sort(local_workload_seed.begin(), local_workload_seed.end());
        ret.emplace_back(dataset, local_workload_seed);
        local_workload_seed.clear();
    }

    return ret;

}

void DatasetAndWorkloadSeed::clear() {
    workload_seed.clear();
}


vector<DatasetAndWorkload> DatasetAndWorkload::split_workload(int num_splits) {
    vector<DatasetAndWorkload> ret;
    Workload local_workload_seed;

    shuffle(workload.begin(), workload.end(), std::default_random_engine(0));

    size_t size_of_bucket = workload.size()/num_splits;

    for(size_t i = 0;i<workload.size();i++)
    {
        local_workload_seed.push_back(workload[i]);
        if(local_workload_seed.size() == size_of_bucket && workload.size()-i-1 >= size_of_bucket) {
            sort(local_workload_seed.begin(), local_workload_seed.end());
            ret.emplace_back(dataset, local_workload_seed, *this);
            local_workload_seed.clear();
        }
    }

    if(!local_workload_seed.empty()) {
        assert(local_workload_seed.size() >= size_of_bucket);
        sort(local_workload_seed.begin(), local_workload_seed.end());
        ret.emplace_back(dataset, local_workload_seed, *this);
        local_workload_seed.clear();
    }

    sort(workload.begin(), workload.end());

    return ret;
}

void DatasetAndWorkload::build_ground_truth_range_filter() {

    assert(ground_truth_range_filter == nullptr);

    get_negative_workload();

    GroundTruthPointQuery *ground_truth_point_query = new GroundTruthPointQuery();
    ground_truth_range_filter =
            new RangeFilterTemplate(*this, ground_truth_point_query, false);

    dataset.set_ground_truth_point_query(ground_truth_point_query);
}

const RangeFilterTemplate &DatasetAndWorkload::get_ground_truth_range_filter() {
    return *ground_truth_range_filter;
}


string DatasetAndWorkload::stats_to_string()  {
    //#num_keys, #num_prefixes, #cutoff, base
    //#n.r.q,
    //#n.p.q, #unique n.p.q

    assert(ground_truth_range_filter != nullptr);
    assert(ground_truth_range_filter->is_cold());
    ground_truth_range_filter->build_heatmap(*this);

    size_t cutoff = ground_truth_range_filter->get_cutoff();
    assert(_impossible_depth >= 0);
    assert(cutoff == (size_t)_impossible_depth);

    assert((int)get_unique_chars().size() == max_char-min_char+1);

    string ret;
    ret +=
            "num_keys " + std::to_string(dataset.size()) + " " +
            "num_prefixes " + std::to_string(((MemorizedPointQuery*)ground_truth_range_filter->get_point_query())->get_num_prefixes(cutoff)) + " " +
            "cutoff " + std::to_string(cutoff) + " " +
            "char_range_size " + std::to_string(max_char-min_char+1) + " " +
            "num_nrqs " + std::to_string(get_negative_workload_assert_has().size()) + " " +
            "num_npqs " + std::to_string(ground_truth_range_filter->get_negative_point_queries()) + " " +
            "num_unique_npqs " + std::to_string(ground_truth_range_filter->get_unique_negative_point_queries());

    cout << ret << endl;

    ret = "";
    ret += std::to_string(dataset.size()) + " " +
           std::to_string(((MemorizedPointQuery*)ground_truth_range_filter->get_point_query())->get_num_prefixes(cutoff)) + " " +
           std::to_string(cutoff) + " " +
           std::to_string(max_char-min_char+1) + " " +
           std::to_string(get_negative_workload_assert_has().size()) + " " +
           std::to_string(ground_truth_range_filter->get_negative_point_queries()) + " " +
           std::to_string(ground_truth_range_filter->get_unique_negative_point_queries());



    return ret;

}

void DatasetAndWorkload::clear()
{
    dataset.clear();
    workload.clear();

    if(ground_truth_range_filter != nullptr)
    {
        ground_truth_range_filter->get_point_query()->clear();
        delete ground_truth_range_filter;
    }

    negative_workload.clear();
}
