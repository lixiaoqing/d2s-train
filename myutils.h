#include "stdafx.h"

void TrimLine(string &line);
vector<string> Split(const string &s);
vector<string> Split(const string &s, const string &sep);
void print_vector(vector<int> &v);
void load_data_into_blocks(vector<vector<string> > &data_blocks, ifstream &fin,int block_size);
