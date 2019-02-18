#ifndef __torch__
#define __torch__

#ifdef __cplusplus
extern "C" {
#endif

/* Go one step up in the stack, it will be empty there */
void torch_push();
/* Go one step down in the stack, promise to leave the current level empty */
void torch_pop();

/*
* push const embedding as the next arguemnt on the current stack level
*/
void torch_stack_const(const char* nm);

/* Try retrieving term embedding from cache,
  if successful, just push it as the next argument on the current stack level
  otherwise just return false */
bool torch_stack_term_or_negation(void* term, bool negated);

/* Use sname, the name of term's top level symbol,
 to lookup the appropriate model and use it on the arguments on the current level.
 Cache the result (under term),
 clear the current level,
 go one step down in the stack,
 and push the result there.
*/
void torch_embed_and_cache_term(const char* sname, void* term);

/* current stack level containts single term's embedding,
  negate it, clear the current level,
  go one step down in stack,
  push the result there and also cache it (under `negated` term).
*/
void torch_embed_and_cache_term_negation(void* term);

/*
* Similar to torch_stack_term_or_negation,
* but tries to retrieve equalities of term pairs (l,r)
*/
bool torch_stack_equality_or_negation(void* l, void* r, bool negated);

/* currect stack level contains two terms' embeddings,
  evaluate them as equality,
  go one step down in stack,
  push the result there
*/
void torch_embed_and_cache_equality(void* l, void* r);

/* current stack level containts single eqn's embedding,
  negate it, clear the current level,
  go one step down in stack,
  push the result there and also cache it (under `negated` eqn's).
*/
void torch_embed_and_cache_equality_negation(void* l, void* r);

/* current stack level contians literal embeddings,
 sum them up and store aside (if aside true, otherwise ... ).
 Leave the stack level empty and go down one level (... otherwise, push the resul there)
*/
void torch_embed_clause(bool aside);

/* current stack level contians clause embeddings,
 sum them up and store aside.
 Leave the stack level empty and go down one level. */
void torch_embed_conjectures();

/*
Use the last embedded conjecture and the last embedded clause
to produce a softmax evaluation!
*/
float torch_eval_clause();

#ifdef __cplusplus
}
#endif



#endif
