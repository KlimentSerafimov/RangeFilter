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

    // we are at the last character of the string, and current nod is a leaf
    return curr->isleaf;
}

// check of a node has children or not

// delete a key in trie


