import getopt
import sys
import zipfile
import os
import spacy
import nltk
import time
from blingfiretok import *
import urllib.request
import json

number_of_docs = 1100
warm_up_until = 100
data = ""
output_tokenization_result = False
dataset = "gigawords"


def main():
    # process args
    parse_args()

    # setup packages
    setup_packages()

    # load data
    documents = load_document(data, dataset)

    # run benchmarks
    benchmark_blingfire(documents)
    benchmark_spacy(documents)
    benchmark_nltk(documents)


def setup_packages():
    try:
        nltk.data.find("tokenizers/punkt")
    except LookupError:
        nltk.download("punkt")


def parse_args():
    global number_of_docs
    global data
    global output_tokenization_result
    global dataset
    global warm_up_until

    try:
        opts, args = getopt.getopt(sys.argv[1:], "d:n:w:s:o")
    except getopt.GetoptError as err:
        # print help information and exit:
        print("Error in args. Please check.")
        print(err)
        sys.exit(2)

    for opt, arg in opts:
        if opt == "-d":
            data = arg
        elif opt == "-n":
            number_of_docs = int(arg)
        elif opt == "-o":
            output_tokenization_result = True
        elif opt == "-s":
            dataset = arg
        elif opt == "-w":
            warm_up_until = int(arg)


# load_gigawords function is referred from GitHub https://github.com/explosion/spacy-benchmarks
def load_gigawords(datapath):
    doc = []
    para = []
    result = []
    in_doc = False
    in_paragraph = False
    lines = open(datapath, "r").read().replace("&AMP;", "&").split("\n")

    for line in lines:
        if len(result) >= number_of_docs:
            return result

        line = line.strip()
        if not line:
            pass
        elif line[0] != "<":
            if in_paragraph:
                para.append(line)
        elif line.startswith("<DOC"):
            in_doc = True
        elif line.startswith("<P>"):
            assert in_doc
            in_paragraph = True
        elif line.startswith("</DOC"):
            assert not in_paragraph
            in_doc = False
            result.append("\n\n".join(doc))
            doc = []
        elif line.startswith("</P>"):
            doc.append(" ".join(para))
            in_paragraph = False
            para = []
        else:
            pass

    return result


def load_marco(datapath):
    marco_dataset = []
    doc_in_json = open(datapath, "r", encoding="utf8").read().split("\n")[:number_of_docs]
    for doc in doc_in_json:
        try:
            deserialized = json.loads(doc)
            marco_dataset.append(deserialized["document_text"])
        except json.decoder.JSONDecodeError:
            pass

    return marco_dataset


def load_plaintext(datapath):
    return open(datapath, "r", encoding="utf8").read().split("\n")[:number_of_docs]


def load_document(input_data, dataset):
    if len(input_data) <= 0:
        raise Exception("invalid input path")

    if str.lower(dataset) == "marco":
        return load_marco(input_data)
    elif str.lower(dataset) == "englishgigawords":
        return load_gigawords(input_data)
    else:
        return load_plaintext(input_data)


def write_to_textfile(data, lib_name):
    if os.path.exists(lib_name):
        os.remove(lib_name)

    f = open(lib_name, "w+")
    f.write("\n".join(data))
    f.close()


def benchmark_spacy(docset):
    print("running spacy benchmarking")
    nlp = spacy.load("en_core_web_sm", disable=["parser", "tagger", "ner"])
    tokenized_docs_raw = []
    i = 0
    warm_up_time = time.time()  # default
    for doc in docset:
        if i == warm_up_until + 1:
            warm_up_time = time.time()  # end of warm up, start timing
        tokens = nlp(doc)
        tokenized_docs_raw.append(tokens)
        i += 1
    print("------spacy: %s seconds for %d documents------" % ((time.time() - warm_up_time), len(docset) - warm_up_until))

    # output tokenization result to a text file, including warm-ups
    if output_tokenization_result:
        ready_to_write = []
        for raw_doc in tokenized_docs_raw:
            doc_text = []
            for word in raw_doc:
                doc_text.append(word.text)

            doc_text_string = " ".join(doc_text)
            ready_to_write.append(doc_text_string)

        write_to_textfile(ready_to_write, "benchmark_spacy.txt")
        print("Wrote tokenized docs to benchmark_spacy.txt")


def benchmark_nltk(docset):
    print("running nltk benchmarking")
    tokenized_docs_raw = []
    warm_up_time = time.time()  # default
    i = 0
    for doc in docset:
        if i == warm_up_until + 1:
            warm_up_time = time.time()  # end of warm up, start timing
        tokens = nltk.word_tokenize(doc)
        tokenized_docs_raw.append(tokens)
        i += 1
    print("------nltk: %s seconds for %d documents------" % ((time.time() - warm_up_time), len(docset) - warm_up_until))

    # output tokenization result to a text file
    if output_tokenization_result:
        ready_to_write = []
        for raw_doc in tokenized_docs_raw:
            ready_to_write.append(" ".join(raw_doc))

        write_to_textfile(ready_to_write, "benchmark_nltk.txt")
        print("Wrote tokenized docs to benchmark_nltk.txt")


def benchmark_blingfire(docset):
    print("running blingfire benchmarking")
    tokenized_docs_raw = []
    warm_up_time = time.time()  # default
    i = 0
    for doc in docset:
        if i == warm_up_until + 1:
            warm_up_time = time.time()  # end of warm up, start timing
        tokens = text_to_words(doc)
        tokenized_docs_raw.append(tokens)
        i += 1
    print("------blingfire: %s seconds for %d documents------" % ((time.time() - warm_up_time), len(docset) - warm_up_until))

    # output tokenization result to a text file
    if output_tokenization_result:
        write_to_textfile(tokenized_docs_raw, "benchmark_blingfire.txt")
        print("Wrote tokenized docs to benchmark_blingfire.txt")


if __name__ == "__main__":
    main()
