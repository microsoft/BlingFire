from setuptools import setup

with open("README.md", "r") as fh:
    long_description = fh.read()

setup(
    name="blingfire",
    version="0.1.7",
    author="Bling",
    author_email="bling@microsoft.com",
    description="Python wrapper of lightning fast Finite State Machine based NLP library.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/microsoft/blingfire/",
    packages=['blingfire'],
    package_data={'blingfire':['bert_base_tok.bin','bert_base_cased_tok.bin','bert_chinese.bin','bert_multi_cased.bin','wbd_chuni.bin','xlnet.bin','xlnet_nonorm.bin','xlm_roberta_base.bin','gpt2.bin','roberta.bin','laser100k.bin','laser250k.bin','laser500k.bin','uri100k.bin','uri250k.bin','uri500k.bin','syllab.bin','libblingfiretokdll.so','blingfiretokdll.dll','blingfiretokdll.pdb','libblingfiretokdll.dylib']},
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
