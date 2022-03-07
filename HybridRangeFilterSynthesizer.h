//
// Created by kliment on 3/5/22.
//

#ifndef SURF_HYBRIDRANGEFILTERSYNTHESIZER_H
#define SURF_HYBRIDRANGEFILTERSYNTHESIZER_H

#include "PointQuery.h"
#include "Frontier.h"
#include "DatasetAndWorkload.h"
#include "SurfPointQuery.h"
#include "RangeFilterTemplate.h"
#include "RangeFilterOneBloom.h"
#include "RangeFilterMultiBloom.h"
#include "HybridPointQuery.h"
#include "queue"

Frontier<RichMultiBloomParams> *
simulated_annealing(const DatasetAndWorkload &dataset_and_workload, ofstream &frontiers, int meta_iter,
                    Frontier<RichMultiBloomParams> *frontier_p = nullptr);

static int id_ = 0;
class HybridRangeFilterSynthesizer
{

public:
    class PointQueryPointer
    {
        bool cleared = false;
        PointQuery* pq = nullptr;

    public:
        string to_string() const
        {
            assert(pq != nullptr);
            return pq->to_string();
        }

        explicit PointQueryPointer(PointQuery* _pq): pq(_pq) {}

        PointQuery* get_pq() const
        {
            assert(pq != nullptr);
            return pq;
        }

        void clear()
        {
            assert(pq != nullptr);
            pq->clear();
            cleared = true;
        }
    };
private:

    static Frontier<PointQueryPointer>* optimize_base_case_with_surf(const DatasetAndWorkload& dataset_and_workload)
    {
        Frontier<PointQueryPointer>* ret = new Frontier<PointQueryPointer>(2);

        for(int trie_size = 0;trie_size<=64;trie_size++) {
            id_++;
            const vector<string>& dataset = dataset_and_workload.get_dataset();
            const vector<pair<string, string> >& workload = dataset_and_workload.get_workload();
            SurfPointQuery *surf_pq = new SurfPointQuery(dataset, trie_size);
            RangeFilterTemplate *surf_rf = new RangeFilterTemplate(dataset_and_workload, surf_pq);

            test_surf(dataset, workload, trie_size);

            const RangeFilterScore& rez = *dataset_and_workload.test_range_filter(surf_rf);

            if(rez.get_num_false_negatives() == 0) {
                ret->insert(PointQueryPointer(surf_pq), rez.get_score_as_vector());
            }
        }
        cout << "frontier size " << ret->get_size() << endl;
        return ret;
    }

    static Frontier<PointQueryPointer>* optimize_base_case_with_one_bloom(const DatasetAndWorkload& dataset_and_workload)
    {
        cout << "ENTERED optimize_base_case " << dataset_and_workload.get_dataset().size() <<" "<< dataset_and_workload.get_workload().size() << endl;
        Frontier<PointQueryPointer>* ret = new Frontier<PointQueryPointer>(2);

        int _sample_id = 0;
        size_t space = dataset_and_workload.get_max_length()*(36);
        int samples = space;
        float ratio = (float)samples/space;
        int samples_taken = 0;

        for(int cutoff = 1; cutoff < (int)dataset_and_workload.get_max_length(); cutoff++) {
            for(float fpr = 0.000001; fpr < 1; fpr = min(fpr+0.05, fpr*1.5)) {
                _sample_id++;
                if(rand()%1000 <= ratio*1000){
                    samples_taken++;
                }
                else {
                    continue;
                }
                //            cout << "evaluating (sample " << samples_taken << ", _sample_id " << _sample_id << "): " << cutoff <<" " << fpr << endl;
                OneBloom *pq = new OneBloom(dataset_and_workload.get_dataset(), fpr, cutoff);
                const RangeFilterScore& rez = *dataset_and_workload.eval_point_query(pq);
                ret->insert(PointQueryPointer(pq), rez.get_score_as_vector());
            }
        }


        cout << "TOTAL IDS: " << _sample_id << " TOTAL SAMPLES: " << samples_taken << endl;

        ret->print(cout, 10, true);

        cout << "DONE optimize_base_case" << endl << endl;

        return ret;
    }

