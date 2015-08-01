#include "stdafx.h"

void TrimLine(string &line);
vector<string> Split(const string &s);
vector<string> Split(const string &s, const string &sep);
string Join(const vector<string> &vs);
void print_vector(vector<int> &v);
int get_word_num(string s);
void load_data_into_blocks(vector<vector<string> > &data_blocks, ifstream &fin,int block_size);
double d2log(double prob);
