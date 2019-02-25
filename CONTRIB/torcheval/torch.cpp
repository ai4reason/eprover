#include "torch.h"

#include <torch/script.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>

using std::size_t;
using std::pair;
using std::vector;
using std::unordered_map;
using std::string;
using std::make_pair;
using std::endl;
using std::cout;
using std::cerr;

// #define LOGGING 1

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

string modeldir;

void torch_setbase(const char* basename)
{
  modeldir = string("models/")+basename;
}

static unordered_map<string,long> load_constant_id_lookup()
{
  std::ifstream infile(modeldir+"/translate_const.txt");
  
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
  static unordered_map<string,long> constant_id_lookup = load_constant_id_lookup();
  static unordered_map<string,Tensor> constant_embeddings;
  
  auto search_name = constant_embeddings.find(name);
  if (search_name != constant_embeddings.end()) {
    return search_name->second;
  } else {
    static Model m = torch::jit::load(modeldir+"/s_emb.pt");
    
    auto search_id = constant_id_lookup.find(name);
    if (search_id == constant_id_lookup.end()) {
      cerr << "Couldn't resolve constant index for "<< name << endl;
      exit(1);
    }
    
    // cout << (int64_t)search_id->second << endl;
    
    // fprintf(stdout,"FORWARD: models2/s_emb.pt\n");
    
    static std::vector<IVal> inputs;
    inputs.clear();
    inputs.push_back(torch::tensor((int64_t)search_id->second,at::kLong));
    Tensor result = m->forward(inputs).toTensor()[0];
    constant_embeddings[name] = result;
    
#ifdef LOGGING
    cout << "constant " << name << endl;
    cout << result << endl;
#endif

/*
    for (int64_t i = 0;i < 540; i++) {
      static std::vector<IVal> inputs;
      inputs.clear();
      inputs.push_back(torch::tensor(i,at::kLong));
      Tensor result = m->forward(inputs).toTensor()[0];
      constant_embeddings[name] = result;
      
      cout << "constant " << i << endl;
      cout << result << endl;
    }
*/
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

static Model get_named_model(const string& name)
{
  static unordered_map<string,Model> named_model_cache;
  
  auto search = named_model_cache.find(name);
  if (search != named_model_cache.end()) {
    return search->second;
  } else {
    Model m = torch::jit::load(modeldir+"/str/"+name+".pt");
    named_model_cache[name] = m;
    return m;
  }
}

void torch_embed_and_cache_term(const char* sname, void* term)
{
  // get model
  Model m = get_named_model(sname);
  
  // fprintf(stdout,"FORWARD: %s\n",sname);
  
  // compute
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  
  Tensor result = m->forward(inputs).toTensor();
  
#ifdef LOGGING
  cout << "function " << sname << endl;
  cout << inputs[0] << endl;
  cout << result << endl;
#endif

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

Model get_negator() {
  static Model model_negator = torch::jit::load(modeldir+"/str/~.pt");
  return model_negator;
}

void torch_embed_and_cache_term_negation(void* term)
{
  // fprintf(stdout,"FORWARD: ~\n");

  // compute
  assert(main_stack_top>=0);
  assert(main_stack[main_stack_top]->size() == 1);
  static std::vector<IVal> inputs;
  inputs.clear();
  
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = get_negator()->forward(inputs).toTensor();
  
#ifdef LOGGING
  cout << "negating " << endl;
  cout << inputs[0] << endl;
  cout << result << endl;
#endif

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
  static Model m = torch::jit::load(modeldir+"/str/=.pt");

 // fprintf(stdout,"FORWARD: =\n");

  // compute
  assert(main_stack_top>=0);
  assert(main_stack[main_stack_top]->size() == 2);
  
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = m->forward(inputs).toTensor();
  
#ifdef LOGGING
  cout << "equality " << endl;
  cout << inputs[0] << endl;
  cout << result << endl;
#endif
  
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
  // fprintf(stdout,"FORWARD: ~\n");

  // compute
  assert(main_stack_top>=0);
  assert(main_stack[main_stack_top]->size() == 1);
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  Tensor result = get_negator()->forward(inputs).toTensor();
  
#ifdef LOGGINGGING
  cout << "negating equality" << endl;
  cout << inputs[0] << endl;
  cout << result << endl;
#endif
  
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
  // fprintf(stdout,"FORWARD: clausenet.pt\n");
  // cerr << "torch_embed_clause " << main_stack_top << " " << main_stack[main_stack_top]->size() << endl;

  // load model (unless already there)
  static Model m = torch::jit::load(modeldir+"/clausenet.pt");

  // compute a single clause embedding
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  
  Tensor result = m->forward(inputs).toTensor();

#ifdef LOGGING
  cout << "clausenet " << endl;
  cout << inputs[0] << endl;
  cout << result << endl;
#endif

  main_stack[main_stack_top]->clear();
  main_stack_top--;
  
  // Tensor result = torch::zeros(8);
  
  if (aside) {
    clause_embedding = result;
  } else {
    assert(main_stack_top>=0);
    main_stack[main_stack_top]->push_back(result);
  }
}

void torch_embed_conjectures()
{
  // fprintf(stdout,"FORWARD: conjecturenet.pt\n");
  
  // load model -- conjecture net used only once, so don't cache it
  Model m = torch::jit::load(modeldir+"/conjecturenet.pt");

  // compute a single conjecture embedding
  static std::vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat(*main_stack[main_stack_top],0));
  
  conj_embedding = m->forward(inputs).toTensor();
  
#ifdef LOGGING
  cout << "conjecturenet" << endl;
  cout << inputs[0] << endl;
  cout << conj_embedding << endl;
#endif
  
  main_stack[main_stack_top]->clear();
  main_stack_top--;
}

float torch_eval_clause()
{
  // fprintf(stdout,"FORWARD: final.pt\n");

  // load model (unless already there)
  static Model m = torch::jit::load(modeldir+"/final.pt");
  
  static vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(torch::cat({clause_embedding,conj_embedding},0));
  Tensor output = m->forward(inputs).toTensor();
  
#ifdef LOGGING
  cout << "final" << endl;
  cout << inputs[0] << endl;
  cout << output << endl;
#endif
  
  auto raw_data = output.data<float>();
  
  return 1.0 / (1.0 + exp(raw_data[1]-raw_data[0]));
}