    static Frontier<PointQueryPointer>* optimize_base_case(
            const DatasetAndWorkload& dataset_and_workload,
            const vector<MultiBloomParams*>* init_frontier = nullptr) {
        Frontier<PointQueryPointer>* ret = nullptr;

        ret = optimize_base_case_with_multi_bloom(dataset_and_workload, init_frontier);

        for(auto it: ret->get_frontier()){
            assert(it->get_params().get_pq()->get_is_score_set());
        }

        return ret;

//        return optimize_base_case_with_surf(dataset_and_workload);
//        return optimize_base_case_with_one_bloom(dataset_and_workload);
    }

public:

    static Frontier<PointQueryPointer>* optimize_base_case_with_multi_bloom(
            const DatasetAndWorkload& dataset_and_workload,
            const vector<MultiBloomParams*>* init_params = nullptr)
    {
        ofstream frontiers = ofstream ("frontiers.out");
        int meta_iter = 1;

        Frontier<RichMultiBloomParams> *frontier = new Frontier<RichMultiBloomParams>(2);

            bool fresh_init = true;

            if (fresh_init) {

                for (int cutoff = 1; cutoff <= (int) dataset_and_workload.get_max_length(); cutoff++) {
                    for (double seed_fpr = 0.000001; seed_fpr <= 0.95; seed_fpr = min(seed_fpr + 0.1, seed_fpr * 10)) {
                        bool do_print = false;
                        RichMultiBloom *pq = new RichMultiBloom(dataset_and_workload, seed_fpr,
                                                                cutoff, do_print);
                        auto *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);
                        const RangeFilterScore &ret = *dataset_and_workload.test_range_filter(rf, do_print);

                        RichMultiBloom *tmp_rmb_pq = (RichMultiBloom *) ret.get_params();
                        RichMultiBloomParams *tmp_params = (tmp_rmb_pq->add_iter_and_epoch(1, 0));
                        tmp_params->set_score(ret);
                        bool inserted = frontier->insert(*tmp_params, ret.get_score_as_vector()) != nullptr;

                        if(inserted)
                        cout << ret.to_string() << endl;
                    }
                }
                cout << "DONE INIT" << endl;
            }
            else {

                string file_name = "frontier_from_file.out";

                ifstream fin(file_name);

                int line_id = 0;
                for (string line; getline(fin, line);) {

                    cout << "READING AND PROCESSING line_id " << line_id++ << endl;
                    cout << line << endl;

                    bool do_print = false;
                    RichMultiBloom *pq = new RichMultiBloom(dataset_and_workload, line, do_print);
                    auto *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);
                    const RangeFilterScore &ret = *dataset_and_workload.test_range_filter(rf, do_print);
                    RichMultiBloom &tmp_rmb_pq = *(RichMultiBloom *) ret.get_params();
                    RichMultiBloomParams &tmp_params = *(tmp_rmb_pq.add_iter_and_epoch(1, 0));
                    assert(false); // NEED TO UPDATE SCORE IN tmp_params TO ret.
                    auto new_point = frontier->insert(tmp_params, ret.get_score_as_vector());
                    cout << "RESULT" << endl;

                    if (new_point == nullptr) {
                        cout << "NULLPTR" << endl;
                        continue;
                    }

                    vector<string> dim_names;
                    dim_names.emplace_back("bpk");
                    dim_names.emplace_back("fpr");
                    string result_line = new_point->to_string(dim_names);
                    cout << result_line << endl;
                    //                assert(RichMultiBloom::split(result_line, "METAPARAMS")[0] == RichMultiBloom::split(line, "METAPARAMS")[0]);
                }

                frontier->print(cout);

