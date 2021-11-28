/*
 *********************************************************************
 *                                                                   *
 *                           Open Bloom Filter                       *
 *                                                                   *
 * Description: Basic Bloom Filter Usage                             *
 * Author: Arash Partow - 2000                                       *
 * URL: http://www.partow.net                                        *
 * URL: http://www.partow.net/programming/hashfunctions/index.html   *
 *                                                                   *
 * Copyright notice:                                                 *
 * Free use of the Open Bloom Filter Library is permitted under the  *
 * guidelines and in accordance with the MIT License.                *
 * http://www.opensource.org/licenses/MIT                            *
 *                                                                   *
 *********************************************************************
*/


/*
   Description: This example demonstrates basic usage of the Bloom filter.
				Initially some values are inserted then they are subsequently
				queried, noting any false positives or errors.
*/


#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "bloom/bloom_filter.hpp"

using namespace std;
vector<bloom_filter> bfs;
vector<string> keys;

int create_multi_level_bfs(){

	// std::vector<bloom_filter> bfs;
	ifstream nameFileout;
	// change this to read from datasets dataset
	// nameFileout.open("data/emails_dataset.txt");
	// nameFileout.open("data/urls_dataset.txt");
	nameFileout.open("data/small_data_dataset.txt");

	string line;

	while(std::getline(nameFileout, line)){
		// cout<<"line: "<<line<<endl;
		keys.push_back(line);
		// insert each prefix into each bloom filter
		for (int i = 0; i < line.length(); i++){
			if (bfs.size() <= i){
				// initialize bf
				//[bf1, bf2, bf3... bfn]
				bloom_parameters parameters;

				// bf initialiation code
				// How many elements roughly do we expect to insert?
				parameters.projected_element_count = 500; // for small_dataset.txt
				// Maximum tolerable false positive probability? (0,1)
				parameters.false_positive_probability = 0.5; // 1 in 10000
				// Simple randomizer (optional)
				parameters.random_seed = 0xA5A5A5A5;
				if (!parameters){
					std::cout << "Error - Invalid set of bloom filter parameters!" << std::endl;
					return 1;
				}
				parameters.compute_optimal_parameters();
				//Instantiate Bloom Filter
				bloom_filter filter(parameters);
				bfs.push_back(filter);
			}
			// add this prefix to bf
			bfs[i].insert(line.substr(0, i + 1));
			// cout<<"inserting into bloom filter "<<i<<": "<<line.substr(0, i + 1)<<endl;
		}	
	}
	nameFileout.close();

	cout<<"key vector size: "<<keys.size()<<endl;

   return 0;
}


// TODO: implement binary search to determine true positives

int query_binary_search(vector<string> keys, string left_key, string right_key){

	int i = 0;
	while (left_key[i] == right_key[i]){
		// if dataset does not contain a common prefix, retuurn false
		if(!binary_search(keys.begin(), keys.end(), left_key.substr(0, i + 1))){
			return 0;
		}
		i += 1;

	}

	// gotten past common substring;
	// query left_key[i] .... right_key[i] on BF
	// edge case: BF for a certain index doesn't exist
	// bounds checking: make sure right_key[i] is within the a-z || A-Z bounds
	for (char ch = left_key[i]; ch < right_key[i] + 1; ch++){
		if (binary_search(keys.begin(), keys.end(),left_key.substr(0, i) + ch)){
			return 1;
		}
	return 0;
	}
}

/*

	When training the bloom filters at each level, 
	should we train bloom filter 1 only on the first character of each key, 
	bloom filter 2 on the first two characters of each key, 
	bloom filter 3 on the first three characters of each key, etc? 
	
	Then for a query, for example while querying if a key lies between Veronica and Versace, 
	query BF1(V), and if it gives true 
	query BF2(VE) and if it gives true 
	query BF3(VER), if it gives true 
	query BF4(VERO), BF4(VERP), BF4(VERQ), BF4(VERR), and BF4(VERS), and then 
	if one of those is true then we return true?

 */

int bf_query(string left_key, string right_key){
	cout<<"1"<<endl;
	int i = 0;
	while (left_key[i] == right_key[i]){
		// if dataset does not contain a common prefix, retuurn false
		cout<<left_key<<" and "<<left_key[i];
		if (!bfs[i].contains(left_key.substr(0, i + 1))){
			return 0;
		}
		i += 1;
	}

	// gotten past common substring;
	// query left_key[i] .... right_key[i] on BF
	// edge case: BF for a certain index doesn't exist
	// bounds checking: make sure right_key[i] is within the a-z || A-Z bound
	cout<<"left "<<left_key[i]<<" and right "<< right_key[i]+1<<endl;
	for (char ch = left_key[i]; ch < right_key[i] + 1; ch++){
		cout<<"checking for "<<left_key.substr(0, i) + ch<<endl;

		if (bfs[i].contains(left_key.substr(0, i) + ch)){

			return 1;
		}
	}

	return 0;
}


int query_bloom_filters(){

	ifstream nameFileout;
	// change this to read from workload dataset
	// nameFileout.open("data/emails_workload.txt");
	// nameFileout.open("data/url_workload.txt");
	nameFileout.open("/home/sparklab/RangeFilter/small_data_workload.txt");

	string line;
	string left_key;
	string right_key;

	int fp = 0;
	int tn = 0;
	double fpr = 0;


	while(std::getline(nameFileout, line)){
		left_key = line;
		right_key = left_key;
		right_key[right_key.length() - 1]++;	

		int bf_query_result = bf_query(left_key, right_key);
		int binary_search_result = query_binary_search(keys, left_key, right_key);

		if (bf_query_result == 1 && binary_search_result == 0){
			fp += 1;
		} else if (bf_query_result == 0 && binary_search_result == 0){
			tn += 1;
		}
	}
	nameFileout.close();

	fpr = (fp * 1.0)/ (fp + tn);
	cout<<"fpr: "<<fpr<<endl;
	return 0;
}


int get_bf_memory(){
	int total_mem = 0;
	for (int i = 0; i < bfs.size(); i++){
		total_mem += bfs[i].get_memory();
	}
	cout<<"total bloom filter space: "<<total_mem<<endl;
	return total_mem;
}

int main(){

	int return_code = create_multi_level_bfs();
	cout<<"keys: "<<keys.size()<<endl;
	
	query_bloom_filters();
	get_bf_memory();
	
	return 0;
}

