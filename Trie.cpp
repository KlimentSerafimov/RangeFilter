//
// Created by kliment on 11/16/21.
//

#include "Trie.h"


#include <cassert>


// insert function for putting key in Trie
void Trie::insert(string key){
    Trie *curr = this;

    for (int i = 0; i < (int)key.length(); i++){
        // if path doesn't exist, create a new node
        if (curr->character[(int)key[i]] == nullptr){
            curr->character[(int)key[i]] = new Trie(this);
        }
        // go to next node
        curr = curr->character[(int)key[i]];
    }
    // mark the last iterated node as a leaf
    curr->isleaf = true;
}


// search a key in Trie
bool Trie::contains(string key){
    Trie *curr = this;
    for (int i = 0; i < (int)key.length(); i++){
        // go to the next node
        curr = curr->character[(int)key[i]];
        // if the string is invalid, reached the end of a path
        if (curr == nullptr){
            return false;
        }
    }

    // we are at the last character of the string, and current node is a leaf
    return curr->isleaf;
}

void breakpoint(bool ret)
{

}

bool Trie::query(string left, string right) {
    assert(left <= right);
    assert(!left.empty() && !right.empty());

    if(left[0] == right[0])
    {
        char c = left[0];
        if(character[(int)c] == nullptr)
        {
            return ret(false);
            return false;
        }
        else
        {
            string left_rec = left.substr(1, left.size()-1);
            string right_rec = right.substr(1, right.size()-1);
            if(left_rec.empty())
            {
                if(character[(int)c]->isleaf)
                {
                    return ret(true);
                    return true;
                }
                left_rec += init_char;
            }
            if(right_rec.empty())
            {
                if(character[(int)c]->isleaf)
                {
                    assert(false);
                }
                return ret(false);
                return false;
            }
//            return ret(character[(int)c]->query(left_rec, right_rec));
            return character[(int)c]->query(left_rec, right_rec);
        }
    }

    for(char c = (char)((int)left[0]+1); c <= (char)((int)right[0]-1); c++)
    {
        if(character[(int)c] != nullptr)
        {
            return ret(true);
            return true;
        }
    }

    {
        if(character[(int)left[0]] == nullptr)
        {

        }
        else {
            string right_rec = "";
            string left_rec = left.substr(1, left.size() - 1);
            if (left_rec.empty()) {
                if(character[(int)left[0]]->isleaf)
                {
                    breakpoint(true);
                    return ret(true);
                    return true;
                }
                left_rec += init_char;
            }
            if (right_rec.empty()) {
                while(right_rec.size() < left_rec.size()) {
                    right_rec += last_char;
                }
            }
            bool ret_val = character[(int)left[0]]->query(left_rec, right_rec);

            if (ret_val) {
//                return ret(ret_val);
                return ret_val;
            }
        }
    }

    {
        if(character[(int)right[0]] == nullptr)
        {
            breakpoint(false);
            return ret(false);
            return false;
        }

        if(character[(int)right[0]]->isleaf)
        {

            breakpoint(true);
            return ret(true);
            return true;
        }

        string right_rec = right.substr(1, right.size() - 1);
        string left_rec;
        if (left_rec.empty()) {
            while(left_rec.size() < right_rec.size()) {
                left_rec += init_char;
            }
        }
        if (right_rec.empty()) {
            breakpoint(false);
            return ret(false);
            return false;
        }

        bool ret_val = character[(int)right[0]]->query(left_rec, right_rec);
//        return ret(ret_val);
        return ret_val;
    }
}

// check of a node has children or not
bool Trie::haveChildren(Trie const* curr){
    for (int i = 0; i < CHAR_SIZE; i++){
        // check if the Trie has any child nodes
        if (curr->character[i]){
            return true;
        }
    }
    return false;
}

// delete a key in Trie
// returns true if key has been deleted
bool Trie::deletion(Trie *&curr, string key){
    if (curr == nullptr){
        return false;
    }
    // if the end of the key is not reached, recurse on the next character node and if it returns true, delete the current node if it is not a leaf
    if (key.length()){
        if (curr != nullptr &&
            curr->character[(int)key[0]] != nullptr &&
            deletion(curr->character[(int)key[0]], key.substr(1)) &&
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

string Trie::get_init_char() {
    return ""+init_char;
}

unsigned long long Trie::get_memory() {

//    bool isleaf;
//    Trie* parent = nullptr; //opt away
//    Trie *character[CHAR_SIZE]{};
//
//    char last_char;  //opt away
//    char init_char;  //opt away
//    int max_length;  //opt away

    unsigned long long ret = 0;
    int unused = 0;
    for(int i = 0;i<CHAR_SIZE; i++)
    {
        if(character[i] != nullptr)
        {
            ret += character[i]->get_memory();
        }
        else
        {
            unused += 1;
        }
    }

    //1264.27
    return ret + sizeof(bool) + (CHAR_SIZE-unused)*sizeof(Trie*);

    //143987
    return ret + sizeof(bool) + (CHAR_SIZE)*sizeof(Trie*);

    //145953
    return ret + sizeof(bool) + sizeof(Trie*) + CHAR_SIZE*sizeof(Trie*) + 2*sizeof(char) + sizeof(int);

    return 0;
}



