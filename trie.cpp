#include <iostream>
using namespace std;
#define CHAR_SIZE 256 // was 128

class Trie {

	bool isleaf;
	Trie *character[CHAR_SIZE];

	// constructor
	Trie(){
		this->isleaf = false;
		for (int i = 0; i <CHAR_SIZE; i ++){
			this->character[i] = nullptr;
		}
	}
	void insert(string);
	bool deletion(Trie*&, string);
	bool search(string);
	bool haveChildren(Trie const*);
};

// insert function for putting key in trie
void Trie::insert(string key){
	Trie *curr = this;

	for (int i = 0; i < key.length(); i++){
		// if path doesn't exist, create a new node
		if (curr->character[key[i]] == nullptr){
			curr->character[key[i]] = new Trie;
		}
		// go to next node
		curr = curr->character[key[i]];
	}
	// mark the last iterated node as a leaf
	curr->isleaf = true;
}

// search a key in trie
bool Trie::search(string key){
	// base case: trie is empty, return false
	if (this == nullptr){
		return false;
	}
	Trie *curr = this;
	for (int i = 0; i < key.length(); i++){
		// go to the next node
		curr = curr->character[key[i]];
		// if the string is invalid, reached the end of a path
		if (curr == nullptr){
			return false;
		}
	}

	// we are at the last character of the string, and current node is a leaf
	return curr->isleaf;
}

// check of a node has children or not
bool Trie::haveChildren(Trie const* curr){
	for (int i = 0; i < CHAR_SIZE; i++){
		// check if the trie has any child nodes
		if (curr->character[i]){
			return true; 
		}
	}
	return false;
}

// delete a key in trie
// returns true if key has been deleted
bool Trie::deletion(Trie *&curr, string key){
	if (curr == nullptr){
		return false;
	}
	// if the end of the key is not reached, recurse on the next character node and if it returns true, delete the current node if it is not a leaf
	if (key.length()){
		if (curr != nullptr && 
			curr->character[key[0]] != nullptr && 
			deletion(curr->character[key[0]], key.substr(1)) &&
			curr->isleaf == false){

				if (!haveChildren(curr)){
					delete(curr);
					curr = nullptr;
					return true;
				}
				return false; // TODO: what is the difference between isleaf and haveChildren?
		}
	}

	// reached end of key
	if (key.length() == 0 && curr->isleaf){
		// if the current node is a leaf and doesn't have any children, delete the current node and the non-leaf parent nodes
		if (!haveChildren(curr)){
			delete curr;
			curr = nullptr;
			return true;
		} else {
			// if the current node is a leaf node and has children, mark it as a non-leaf node but don't delete it
			curr->isleaf = false;
			return false;
		}
	}
	return false;
}


