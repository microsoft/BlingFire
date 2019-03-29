
# Bling Fire

## Introduction

Hi, we are a small team at Microsoft called Bling (Beyond Language Understanding), we help Bing be smarter. Here we wanted to share with all of you our Finite State machine and REgular expression manipulation library (FIRE). We use Fire for many several linguistic operations inside Bing such as Tokenization, Multi-word expression matching, Unknown word-guessing, Stemming / Lemmatization just to mention a few.

## Bling Fire Tokenizer

Bling Fire Tokenizer is a tokenizer designed for fast-speed and quality tokenization of Natural Language text. It mostly follows the tokenization logic of NLTK, except hyphenated words are split and a few errors are fixed. Currently released model supports most of the languages except East Asian (Chinese Simplified, Traditional, Japanese, Korean, Thai). You should expect good results if a language uses space as a main token delimitter. The tokenizer high level API designed in a way that it requires 0 configuration, or initialization, or additional files and is friendly for non-native languages like Python, Perl, C#, Java, etc. It is fast as uses deterministic finite state machines underneath.

## Getting Started

To start using Bling Fire Library and Finite State Machine manipulation tools, you can build the project on Windows/Linux with [CMake](https://cmake.org/). You need this if you want to create your own tokenization / segmentation, stemming etc. logic or need finite state machines for any other need. If you simply want to use the existing Python library you don't need this step.

For Python users, you can install the latest release using [pip](https://pypi.org/project/pip/).
`pip install blingfire`



## Benchmarking

Comparing Bling Fire with other popular NLP libraries, Bling Fire shows **10X faster** speed in tokenization task

| System   | Avg Run Time (Second Per 10,000 Passages) |
|------------|---------------------------------------|
| Bling Fire | 0.823                                 |
| SpaCy      | 8.653                                 |
| NLTK       | 17.821                                |

See more at [benchmark wiki](https://github.com/Microsoft/BlingFire/wiki/Benchmark-Guide)

## Example code
### Python
```python
from blingfire import *
text = 'This is the Bling-Fire tokenizer'
output = text_to_words(text)
```
### Jupyter Notebook

[This notebook](/doc/Bling%20Fire%20Tokenizer%20Demo.ipynb) demostrates how Bling Fire tokenizer helps in Stack Overflow posts classification problem.

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

To contribute directly to code base, you should create a personal fork and create feature branches there when you need them. This keeps the main repository clean and your personal workflow cruft out of sight.

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
