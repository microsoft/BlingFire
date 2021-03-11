
# Bling Fire

## Introduction

Hi, we are a team at Microsoft called Bling (Beyond Language Understanding), we help Bing be smarter. Here we wanted to share with all of you our **FI**nite State machine and **RE**gular expression manipulation library (**FIRE**). We use Fire for many linguistic operations inside Bing such as Tokenization, Multi-word expression matching, Unknown word-guessing, Stemming / Lemmatization just to mention a few.

## Bling Fire Tokenizer Overview

Bling Fire Tokenizer provides state of the art performance for Natural Language text tokenization. Bling Fire supports the following tokenization algorithms:

1. Pattern-based tokenization
2. [WordPiece](https://arxiv.org/pdf/1609.08144.pdf) tokenization
3. [SentencePiece](https://github.com/google/sentencepiece) Unigram LM
4. [SentencePiece](https://github.com/google/sentencepiece) BPE
5. Induced/learned syllabification patterns (identifies possible hyphenation points within a token)

Bling Fire provides uniform interface for working with all four algorithms so there is no difference for the client whether to use tokenizer for XLNET, BERT or your own custom model.

Model files describe the algorithms they are built for and are loaded on demand from external file. There are also two default models for NLTK-style tokenization and sentence breaking, which does not need to be loaded. The default tokenization model follows logic of NLTK, except hyphenated words are split and a few "errors" are fixed. 

Normalization can be added to each model, but is optional. 

Diffrences between algorithms are [summarized here](https://github.com/microsoft/BlingFire/blob/master/doc/Bling_Fire_Tokenizer_Algorithms.pdf).

Bling Fire Tokenizer high level API designed in a way that it requires minimal or no configuration, or initialization, or additional files and is friendly for use from languages like Python, Ruby, Rust, C#, JavaScript (via WASM), etc.

We have precompiled some popular models and listed with the source code reference below:

| File Name   | Models it should be used for | Algorithm | Source Code |
|------------|---------------------------------------|----|----|
| wbd.bin | Default Tokenization Model    | Pattern-based | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/wbd) | 
| sbd.bin | Default model for Sentence breaking    | Pattern-based | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/sbd) | 
| bert_base_tok.bin | BERT Base/Large                                 | WordPiece | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/bert_base_tok) |
| bert_base_cased_tok.bin      | BERT Base/Large Cased                                 | WordPiece | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/bert_base_cased_tok) | 
| bert_chinese.bin       | BERT Chinese                                | WordPiece | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/bert_chinese) | 
| bert_multi_cased.bin       | BERT Multi Lingual Cased                                | WordPiece | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/bert_multi_cased) | 
| xlnet.bin | XLNET Tokenization Model    | Unigram LM | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/xlnet) | 
| xlnet_nonorm.bin | XLNET Tokenization Model /wo normalization    | Unigram LM | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/xlnet_nonorm) | 
| bpe_example.bin | A model to test BPE tokenization    | BPE | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/bpe_example) | 
| xlm_roberta_base.bin | XLM Roberta Tokenization | Unigram LM | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/xlm_roberta_base) | 
| laser(100k\|250k\|500k).bin | Trained on balanced by language WikiMatrix corpus of 80+ languages | Unigram LM | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/laser100k) | 
| uri(100k\|250k\|500k).bin | URL tokenization model trained on a large set of random URLs from the web | Unigram LM | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/uri250k) |
| gpt2.bin | Byte-BPE tokenization model for GPT-2 | byte BPE | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/gpt2) |
| roberta.bin | Byte-BPE tokenization model for Roberta model | byte BPE | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/roberta) |
| syllab.bin | Multi lingual model to identify allowed hyphenation points inside a word. | W2H | [src](https://github.com/microsoft/BlingFire/tree/master/ldbsrc/syllab) |

Oh yes, it is also the fastest! We did a comparison of Bling Fire with tokenizers from Hugging Face, [Bling Fire runs 4-5 times faster than Hugging Face Tokenizers](https://github.com/Microsoft/BlingFire/wiki/Comparing-performance-of-Bling-Fire-and-Hugging-Face-Tokenizers), see also [Bing Blog Post](https://blogs.bing.com/Developers-Blog/march-2020/Bling-FIRE-Tokenizer-for-BERT). We did comparison of Bling Fire Unigram LM and BPE implementaion to the same one in [SentencePiece](https://github.com/google/sentencepiece) library and our implementation is ~2x faster, see [XLNET benchmark](https://github.com/microsoft/BlingFire/blob/master/ldbsrc/xlnet/README.TXT) and [BPE benchmark](https://github.com/microsoft/BlingFire/blob/master/ldbsrc/bpe_example/README.TXT). Not to mention our default models are 10x faster than the same functionality from [SpaCy](https://github.com/explosion/spaCy), see [benchmark wiki](https://github.com/Microsoft/BlingFire/wiki/Benchmark-Guide) and this [Bing Blog Post](https://blogs.bing.com/Developers-Blog/2019-04/bling-fire-tokenizer-released-to-open-source).

So if low latency inference is what you need then you have to try Bling Fire!


## Python API Description

If you simply want to use it in Python, you can install the latest release using [pip](https://pypi.org/project/pip/):

`pip install -U blingfire`


## Examples
### 1. Python example, using default pattern-based tokenizer:
```python
from blingfire import *

text = 'After reading this post, you will know: What "natural language" is and how it is different from other types of data. What makes working with natural language so challenging. [1]'

print(text_to_sentences(text))
print(text_to_words(text))
```

Expected output:
```
After reading this post, you will know: What "natural language" is and how it is different from other types of data.
What makes working with natural language so challenging. [1]
After reading this post , you will know : What " natural language " is and how it is different from other types of data . What makes working with natural language so challenging . [ 1 ]
```


### 2. Python example, load a custom model for a pattern-based tokenizer: 

```python
from blingfire import *

# load a custom model from file
h = load_model("./wbd_chuni.bin")

text = 'This is the Bling-Fire tokenizer. 2007年9月日历表_2007年9月农历阳历一览表-万年历'

# custom model output
print(text_to_words_with_model(h, text))

# default model output
print(text_to_words(text))

free_model(h)
```

Expected output:
```
This is the Bling - Fire tokenizer . 2007 年 9 月 日 历 表 _2007 年 9 月 农 历 阳 历 一 览 表 - 万 年 历
This is the Bling - Fire tokenizer . 2007年9月日历表_2007年9月农历阳历一览表 - 万年历
```


### 3. Python example, calling BERT BASE tokenizer
On one thread, it works 14x faster than orignal BERT tokenizer written in Python. Given this code is written in C++ it can be called from multiple threads without blocking on global interpreter lock thus achieving higher speed-ups for batch mode.

```python
import os
import blingfire

s = "Эpple pie. How do I renew my virtual smart card?: /Microsoft IT/ 'virtual' smart card certificates for DirectAccess are valid for one year. In order to get to microsoft.com we need to type pi@1.2.1.2."

# one time load the model (we are using the one that comes with the package)
h = blingfire.load_model(os.path.join(os.path.dirname(blingfire.__file__), "bert_base_tok.bin"))
print("Model Handle: %s" % h)

# use the model from one or more threads
print(s)
ids = blingfire.text_to_ids(h, s, 128, 100)  # sequence length: 128, oov id: 100
print(ids)                                   # returns a numpy array of length 128 (padded or trimmed)

# free the model at the end
blingfire.free_model(h)
print("Model Freed")
```

Expected output:
```
Model Handle: 2854016629088
Эpple pie. How do I renew my virtual smart card?: /Microsoft IT/ 'virtual' smart card certificates for DirectAccess are valid for one year. In order to get to microsoft.com we need to type pi@1.2.1.2.
[ 1208  9397  2571 11345  1012  2129  2079  1045 20687  2026  7484  6047
  4003  1029  1024  1013  7513  2009  1013  1005  7484  1005  6047  4003
 17987  2005  3622  6305  9623  2015  2024  9398  2005  2028  2095  1012
  1999  2344  2000  2131  2000  7513  1012  4012  2057  2342  2000  2828
 14255  1030  1015  1012  1016  1012  1015  1012  1016  1012     0     0
     0     0     0     0     0     0     0     0     0     0     0     0
     0     0     0     0     0     0     0     0     0     0     0     0
     0     0     0     0     0     0     0     0     0     0     0     0
     0     0     0     0     0     0     0     0     0     0     0     0
     0     0     0     0     0     0     0     0     0     0     0     0
     0     0     0     0     0     0     0     0]
Model Freed
```


## Supported Platforms

Bling Fire is supported for Windows, Linux and Mac (Thanks to Andrew Kane!)


## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Working Branch

To contribute directly to code base, you should create a personal fork and create feature branches there when you need them. This keeps the main repository clean and your personal workflow out of sight.

### Pull Request

Before we can accept a pull request from you, you'll need to sign a  [Contributor License Agreement (CLA)](https://en.wikipedia.org/wiki/Contributor_License_Agreement). It is an automated process and you only need to do it once.

However, you don't have to do this up-front. You can simply clone, fork, and submit your pull-request as usual. When your pull-request is created, it is classified by a CLA bot. If the change is trivial (i.e. you just fixed a typo) then the PR is labelled with  `cla-not-required`. Otherwise, it's classified as  `cla-required`. In that case, the system will also tell you how you can sign the CLA. Once you have signed a CLA, the current and all future pull-requests will be labelled as  `cla-signed`.

To enable us to quickly review and accept your pull requests, always create one pull request per issue and [link the issue in the pull request](https://github.com/blog/957-introducing-issue-mentions) if possible. Never merge multiple requests in one unless they have the same root cause. Besides, keep code changes as small as possible and avoid pure formatting changes to code that has not been modified otherwise.

## Feedback

* Ask a question on [Stack Overflow](https://stackoverflow.com/questions/tagged/blingfire).
* File a bug in [GitHub Issues](https://github.com/Microsoft/BlingFire/issues).

## Reporting Security Issues

Security issues and bugs should be reported privately, via email, to the Microsoft Security
Response Center (MSRC) at [secure@microsoft.com](mailto:secure@microsoft.com). You should
receive a response within 24 hours. If for some reason you do not, please follow up via
email to ensure we received your original message. Further information, including the
[MSRC PGP](https://technet.microsoft.com/en-us/security/dn606155) key, can be found in
the [Security TechCenter](https://technet.microsoft.com/en-us/security/default).

## License

Copyright (c) Microsoft Corporation. All rights reserved.

Licensed under the [MIT](LICENSE) License.
