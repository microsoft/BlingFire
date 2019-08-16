from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys
import tensorflow as tf
import tokenization
import numpy as np
#from blingfiretok import *

class TokenizationTest(tf.test.TestCase):

  def test_full_tokenizer(self):

    tokenizer = tokenization.FullTokenizer(vocab_file="./vocab.txt", do_lower_case=True)

    for line in sys.stdin:

        line = line.strip()

        tokens = tokenizer.tokenize(line)
        #print(tokens)

        ids = tokenizer.convert_tokens_to_ids(tokens)

        padded_ids = [0]*128
        for i in range(min(128, len(ids))):
            padded_ids[i] = ids[i]

        print(line)
        print(np.array(padded_ids))

    self.assertAllEqual([1], [1])


if __name__ == "__main__":
  tf.test.main()
