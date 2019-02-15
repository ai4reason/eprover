#include "torch.h"

#include <torch/script.h>
#include <iostream>
#include <memory>
#include <cassert>
#include <cmath>

using std::vector;
using std::unordered_map;
using std::string;

typedef torch::jit::IValue IVal;
typedef std::shared_ptr<torch::jit::script::Module> Model;

/* when all conjcture clauses have been seen,
conj_embedding is set to the embedding of the concatenated conjectures */
static IVal conj_embedding;

static IVal clause_embedding;

static unordered_map<void*,IVal> term_embedding_cache;

static unordered_map<string,Model> named_model_cache;

static Model get_named_model(const string& name) {
  auto search = named_model_cache.find(name);
  if (search != named_model_cache.end()) {
    return search->second;
  } else {
    Model m = torch::jit::load(name);
    named_model_cache[name] = m;
    return m;
  }
}

static vector<vector<IVal>*> main_stack;
static int main_stack_top = -1;

void torch_push()
{
  main_stack_top++;
  if (main_stack_top == main_stack.size()) {
    main_stack.push_back(new vector<IVal>);
  }
}

void torch_pop()
{
  assert(main_stack_top >= 0);
  assert(main_stack[main_stack_top]->empty());
  main_stack_top--;
}

bool torch_stack_term(void* term)
{
  auto search = term_embedding_cache.find(term);
  if (search != term_embedding_cache.end()) {
    assert(main_stack_top >= 0);
    main_stack[main_stack_top]->push_back(search->second);
    return true;
  }
  return false;
}

void torch_embed_and_cache_term(const char* sname, void* term)
{
  // get model
  Model m = get_named_model(sname);
  
  // compute
  IVal result = m->forward(*main_stack[main_stack_top]);

  // cache
  term_embedding_cache[term] = result;

  // clean
  main_stack[main_stack_top]->clear();
  
  // pop
  assert(main_stack_top>0);
  main_stack_top--;
  
  // store result
  main_stack[main_stack_top]->push_back(result);
}

void torch_embed_clause()
{
  // load model (unless already there)
  static Model m = torch::jit::load("clause_concat.pt");

  // compute a single clause embedding
  clause_embedding = m->forward(*main_stack[main_stack_top]);
  
  main_stack[main_stack_top]->clear();
  main_stack_top--;
}

void torch_embed_conjectures()
{
  // load model (unless already there)
  static Model m = torch::jit::load("conj_concat.pt");

  // compute a single conjecture embedding
  conj_embedding = m->forward(*main_stack[main_stack_top]);
  
  main_stack[main_stack_top]->clear();
  main_stack_top--;
}

float torch_eval_clause()
{
  // load model (unless already there)
  static Model m = torch::jit::load("eval.pt");
  
  static vector<IVal> inputs;
  inputs.clear();
  inputs.push_back(conj_embedding);
  inputs.push_back(clause_embedding);

  auto output = (m->forward(inputs).toTensor()).data<float>();

  return 1.0 / (1.0 + exp(output[0]-output[1]));
}





