from setuptools import setup

with open("README.md", "r") as fh:
    long_description = fh.read()

setup(
    name="blingfire",
    version="0.1.8",
    author="Bling",
    author_email="bling@microsoft.com",
    description="Python wrapper of lightning fast Finite State Machine based NLP library.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/microsoft/blingfire/",
    packages=['blingfire'],
    package_data={'blingfire':['bert_base_tok.bin','bert_base_cased_tok.bin','bert_chinese.bin','bert_multi_cased.bin','wbd_chuni.bin','xlnet.bin','xlnet_nonorm.bin','xlm_roberta_base.bin','gpt2.bin','roberta.bin','laser100k.bin','laser250k.bin','laser500k.bin','uri100k.bin','uri250k.bin','uri500k.bin','syllab.bin','bert_base_cased_tok.i2w', 'bert_base_tok.i2w', 'bert_chinese.i2w', 'bert_multi_cased.i2w', 'gpt2.i2w', 'laser100k.i2w', 'laser250k.i2w', 'laser500k.i2w', 'roberta.i2w', 'uri100k.i2w', 'uri250k.i2w', 'uri500k.i2w', 'xlm_roberta_base.i2w', 'xlnet.i2w', 'xlnet_nonorm.i2w', 'libblingfiretokdll.so','blingfiretokdll.dll','blingfiretokdll.pdb','libblingfiretokdll.dylib']},
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
