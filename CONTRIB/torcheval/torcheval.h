#ifndef __torcheval__
#define __torcheval__

#include <ccl_clauses.h>

/* initialize whatever and get ready for receiving conjecture clauses */
void te_init(char* basename);
/* first add all conjecture clauses one by one (order matters, what's the covention?) */
void te_conjecture_clause(Clause_p cl);
/* call this when all conjecture clauses have been submitted */
void te_conjecture_done();

/* model will now evaluate clauses in the context of the above conjectures,
 caching sub-term embeddings along the way */
// returns "raw_data[1]-raw_data[0]"
// this can be fed into sigmoid: 1.0 / (1.0 + exp(raw_data[1]-raw_data[0])) // less is better
// or just compared against zero for a bool answer // <0 is YES
float te_eval_clause(Clause_p t);

#endif
