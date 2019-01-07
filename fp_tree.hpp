#include <vector>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <stdio.h>
#include <chrono>
#include <memory>
#include <assert.h>
#include <algorithm>

using namespace std;

struct Node {
  Node(uint32_t item_, uint32_t count_=1) :item(item_), count(count_) {}
  uint32_t count;
  uint32_t item;
  map<uint32_t, shared_ptr<Node>> children;
  shared_ptr<Node> parent;
  shared_ptr<Node> next_pos;
};

struct Header {
  Header(shared_ptr<Node> head, uint32_t count)
    :head_node(head), item_count(count){};
  uint32_t item_count;
  shared_ptr<Node> head_node;
};

class FPTree {
public:
  FPTree(uint32_t minsup_):minsup(minsup_) {}

  FPTree(const FPTree &tree) {
    minsup = tree.minsup;
    // copy header
    for(auto kv:tree.header) {
      header.insert(make_pair(kv.first, Header(nullptr, kv.second.item_count)));
    }
    // copy nodes
    stack<shared_ptr<Node>> stack_old;
    stack<shared_ptr<Node>> stack_new;
    for(auto it = tree.roots.rbegin(); it != tree.roots.rend(); ++it) {
      roots[it->first] = shared_ptr<Node>(new Node(it->second->item, it->second->count));
      stack_old.push(it->second);
      stack_new.push(roots[it->first]);
    }
    while(!stack_old.empty()) {
      auto n_old = stack_old.top();
      auto n_new = stack_new.top();
      stack_old.pop();
      stack_new.pop();
      for (auto it = n_old->children.rbegin();
           it != n_old->children.rend(); ++it ) {
        stack_old.push(it->second);
        n_new->children[it->first] = shared_ptr<Node>(new Node(it->second->item, it->second->count));
        n_new->children[it->first]->parent = n_new;
        stack_new.push(n_new->children[it->first]);
      }
      insert_header_link(n_new);
    }
  }

  static FPTree get_conditional_pattern_base(FPTree& tree, uint32_t item) {
    FPTree sub_tree(tree.minsup);
    assert(tree.header.find(item) != tree.header.end());
    shared_ptr<Node> node = tree.header.find(item)->second.head_node;
    vector<vector<uint32_t>> trans;
    while(node) {
      vector<uint32_t> tran;
      auto parent = node->parent;
      while(parent) {
        tran.insert(tran.begin(), parent->item);
        parent = parent->parent;
      }
      for(int i = 0; i < node->count; i++) {
        trans.push_back(tran);
      }
      node = node->next_pos;
    }
    sub_tree.build(trans);
    return sub_tree;
  }

  void remove_item(uint32_t item) {
    assert(header.find(item) != header.end());
    shared_ptr<Node> node = header.find(item)->second.head_node;
    while(node) {
      assert(node->children.size() == 0);
      auto parent = node->parent;
      if(parent) {
        parent->children.erase(item);
      } else {
        roots.erase(item);
      }
      node = node->next_pos;
    }
    // remove root with zero count
    vector<uint32_t > keys;
    for(auto& pair:roots) {
      if(pair.second->count == 0)
        keys.push_back(pair.first);
    }
    for(uint32_t k:keys) {
      roots.erase(k);
      header.erase(k);
    }
    header.erase(item);
  }

  void remove_infrequent_items() {
    vector<uint32_t> to_remove;
    map<uint32_t, uint32_t> count;
    for(auto& kv:header) {
      uint32_t item = kv.first;
      auto head = kv.second;
      shared_ptr<Node> node = head.head_node;
      count[item] = head.item_count;
      // do I need this?
      // if(node->children.size() > 0) {continue;}
      if (head.item_count < minsup) {
        to_remove.push_back(item);
      }
    }
    // sort in ascending order
    sort(to_remove.begin(), to_remove.end(), [&count](uint32_t a, uint32_t b){ return count[a] < count[b]; });
    for(uint32_t item:to_remove) {
      remove_item(item);
    }
  }

  bool is_empty() {
    return roots.size() == 0;
  }

  bool contains_one_item() {
    return roots.size() == 1 && roots.begin()->second->children.size() == 0;
  }

  void build(vector<vector<uint32_t>> trans) {
    // count items
    map<uint32_t, uint32_t> count;
    for(auto t:trans) {
      for(auto i:t) {
        if (count.find(i) == count.end()) {
          count[i] = 0;
        }
        count[i]++;
      }
    }
    // set up header
    for(auto kv:count) {
      if(kv.second < minsup)
        continue;
      header.insert(make_pair(kv.first, Header(nullptr, kv.second)));
    }
    vector<vector<uint32_t>> result;
    uint32_t least_freq = 0;
    int least_item = -1;
    for(auto& t:trans) {
      std::sort(t.begin(), t.end(), [&count](uint32_t a, uint32_t b) { return count[a] > count[b] || (count[a] == count[b] && a > b); });
      vector<uint32_t> tran;
      std::copy_if (t.begin(), t.end(), std::back_inserter(tran), [&count, this](int i) {return count[i]>=minsup;} );
      if(tran.size() > 0)
        result.push_back(tran);
    }
    trans.clear();
    std::copy(result.begin(), result.end(), std::back_inserter(trans));

    for(auto tran:trans) {
      if(tran.size() == 0) {continue;}
      insert(tran);
    }
  }

