
## ProofWatch Implementation in E

### Instructions

The following describes how to use the ProofWatch [1] implementation.

1) Load multiple watchlists using `--watchlist-dir`.

2) Use a custom heuristic `-H` with PreferProofWatch priority function.
For example:

```sh
eprover -H'(1*Clauseweight(PreferProofWatch,1,1,1))' --watchlist-dir=watchlists  agatha.p
```

### Inherited Relevance

To use the relevance inherited from parents of a clause, use
`--proofwatch-inherit-relevance`.  Set parameters delta, alpha, and beta from
the paper [1] using `--proofwatch-decay`, `--proofwatch-alpha`, and/or,
`--proofwatch-beta`.

### Training Vectors

To output ProofWatch proof progress with training examples (for example, to be
used with ENIGMAWatch) use `--training-proofwatch`.

### Analyse and Debug

To output the ProofWatch proof progress whenever a watchlist clause is matched,
use `-l1`.

To see the relevance values and further debug, compile with `DEBUG_PROOF_WATCH`.

## Credits

Partially supported by the AI4REASON ERC Consolidator grant number 649043, and
by the Czech project AI&Reasoning CZ.02.1.01/0.0/0.0/15003/0000466 and the
European Regional Development Fund.

## References

[1]: Zarathustra Goertzel, Jan Jakubuv, Stephan Schulz, Josef Urban:
     ProofWatch: Watchlist Guidance for Large Theories in E. 
     ITP 2018: 270-288, https://doi.org/10.1007/978-3-319-94821-8\_16.

