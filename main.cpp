#include <iostream>
#include "fp_tree.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <fstream>
#include <sstream>

using namespace std;

set<uint32_t> get_union_set(set<uint32_t> left, set<uint32_t> right) {
  vector<uint32_t > u_vec;
  u_vec.resize(left.size() + right.size());
  auto end_pos = std::set_union(left.begin(), left.end(), right.begin(), right.end(), u_vec.begin());
  u_vec.resize(end_pos - u_vec.begin());
  set<uint32_t> u_set;
  for(auto item:u_vec) {u_set.insert(item);}
  return u_set;
}

float get_conf(set<uint32_t> left, set<uint32_t> right, FPTree& tree) {
  auto u_set = get_union_set(left, right);
  float sup_union = tree.get_support(u_set);
  float sup_left = tree.get_support(left);
//  float sup_right = tree.get_support(right);
  return sup_union / sup_left;
}

using Rule = pair<set<uint32_t>, set<uint32_t>>;
vector<Rule> rule_mining(vector<uint32_t> t, FPTree& tree, float minconf) {
  vector<Rule> rules;
  for(uint32_t i = 0; i < t.size(); i++) {
    set<uint32_t> left;
    vector<uint32_t> left_vec;
    set<uint32_t> right;
    for(uint32_t j = 0; j < t.size(); j++) {
      if(j != i) {
        left_vec.push_back(t[j]);
        left.insert(t[j]);
      }
      else
        right.insert(t[j]);
    }
    if (get_conf(left, right, tree) >= minconf && left.size() >= 2) {
      rules.push_back(make_pair(left, right));
      auto sub_rules = rule_mining(left_vec, tree, minconf);
      std::copy(sub_rules.begin(), sub_rules.end(), std::back_inserter(rules));
    }
    if (get_conf(right, left, tree) >= minconf) {
      rules.push_back(make_pair(right, left));
    }
  }
  return rules;
}

vector<set<uint32_t>> find_freq_itemset(FPTree tree) {
  tree.remove_infrequent_items();
//  tree.shrink_tree(minsup);
//    cout << "========tree=======" <<endl;
//  tree.print();
//  cout << "================" <<endl;
  using itemsets = vector<set<uint32_t>>;
  itemsets freq_sets;
  if(tree.is_empty())
    return freq_sets;
  if(tree.contains_one_item()) {
    freq_sets = tree.get_itemsets();
    return freq_sets;
  }

  uint32_t z = tree.get_least_item();
//  printf("z: %d\n", z);
  FPTree s1 = FPTree::get_conditional_pattern_base(tree, z);
//  cout << "=======s1========" <<endl;
//  s1.print();
//  cout << "================" <<endl;
  itemsets f1 = find_freq_itemset(s1);
  // add z to every itemset in f1, add {z} to f1
  for(auto& s:f1) {
    s.insert(z);
  }
  f1.push_back(set<uint32_t>{z});

//  cout << "========tree=======" <<endl;
//  tree.print();
//  cout << "================" <<endl;
  FPTree s2 = tree;
//  cout << "========s2=======" <<endl;
//  s2.print();
//  cout << "================" <<endl;
  s2.remove_item(z);
  itemsets f2 = find_freq_itemset(s2);
  // mege f1, f2 into itemsets
  freq_sets.insert( freq_sets.end(), f1.begin(), f1.end() );
  freq_sets.insert( freq_sets.end(), f2.begin(), f2.end() );
  return freq_sets;
}

vector<Rule> find_asso_rules(vector<set<uint32_t>> f_sets, FPTree tree, float minconf) {
  vector<Rule> all_rules;
  for(auto f_set:f_sets) {
    if(f_set.size() <= 1)
      continue;
    vector<uint32_t> f_vec;
    std::copy(f_set.begin(), f_set.end(), std::back_inserter(f_vec));
    auto rules = rule_mining(f_vec, tree, minconf);
    std::copy_if(rules.begin(), rules.end(), std::back_inserter(all_rules), [&all_rules](Rule r){
      for(auto& rule:all_rules) {
        if(rule.first == r.first && rule.second==r.second)
          return false;
      }
      return true;
    });
  }
  return all_rules;
}

void get_rule_sup_conf(Rule rule, uint32_t& sup, float& conf, FPTree& tree) {
  conf = get_conf(rule.first, rule.second, tree);
  auto u_set = get_union_set(rule.first, rule.second);
  sup = tree.get_support(u_set);
}

vector<vector<uint32_t>> read_data(std::string fname) {
  vector<vector<uint32_t>> ret;
  ifstream fin(fname);
  string line = "";
  if (!fin.good()) {
    throw "Open data failed";
  }
  while(getline(fin, line)) {
    std::stringstream ss(line);
    string col = "";
    vector<uint32_t> tran;
    getline(ss, col, ','); // discard first column
    while(getline(ss, col, ',')) {
      tran.push_back(std::atoi(col.c_str()));
    }
    ret.push_back(tran);
  }
  return ret;
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " data_file support[0-1] confidence[0-1]" << endl;
    return 1;
  }
  vector<vector<uint32_t>> trans = read_data(argv[1]);
  float support = std::atof(argv[2]);
  float confidence = std::atof(argv[3]);
  uint32_t total = trans.size();
  uint32_t minsup = support * total;

//  vector<vector<uint32_t>> trans({
//    {1,2,3},
//    {1,3},
//    {1,4}
//  });
  auto start = std::chrono::steady_clock::now();
  FPTree tree(minsup);
  tree.build(trans);
//  tree.print();

  vector<set<uint32_t>> itemsets = find_freq_itemset(tree);
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);
  printf("Find %d frequent itemset in %ds. \n", itemsets.size(), duration);

  // sorting is too slow because of get_support, if test on large frequent itemset, comment this line
  sort(itemsets.begin(), itemsets.end(), [&tree](set<uint32_t> a, set<uint32_t> b){return tree.get_support(a) < tree.get_support(b);});
  // print frequent itemsets
  for(auto& s:itemsets) {
    auto sup = tree.get_support(s);
    printf("support: %.4f ", float(sup)/total);
    cout << "{";
    for(auto item : s) {cout << item << ",";}
    cout << "}";
    cout << endl;
  }
  cout << "=============================================" << endl;
  start = std::chrono::steady_clock::now();
  auto asso_rules = find_asso_rules(itemsets, tree, confidence);

  duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);
  printf("Find %d association rules in %ds. \n", asso_rules.size(), duration);
  // print association rules
  for(auto& r:asso_rules) {
    uint32_t rule_support = 0;
    float rule_confidence = 0.0;
    get_rule_sup_conf(r, rule_support, rule_confidence, tree);
    printf("support: %.4f, confidence: %.4f ", float(rule_support)/total, rule_confidence);
    cout << "{";
    for(auto item : r.first) {cout << item << ",";}
    cout << "}->";
    cout << "{";
    for(auto item : r.second) {cout << item << ",";}
    cout << "}";
    cout << endl;
  }
  return 0;
}