//
// Created by kliment on 2/26/22.
//

#include <random>
#include "DatasetAndWorkload.h"
#include "RangeFilterTemplate.h"

RangeFilterStats
DatasetAndWorkload::test_range_filter(RangeFilterTemplate *rf, bool do_print)
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

        bool ground_truth = contains(dataset, left_key, right_key);
        bool prediction = rf->query(left_key, right_key);

        if (ground_truth) {
            num_positive += 1;
            if (!prediction) {
                num_false_negatives += 1;
                assert(false);
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
            rf->get_point_query(),
            (int)dataset.size(),
            (int)workload.size(),
            num_false_positives,
            num_negative,
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

int DatasetAndWorkload::prep_dataset_and_workload(const string& file_path, const string& workload_difficulty, int impossible_depth)
{

    vector<string> workload_seed;
    vector<string> workload_seed_and_dataset;

    workload_seed_and_dataset = read_dataset(file_path);

//    workload_seed_and_dataset.emplace_back("aaabbb");
//    workload_seed_and_dataset.emplace_back("aaaqqq");
//    workload_seed_and_dataset.emplace_back("aaazzz");
//    workload_seed_and_dataset.emplace_back("aaaff");
//    workload_seed_and_dataset.emplace_back("ccc");

    cout << "workload_seed_and_dataset.size() " << workload_seed_and_dataset.size() << endl;

    cout <<"shuffling" << endl;
    shuffle(workload_seed_and_dataset.begin(), workload_seed_and_dataset.end(), std::default_random_engine(0));
    cout <<"done shuffling" << endl;
    cout << "splitting" << endl;

    char init_char = (char)127;



    for (size_t i = 0; i < workload_seed_and_dataset.size(); i++) {
        if (i < workload_seed_and_dataset.size() / 2
//        && workload_difficulty != "impossible"
        ) {
            workload_seed.push_back(workload_seed_and_dataset[i]);
        } else {
            dataset.push_back(workload_seed_and_dataset[i]);
            assert(!dataset.rbegin()->empty());
            for(size_t j = 0; j < workload_seed_and_dataset[i].size();j++){
                init_char = min(init_char, (char)workload_seed_and_dataset[i][j]);
            }
        }
    }

    cout << "done splitting" << endl;

    workload_seed_and_dataset.clear();

    cout << "sorting" << endl;

    sort(workload_seed.begin(), workload_seed.end());
    sort(dataset.begin(), dataset.end());

    cout << "done sorting"<< endl;

    string midpoint1 = "";
    string midpoint2 = "";

    if(workload_difficulty == "hybrid")
    {
        midpoint1 = dataset[dataset.size()/3];
        midpoint2 = dataset[2*dataset.size()/3];
        cout << "midpoint1: " << midpoint1 << endl;
        cout << "midpoint2: " << midpoint2 << endl;
    }

    if(workload_difficulty == "easy" || workload_difficulty == "medium" || workload_difficulty == "hard" || workload_difficulty == "hybrid") {
        for (size_t i = 0; i < workload_seed.size() - 1; i++) {
            string local_workload_difficulty = workload_difficulty;

            if(local_workload_difficulty == "hybrid")
            {
                if(workload_seed[i] < midpoint1)
                {
                    local_workload_difficulty = "hard";
                }
                else if(workload_seed[i] < midpoint2)
                {
                    local_workload_difficulty = "easy";
                }
                else
                {
                    local_workload_difficulty = "hard";
                }
            }

            if (local_workload_difficulty == "easy") {
                //eeasy workload
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
                    left_key += init_char;
                    assert(right_key[right_key.size() - 1] >= 1);
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
        }
    }
    else {
        assert(false);
    }
}

RangeFilterStats DatasetAndWorkload::eval_point_query(PointQuery *pq) {
    RangeFilterTemplate* rf = new RangeFilterTemplate(*this, pq);
    RangeFilterStats rez = test_range_filter(rf);
    return rez;
}

int DatasetAndWorkload::get_max_length_of_dataset() {
    int ret = 0;
    for(int i = 0;i<dataset.size();i++) {
        ret = max(ret, (int)dataset[i].size());
    }
    return ret;
}
