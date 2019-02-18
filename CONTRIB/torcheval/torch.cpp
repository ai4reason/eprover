#include "torch.h"

#include <torch/script.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <cmath>

using std::size_t;
using std::pair;
using std::vector;
using std::unordered_map;
using std::string;
using std::make_pair;
using std::endl;
using std::cout;
using std::cerr;

template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template <class T>
struct pair_hasher {
  size_t operator()(const pair<T,T>& p) const
  {
    std::hash<T> hasher;
    size_t hash = hasher(p.first);
    hash_combine(hash,p.second);
    return hash;
  }
};

typedef at::Tensor Tensor;
typedef torch::jit::IValue IVal;
typedef std::shared_ptr<torch::jit::script::Module> Model;

/* when all conjecture clauses have been seen,
conj_embedding is set to the embedding of the concatenated conjectures */
static Tensor conj_embedding;

/* temporarily stores the embedding of the last clause, before being used by eval */
static Tensor clause_embedding;

/* there are actually two caches here, the second (idx 1) is for negated literals ... */
static unordered_map<void*,Tensor> term_embedding_cache[2];
/* ... and there are two caches for non-negated and negated equations */
static unordered_map<pair<void*,void*>,Tensor,pair_hasher<void*>> eqn_embedding_cache[2];

static Model get_named_model(const string& name)
{
  static unordered_map<string,Model> named_model_cache;
  
  auto search = named_model_cache.find(name);
  if (search != named_model_cache.end()) {
    return search->second;
  } else {
    Model m = torch::jit::load(name);
    named_model_cache[name] = m;
    return m;
  }
}

static unordered_map<string,long> load_constant_lookup()
{
  std::ifstream infile("models/translate_const.txt");
  
  unordered_map<string,long> result;
  
  string sym;
  long id;
  while (infile >> sym >> id)
  {
    result[sym] = id;
  }
  return result;
}

static Tensor get_named_constant(const string& name)
{
  static unordered_map<string,long> constant_lookup = load_constant_lookup();
  static unordered_map<string,Tensor> constant_embeddings;
  
  auto search = constant_embeddings.find(name);
  if (search != constant_embeddings.end()) {
    return search->second;
  } else {
    static Model m = torch::jit::load("models/s_emb.pt");
    static std::vector<IVal> inputs;
    inputs.clear();
    inputs.push_back(torch::tensor({constant_lookup[name]},at::kLong));
    Tensor result = m->forward(inputs).toTensor()[0];
    constant_embeddings[name] = result;
    
    // cerr << "get_named_constant: " << result << endl;
    
    return result;
  }
}

static vector<vector<Tensor>*> main_stack;
static int main_stack_top = -1;

void torch_push()
{
  main_stack_top++;
  if (main_stack_top == (int)main_stack.size()) {
    main_stack.push_back(new vector<Tensor>);
  }
}

void torch_pop()
{
  assert(main_stack_top >= 0);
  assert(main_stack[main_stack_top]->empty());
  main_stack_top--;
}

void torch_stack_const(const char* nm)
{
  assert(main_stack_top >= 0);
  main_stack[main_stack_top]->push_back(get_named_constant(nm));
}

bool torch_stack_term_or_negation(void* term, bool negated)
{
  auto search = term_embedding_cache[negated].find(term);
  if (search != term_embedding_cache[negated].end()) {
    assert(main_stack_top >= 0);
    main_stack[main_stack_top]->push_back(search->second);
    return true;
  }
  return false;
}

bool torch_stack_equality_or_negation(void* l, void* r, bool negated)
{
  auto search = eqn_embedding_cache[negated].find(make_pair(l,r));
  if (search != eqn_embedding_cache[negated].end()) {
    assert(main_stack_top >= 0);
    main_stack[main_stack_top]->push_back(search->second);
    return true;
  }
  return false;
}

void torch_embed_and_cache_term(const char* sname, void* term)
{
  // get model
  Model m = get_named_model(/*TODO: more name magic here*/sname);
  
  // compute
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = m->forward(inputs).toTensor();

  // cache
  term_embedding_cache[/*negated=*/false][term] = result;

  // clean
  main_stack[main_stack_top]->clear();
  
  // pop
  assert(main_stack_top>0);
  main_stack_top--;
  
  // store result
  main_stack[main_stack_top]->push_back(result);
}

static Model model_negator = torch::jit::load("models/str/~.pt");

void torch_embed_and_cache_term_negation(void* term)
{
  // compute
  assert(main_stack_top>=0);
  assert(main_stack[main_stack_top]->size() == 1);
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = model_negator->forward(inputs).toTensor();
  
  // cache
  term_embedding_cache[/*negated=*/true][term] = result;
  
  // clean
  main_stack[main_stack_top]->clear();
  
  // pop
  assert(main_stack_top>0);
  main_stack_top--;
  
  // store result
  main_stack[main_stack_top]->push_back(result);
}

void torch_embed_and_cache_equality(void* l, void* r)
{
  static Model m = torch::jit::load("models/str/=.pt");

  // compute
  assert(main_stack_top>=0);
  assert(main_stack[main_stack_top]->size() == 2);
  
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = m->forward(inputs).toTensor();
  
  // cache
  eqn_embedding_cache[/*negated=*/false][make_pair(l,r)] = result;
  
  // clean
  main_stack[main_stack_top]->clear();
  
  // pop
  assert(main_stack_top>0);
  main_stack_top--;
  
  // store result
  main_stack[main_stack_top]->push_back(result);
}

void torch_embed_and_cache_equality_negation(void* l, void* r)
{
  // compute
  assert(main_stack_top>=0);
  assert(main_stack[main_stack_top]->size() == 1);
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = model_negator->forward(inputs).toTensor();
  
  // cache
  eqn_embedding_cache[/*negated=*/true][make_pair(l,r)] = result;
  
  // clean
  main_stack[main_stack_top]->clear();
  
  // pop
  assert(main_stack_top>0);
  main_stack_top--;
  
  // store result
  main_stack[main_stack_top]->push_back(result);
}

void torch_embed_clause(bool aside)
{
  cerr << "torch_embed_clause " << main_stack_top << " " << main_stack[main_stack_top]->size() << endl;

  // load model (unless already there)
  static Model m = torch::jit::load("models/clausenet.pt");

  // compute a single clause embedding
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = m->forward(inputs).toTensor();
  
  main_stack[main_stack_top]->clear();
  main_stack_top--;
  
  if (aside) {
    clause_embedding = result;
  } else {
    assert(main_stack_top>=0);
    main_stack[main_stack_top]->push_back(result);
  }
}

void torch_embed_conjectures()
{
  // load model (unless already there)
  static Model m = torch::jit::load("conj_concat.pt");

  // compute a single conjecture embedding
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  conj_embedding = m->forward(inputs).toTensor();
  
  main_stack[main_stack_top]->clear();
  main_stack_top--;
}

float torch_eval_clause()
{
  // load model (unless already there)
  static Model m = torch::jit::load("models/final.pt");
  
  static vector<IVal> inputs;
  inputs.clear();
  // inputs.push_back(conj_embedding);
  inputs.push_back(clause_embedding);

  auto output = (m->forward(inputs).toTensor()).data<float>();

  return 1.0 / (1.0 + exp(output[1]-output[0]));
}





