#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

int emails(){
    ifstream nameFileout;

    ofstream datasetfile;
    ofstream workloadfile;
	datasetfile.open("data/emails_dataset.txt");
    workloadfile.open("data/emails_workload.txt");

	nameFileout.open("../datasets.nosync/emails-validated-random-only-30-characters.txt.sorted");
	// nameFileout.open("url_data_set.txt");
    string line;

	while(std::getline(nameFileout, line)){
        // cout<<line<<endl;
        double r = rand()/RAND_MAX;
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
	datasetfile.open("data/url_dataset.txt");
    workloadfile.open("data/url_workload.txt");

	// nameFileout.open("..datasets/emails-validated-random-only-30-characters.txt.sorted");
	nameFileout.open("../datasets.nosync/url_data_set.txt");
    string line;

	while(std::getline(nameFileout, line)){

        double r = rand()/RAND_MAX;
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
    emails();
    urls();
    return 0;
}