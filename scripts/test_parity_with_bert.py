from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys
import tensorflow as tf
import tokenization
import numpy as np
import argparse
from blingfire import *

parser = argparse.ArgumentParser()
parser.add_argument("-v", "--vocab", default="./vocab.txt", help="Vocabulary to use, ./vocab.txt is used by default")
parser.add_argument("-i", "--ignore-case", help="Ignore case, False by default")
parser.add_argument("-s", "--no-output", default=False, help="No output, False by default")
parser.add_argument("-n", "--no-process", default=False, help="No process, False by default")
args = parser.parse_args()

h = load_model("./ldb/bert_multi_cased.bin~")

class TokenizationTest(tf.test.TestCase):

  def test_full_tokenizer(self):

    tokenizer = tokenization.FullTokenizer(vocab_file=args.vocab, do_lower_case=False)

    for line in sys.stdin:

        line = line.strip()

        if not args.no_process:
            tokens = tokenizer.tokenize(line)
            ids = tokenizer.convert_tokens_to_ids(tokens)
            padded_ids = [0]*128
            for i in range(min(128, len(ids))):
                padded_ids[i] = ids[i]

            ids2 = text_to_ids(h, line, 128, 100)
            if not np.array_equal(ids2, padded_ids):
                if not args.no_output:
                    print(line)
                    print(np.array(padded_ids))
                    print("produced:")
                    print(ids2)

    self.assertAllEqual([1], [1])


if __name__ == "__main__":
  tf.test.main()
