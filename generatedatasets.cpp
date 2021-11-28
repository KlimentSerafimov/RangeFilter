#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

int emails(){
    ifstream nameFileout;

    ofstream datasetfile;
    ofstream workloadfile;
	datasetfile.open("data.nosync/emails_dataset.txt");
    workloadfile.open("data.nosync/emails_workload.txt");

	nameFileout.open("../datasets.nosync/emails-validated-random-only-30-characters.txt.sorted");
	// nameFileout.open("url_data_set.txt");
    string line;

	while(std::getline(nameFileout, line)){
        // cout<<line<<endl;
        double r = (rand() * 1.0) /RAND_MAX;
        // cout<<"r: "<<r<<endl;

		if (r > 0.5){
            datasetfile<<line<<endl;
		} else {
            workloadfile<<line<<endl;
		}
    }

    nameFileout.close();
    workloadfile.close();
    datasetfile.close();

    return 0;
}

int urls(){
    ifstream nameFileout;

    ofstream datasetfile;
    ofstream workloadfile;
	datasetfile.open("data.nosync/url_dataset.txt");
    workloadfile.open("data.nosync/url_workload.txt");

	// nameFileout.open("..datasets/emails-validated-random-only-30-characters.txt.sorted");
	nameFileout.open("../datasets.nosync/url_data_set.txt");
    string line;

	while(std::getline(nameFileout, line)){

        // double r = rand()/RAND_MAX;
        double r = (rand() * 1.0) /RAND_MAX;

		if (r > 0.5){
            datasetfile<<line<<endl;
		} else {
            workloadfile<<line<<endl;
		}
    }
    
    nameFileout.close();
    workloadfile.close();
    datasetfile.close();
    return 0;
}

int small_dataset(){
    ifstream nameFileout;

    ofstream datasetfile;
    ofstream workloadfile;
	datasetfile.open("data/small_data_dataset.txt");
    workloadfile.open("data/small_data_workload.txt");

	// nameFileout.open("..datasets/emails-validated-random-only-30-characters.txt.sorted");
	nameFileout.open("data/small_dataset.txt");
    string line;

	while(std::getline(nameFileout, line)){

        // double r = rand()/RAND_MAX;
        double r = (rand() * 1.0) /RAND_MAX;

		if (r > 0.5){
            datasetfile<<line<<endl;
		} else {
            workloadfile<<line<<endl;
		}
    }
    
    nameFileout.close();
    workloadfile.close();
    datasetfile.close();
    return 0;
}

int main(){
    // emails();
    // urls();
    small_dataset();
    return 0;
}