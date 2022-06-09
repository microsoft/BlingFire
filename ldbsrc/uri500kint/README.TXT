This model was created the same way as ldbsrc/uri250k/README.TXT describes

URI  - is a URL
int  - in the suffix stands for international
nnnK - is the number of unique IDs model produce

Training time difference with non-int models:
  1. we selected only clicked URLs (not all random URLs) for training
  2. we selected evenly for the top countries and all data for other countries and then made a uniform random subsample
  3. URLs were UTF-8 decoded and Punycode decoded (so at runtime international URLs should be also UTF-8 and Punycode decoded)

Runtime differnce with non-int models:
  1. Essentialy you suppose to feed what you see in a browser line as input in UTF-8 encoding, in case URL is international. 
     (As with non-int models, there is no need to normalize the case, the model expects to get original case.)


# see ldbsrc/uri250k/README.TXT for more details