  void insert_header_link(shared_ptr<Node> new_node) {
    auto item = new_node->item;
    auto it = header.find(item);
    assert(it != header.end());
    // update link
    if(!it->second.head_node) {
      it->second.head_node = new_node;
    } else {
      auto left = header.find(item)->second.head_node;
      while(left->next_pos) {
        left = left->next_pos;
      }
      left->next_pos = new_node;
    }
  }

  void insert(vector<uint32_t> transaction) {
    shared_ptr<Node> new_node = nullptr;
    if (roots.find(transaction[0]) == roots.end()) {
      roots[transaction[0]] = shared_ptr<Node>(new Node(transaction[0]));
      insert_header_link(roots[transaction[0]]);
    } else {
      roots[transaction[0]]->count++;
    }
    auto cur = roots[transaction[0]];
    for (uint32_t i = 1; i < transaction.size(); i++) {
      auto item = transaction[i];
      if (cur->children.find(item) == cur->children.end()) {
        // insert new node
        cur->children[item] = shared_ptr<Node>(new Node(item));
        cur->children[item]->parent = cur;
        insert_header_link(cur->children[item]);
      } else {
        cur->children[item]->count++;
      }
      cur = cur->children[item];
    }
  }

  uint32_t get_least_item() {
    uint32_t least_item;
    uint32_t least_count = 0;
    for(auto& kv:header) {
      uint32_t item = kv.first;
      shared_ptr<Node> node = kv.second.head_node;
      // when there are mutiple least item, pick the one with all leaf nodes
      bool is_all_leaves = true;
      while(node) {
        is_all_leaves = is_all_leaves && node->children.size() == 0;
        node = node->next_pos;
      }
      if(!is_all_leaves) {continue;}
      uint32_t count = kv.second.item_count;
      if (least_count == 0 || count < least_count) {
        least_count = count;
        least_item = item;
      }
    }
    return least_item;
  }

  uint32_t get_support(set<uint32_t> transaction) {
    map<uint32_t, uint32_t> count;
    for(auto& kv:header) {
      count[kv.second.head_node->item] = kv.second.item_count;
    }
    vector<uint32_t> trans_vec;
    std::copy(transaction.begin(), transaction.end(), std::back_inserter(trans_vec));
    std::sort(trans_vec.begin(), trans_vec.end(), [&count](uint32_t a, uint32_t b) { return count[a] > count[b] || (count[a] == count[b] && a > b); });

    uint32_t last_item = trans_vec.back();

    uint32_t support = 0;
    auto it = header.find(last_item);
    shared_ptr<Node> head_node = it->second.head_node;

    while(head_node) {
      shared_ptr<Node> node = head_node;
      uint32_t count = node->count;
      map<uint32_t, bool> contain;
      for(uint32_t elm:transaction) {
        contain[elm] = false;
      }
      while(node) {
        contain[node->item] = true;
        // vertical move
        node = node->parent;
      }
      bool contain_all = true;
      for(auto kv:contain) {
        contain_all = contain_all&&kv.second;
      }
      if(contain_all) {
        support += count;
      }
      // horizontal move
      head_node = head_node->next_pos;
    }

    return support;
  }

  vector<set<uint32_t>> get_itemsets() {
    return vector<set<uint32_t>>({{roots.begin()->second->item}});
  }

  shared_ptr<Node> df_search(shared_ptr<Node> root, uint32_t item) {
    stack<shared_ptr<Node>> node_stack;
    node_stack.push(root);
    shared_ptr<Node> next;
    while(!node_stack.empty()) {
      next = node_stack.top();
      if(next->item == item) {return next;}
      node_stack.pop();
      for (auto it = next->children.rbegin();
           it != next->children.rend(); ++it ) {
        node_stack.push(it->second);
      }
    }
    return nullptr;
  }

  void dfs(shared_ptr<Node> node) {
    printf("%d:%d ", node->item, node->count);
    for(auto& pair : node->children) {
      dfs(pair.second);
    }
    printf("\n");
  }

  void bfs(shared_ptr<Node> node) {
    printf("%d:%d\n", node->item, node->count);
    queue<shared_ptr<Node>> que;
    int cur_layer = 0;
    int next_layer = 0;
    for(auto& pair:node->children) {
      que.push(pair.second);
      cur_layer++;
    }
    while(!que.empty()) {
      auto n = que.front();
      que.pop();
      for(auto& pair:n->children) {
        que.push(pair.second);
        next_layer++;
      }
      printf("%d:%d\t", n->item, n->count);
      if (--cur_layer== 0) {
        cur_layer = next_layer;
        next_layer = 0;
        printf("\n");
      }
    }
  }

  void print() {
    for(auto& pair : roots) {
      printf("Tree start \n");
      bfs(pair.second);
    }
  }

private:
  uint32_t minsup;
  map<uint32_t, shared_ptr<Node>> roots;
  map<uint32_t, Header> header;
};