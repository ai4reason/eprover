#ifndef __torcheval__
#define __torcheval__

#include <ccl_clauses.h>

/* first add all conjecture clauses one by one (order matters, what's the covention?) */
void torch_conjecture_clause(Clause_p cl);
/* call this when all conjecture clauses have been submitted */
void torch_conjecture_done();

/* model will now evaluate clauses in the context of the above conjectures,
 caching sub-term embeddings along the way */
float torch_eval_clause(Clause_p t);

#endif