                meta_iter = 3;
            }

        if(init_params != nullptr)
        {
            {
                double frontier_area = frontier->get_area();
                LineLength frontier_line_length = frontier->get_line_length();
                frontiers << "|frontier.area|: " << frontier_area << endl;
                cout << "|frontier.area|: " << frontier_area << endl;
                frontiers << "|frontier.square_area|: " << frontier_line_length.to_string() << endl;
                cout << "|frontier.square_area|: " << frontier_line_length.to_string() << endl;
            }
            cout << "INIT FROM CHILDREN" << endl;

            for(auto it: *init_params)
            {
                bool do_print = false;
                RichMultiBloom *pq = new RichMultiBloom(dataset_and_workload, *it, do_print);
                pq->reset_score();
                auto *rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);
                const RangeFilterScore &ret = *dataset_and_workload.test_range_filter(rf, do_print);

                RichMultiBloom *tmp_rmb_pq = (RichMultiBloom *) ret.get_params();
                RichMultiBloomParams *tmp_params = (tmp_rmb_pq->add_iter_and_epoch(1, 0));
                tmp_params->set_score(ret);
                bool inserted = frontier->insert(*tmp_params, ret.get_score_as_vector()) != nullptr;

                if(inserted)
                cout << ret.to_string() << endl;
            }

        }

        cout << "STARTING SIMULATED ANNEALING" << endl;

        {
            double frontier_area = frontier->get_area();
            LineLength frontier_line_length = frontier->get_line_length();
            frontiers << "|frontier.area|: " << frontier_area << endl;
            cout << "|frontier.area|: " << frontier_area << endl;
            frontiers << "|frontier.square_area|: " << frontier_line_length.to_string() << endl;
            cout << "|frontier.square_area|: " << frontier_line_length.to_string() << endl;

        }

        double prev_area = frontier->get_area();
        LineLength prev_line_length = frontier->get_line_length();

        while(true) {
            frontiers << "NEW META ITER " << meta_iter << endl;
            cout << "NEW META ITER " << meta_iter << endl;
            frontier->changed = false;
            frontier = simulated_annealing(dataset_and_workload, frontiers, meta_iter++, frontier);
            if(!frontier->changed) {
                frontiers << "NO MORE IMPROVEMENT" << endl;
                cout << "NO MORE IMPROVEMENT" << endl;
                break;
            }
            else
            {
                cout << "DONE META ITER " << meta_iter-1 << endl;
                double frontier_area = frontier->get_area();
                LineLength frontier_line_length = frontier->get_line_length();
                frontiers << "|frontier.area|: " << frontier_area << endl;
                cout << "|frontier.area|: " << frontier_area << endl;
                frontiers << "|frontier.square_area|: " << frontier_line_length.to_string() << endl;
                cout << "|frontier.square_area|: " << frontier_line_length.to_string() << endl;

                if(prev_line_length == frontier_line_length) {
                    if (prev_area - frontier_area < frontier_area * 0.17/20) {
                        frontiers << "IMPROVEMENT TOO SMALL; BREAK;" << endl << endl;
                        cout << "IMPROVEMENT TOO SMALL; BREAK;" << endl << endl;
                        break;
                    }
                }
                else
                {
                    assert(prev_line_length < frontier_line_length);
                }

                prev_area = frontier_area;
                prev_line_length = frontier_line_length;
            }
        }

        Frontier<PointQueryPointer>* ret_frontier = new Frontier<PointQueryPointer>(2);

        const auto& dataset = dataset_and_workload.get_dataset();
        for(auto it: frontier->get_frontier())
        {
            MultiBloom* new_multi_bloom = new MultiBloom(dataset, it->get_params());
            new_multi_bloom->set_score(it->get_params().get_score());
            assert(ret_frontier->insert(PointQueryPointer(new_multi_bloom), it->get_score_as_vector()));
        }


        return ret_frontier;
    }

    static pair<Frontier<PointQueryPointer>*, Frontier<PointQueryPointer>*>  construct_hybrid_point_query(
            const DatasetAndWorkload& dataset_and_workload, RangeFilterTemplate& ground_truth, vector<string> path)
    {
        size_t base_case_min_size = 100;

        assert(!dataset_and_workload.get_workload().empty());

        assert(dataset_and_workload.get_workload().size() >= base_case_min_size);

        //base case

        cout << "OPTIMIZING BASE CASE OF PATH" << endl;
        for(const auto& it: path) {
            cout << it;
        }
        cout << " |workload| = " << dataset_and_workload.get_workload().size() <<" |dataset| = " << dataset_and_workload.get_dataset().size() << endl;
        cout << endl;

        if(dataset_and_workload.get_dataset().empty())
        {
            Frontier<PointQueryPointer>* base_case_frontier = optimize_base_case(dataset_and_workload);
            return make_pair(base_case_frontier, base_case_frontier);
        }




        assert(!dataset_and_workload.get_dataset().empty());

        //recursive case

        pair<double, string>* _ratio_and_best_split = ground_truth.analyze_negative_point_query_density_heatmap(dataset_and_workload);

        if(_ratio_and_best_split == nullptr)
        {
            //no need to split;
            //return base case;
            Frontier<PointQueryPointer>* base_case_frontier = optimize_base_case(dataset_and_workload);
            return make_pair(base_case_frontier, base_case_frontier);
        }

        pair<double, string> ratio_and_best_split = *_ratio_and_best_split;

//        float ratio = ratio_and_best_split.first;
        string best_split = ratio_and_best_split.second;

        cout << endl;

        vector<pair<string, string> > left_workload;
        vector<pair<string, string> > right_workload;

        const vector<pair<string, string> >& local_workload = dataset_and_workload.get_workload();

        for (size_t i = 0; i < local_workload.size(); i++) {
            assert(local_workload[i].second != best_split);
            assert(local_workload[i].first != best_split);
            assert(local_workload[i].first <= local_workload[i].second);
            if (local_workload[i].second < best_split) {
                left_workload.push_back(local_workload[i]);
            } else if(local_workload[i].first > best_split){
                right_workload.push_back(local_workload[i]);
            }
            else {
                assert(false);
            }
        }

        if(left_workload.size() < base_case_min_size || right_workload.size() < base_case_min_size)
        {
            Frontier<PointQueryPointer>* base_case_frontier = optimize_base_case(dataset_and_workload);
            return make_pair(base_case_frontier, base_case_frontier);
        }

        vector<string> left_dataset;
        vector<string> right_dataset;

        const vector<string>& local_dataset = dataset_and_workload.get_dataset();

        for (size_t i = 0; i < local_dataset.size(); i++) {
            if (local_dataset[i] < best_split) {
                left_dataset.push_back(local_dataset[i]);
            } else if (local_dataset[i] > best_split) {
                right_dataset.push_back(local_dataset[i]);
            }
            else
            {
                assert(local_dataset[i] == best_split);
                left_dataset.push_back(local_dataset[i]);
                right_dataset.push_back(local_dataset[i]);
            }
        }

        cout << "LEFT subproblem: |workload| = " << left_workload.size() << " |dataset| = " << left_dataset.size() << endl;
        cout << "RIGHT subproblem: |workload| = " << right_workload.size() << " |dataset| = " << right_workload.size() << endl;

        DatasetAndWorkload left_dataset_and_workload(left_dataset, left_workload, dataset_and_workload);
        path.push_back("L");
        left_dataset_and_workload.get_negative_workload();
        pair<Frontier<PointQueryPointer>*, Frontier<PointQueryPointer>*> _left_frontier = construct_hybrid_point_query(left_dataset_and_workload, ground_truth, path);
        Frontier<PointQueryPointer>* left_frontier = _left_frontier.first;
        path.pop_back();

        path.push_back("R");
        DatasetAndWorkload right_dataset_and_workload(right_dataset, right_workload, dataset_and_workload);
        right_dataset_and_workload.get_negative_workload();
        pair<Frontier<PointQueryPointer>*, Frontier<PointQueryPointer>*> _right_frontier = construct_hybrid_point_query(right_dataset_and_workload, ground_truth, path);
        Frontier<PointQueryPointer>* right_frontier = _right_frontier.first;
        path.pop_back();

        Frontier<PointQueryPointer>* ret = new Frontier<PointQueryPointer>(2);

        cout << "TRYING ALL COMBINATIONS " << left_frontier->get_size() <<" x "<< right_frontier->get_size() << endl;

        cout << "OF PATH" << endl;
        for(const auto& it: path) {
            cout << it;
        }
        cout << endl;

        int _sample_id = 0;
        size_t space = left_frontier->get_size() * right_frontier->get_size() ;
        int samples = 100000;
        float ratio = (float)samples/space;
        int samples_taken = 0;

        for(const FrontierPoint<PointQueryPointer>* left_pq: left_frontier->get_frontier())
        {
            for(const FrontierPoint<PointQueryPointer>* right_pq: right_frontier->get_frontier())
            {
                _sample_id++;
                if(rand()%1000 < ratio*1000){
                    samples_taken++;
                }
                else {
                    continue;
                }
//            cout << "evaluating (sample " << samples_taken << ", _sample_id " << _sample_id << ")" << endl;
//            left_pq->get_params().get_pq()->assert_contains = true;
//            right_pq->get_params().get_pq()->assert_contains = true;

//                HybridPointQuery* pq =
//                        new HybridPointQuery(
//                                best_split,
//                                dataset_and_workload.original_meta_data,
//                                left_pq->get_params().get_pq()->clone(),
//                                right_pq->get_params().get_pq()->clone());
////
//            RangeFilterScore rez1 = dataset_and_workload.eval_point_query(pq);

                int num_negatives =
                        left_pq->get_params().get_pq()->get_num_negatives() +
                        right_pq->get_params().get_pq()->get_num_negatives();
                assert(num_negatives == (int)dataset_and_workload.get_negative_workload_assert_has().size());

                assert(left_pq->get_params().get_pq()->get_num_false_negatives() +
                       right_pq->get_params().get_pq()->get_num_false_negatives() == 0);

                int memory =
                        left_pq->get_params().get_pq()->get_memory_from_score() +
                        right_pq->get_params().get_pq()->get_memory_from_score() +
                        best_split.size()*sizeof(char)*8;

                HybridPointQuery* pq = new HybridPointQuery(dataset_and_workload.original_meta_data);

                RangeFilterScore rez2(
                        pq,
                        (int)dataset_and_workload.get_dataset().size(),
                        (int)dataset_and_workload.get_workload().size(),
                        left_pq->get_params().get_pq()->get_num_false_positives() +
                        right_pq->get_params().get_pq()->get_num_false_positives(),
                        num_negatives,
                        0,
//                        (int)pq->get_memory()*8
                        memory
                        );


//            if(!(rez1 == rez2))
//            {
//                assert(rez1.get_num_false_positives() - 1 == rez2.get_num_false_positives());
//            }

                pq->set_score(rez2);

                if(ret->insert(PointQueryPointer(pq), rez2.get_score_as_vector()) != nullptr)
                {
                    pq->set_split(
                            best_split,
                            left_pq->get_params().get_pq()->clone(),
                            right_pq->get_params().get_pq()->clone());
                    assert(pq->get_memory()*8 == rez2.get_memory());
                }
            }
        }

        ret->print(cout, 10, true);

        cout << "OPTIMIZE BASE" << endl;

        vector<MultiBloomParams*> init_params;

        for(auto it: _left_frontier.second->get_frontier()) {
            it->get_params().get_pq()->populate_params(init_params);
        }
        for(auto it: _right_frontier.second->get_frontier()) {
            it->get_params().get_pq()->populate_params(init_params);
        }

        Frontier<PointQueryPointer>* base_case_frontier = optimize_base_case(dataset_and_workload, &init_params);

        for(auto it: base_case_frontier->get_frontier()) {
            ret->insert(it->get_params(), it->get_score_as_vector());
        }

        cout << "DONE WITH RECURSIVE STEP" << endl;

        for(auto it: ret->get_frontier()){
            assert(it->get_params().get_pq()->get_is_score_set());
        }

        return make_pair(ret, base_case_frontier);
    }

};



#endif //SURF_HYBRIDRANGEFILTERSYNTHESIZER_H
