#ifndef __torch__
#define __torch__

#ifdef __cplusplus
extern "C" {
#endif

/* Go one step up in the stack, it will be empty there */
void torch_push();
/* Go one step down in the stack, promise to leave the current level empty */
void torch_pop();

/* Try retrieving term embedding from cache,
  if successful, just push it as the next argument on the current stack level
  otherwise just return false */
bool torch_stack_term(void* term);

/* Use sname, the name of term's top level symbol,
 to lookup the appropriate model and use it on the arguments on the current level.
 Cache the result (under term),
 clear the current level,
 go one step down in the stack,
 and push the result there.
*/
void torch_embed_and_cache_term(const char* sname, void* term);

/* current stack level contians literal embeddings,
 sum them up and store aside.
 Leave the stack level empty and go down one level.
*/
void torch_embed_clause();

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
