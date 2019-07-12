#
# LDB compilation makefile
#
# Usage:
#
# 1. Compilation Makefile.gnu lang=<lang> [ mode=<mode> ] all
# 2. Auto-test   Makefile.gnu lang=<lang> [ mode=<mode> ] test
# 3. Performance Makefile.gnu lang=<lang> [ mode=<mode> ] perf
#

mode   = small
lang   = english

srcdir = $(lang)
tmpdir = $(srcdir)/tmp$(VERSION)
dstdir = ldb

include $(srcdir)/options.$(mode)

OUTPUT := $(dstdir)/$(OUTPUT)

#
# Variables adjustment
#

ifeq ($(USE_PREFIX), 1)
  built_prefix = $(tmpdir)/prefixes.fsa.$(mode).dump
endif

ifeq ($(USE_TEST_WTBT_DICT), 1)
  WTBT_DICT = $(tmpdir)/wtbt.dict.utf8
  TEST_WTBT_DICT = $(tmpdir)/test.wtbt.dict.utf8
  cat_wtbt_dict = cat $(WTBT_DICT)
  cat_test_wtbt_dict = cat $(TEST_WTBT_DICT)
else
  WTBT_DICT = $(srcdir)/wtbt.dict.utf8.zip
  TEST_WTBT_DICT = $(srcdir)/wtbt.dict.utf8.zip
  cat_wtbt_dict = unzip -p $(WTBT_DICT)
  cat_test_wtbt_dict = unzip -p $(TEST_WTBT_DICT)
endif

ifeq ($(TEST_COMP_DICT),)
  TEST_COMP_DICT = $(tmpdir)/test.comp.dict.utf8
endif
ifeq ($(cat_test_comp_dict),)
  cat_test_comp_dict = cat $(TEST_COMP_DICT)
endif

ifeq ($(USE_TEST_W2H_DICT), 1)
  W2H_DICT = $(tmpdir)/w2h.dict.utf8
  TEST_W2H_DICT = $(tmpdir)/test.w2h.dict.utf8
  cat_w2h_dict = cat $(W2H_DICT)
  cat_test_w2h_dict = cat $(TEST_W2H_DICT)
else
  W2H_DICT = $(srcdir)/w2h.dict.utf8.zip
  TEST_W2H_DICT = $(srcdir)/w2h.dict.utf8.zip
  cat_w2h_dict = unzip -p $(W2H_DICT)
  cat_test_w2h_dict = unzip -p $(TEST_W2H_DICT)
endif


#
# Special targets
#

DIRECTORIES = $(tmpdir)

$(DIRECTORIES):
	@for d in $(DIRECTORIES); do \
		{ [ -d $$d ] || mkdir -p $$d ; } ; \
	done

dirs: $(DIRECTORIES)

clean:
	cd $(tmpdir) && rm *

$(OUTPUT): $(tmpdir)/ldb.conf.$(mode).dump $(resources)
	fa_merge_dumps --out=$(OUTPUT) $(tmpdir)/ldb.conf.$(mode).dump $(resources)

all: dirs $(OUTPUT)


dist:
	@name=`realpath .`; name=`basename $$name`;         \
	rm -f ../$$name.dist.tar ;                          \
	tar -cvf ../$$name.dist.tar `find ../$$name -type f -maxdepth 2` 

#
# Output resources <-- Intermediate resources
#

$(tmpdir)/wt2bt.suff.fsa.$(mode).dump: $(tmpdir)/wt2bt.suff.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_suff_fsa) --in=$< --out=$@ --auto-test

$(tmpdir)/wt2bt.suff.acts.$(mode).dump: $(tmpdir)/wt2bt.suff.acts.txt
	fa_fsm2fsm_pack $(opt_pack_suff_acts) --in=$< --out=$@ --auto-test

$(tmpdir)/wt2b.suff.fsa.$(mode).dump: $(tmpdir)/wt2b.suff.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_suff_fsa) --in=$< --out=$@ --auto-test

$(tmpdir)/wt2b.suff.acts.$(mode).dump: $(tmpdir)/wt2b.suff.acts.txt
	fa_fsm2fsm_pack $(opt_pack_suff_acts) --in=$< --out=$@ --auto-test

$(tmpdir)/bt2wt.suff.fsa.$(mode).dump: $(tmpdir)/bt2wt.suff.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_suff_fsa) --in=$< --out=$@ --auto-test

$(tmpdir)/bt2wt.suff.acts.$(mode).dump: $(tmpdir)/bt2wt.suff.acts.txt
	fa_fsm2fsm_pack $(opt_pack_suff_acts) --in=$< --out=$@ --auto-test

$(tmpdir)/b2wt.suff.fsa.$(mode).dump: $(tmpdir)/b2wt.suff.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_suff_fsa) --in=$< --out=$@ --auto-test

$(tmpdir)/b2wt.suff.acts.$(mode).dump: $(tmpdir)/b2wt.suff.acts.txt
	fa_fsm2fsm_pack $(opt_pack_suff_acts) --in=$< --out=$@ --auto-test

$(tmpdir)/w2b.suff.fsa.$(mode).dump: $(tmpdir)/w2b.suff.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_suff_fsa) --in=$< --out=$@ --auto-test

$(tmpdir)/w2b.suff.acts.$(mode).dump: $(tmpdir)/w2b.suff.acts.txt
	fa_fsm2fsm_pack $(opt_pack_suff_acts) --in=$< --out=$@ --auto-test

$(tmpdir)/b2w.suff.fsa.$(mode).dump: $(tmpdir)/b2w.suff.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_suff_fsa) --in=$< --out=$@ --auto-test

$(tmpdir)/b2w.suff.acts.$(mode).dump: $(tmpdir)/b2w.suff.acts.txt
	fa_fsm2fsm_pack $(opt_pack_suff_acts) --in=$< --out=$@ --auto-test

$(tmpdir)/wg.fsa.$(mode).dump: $(tmpdir)/wg.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_word_guesser) --in=$< --out=$@ --auto-test

$(tmpdir)/prefixes.fsa.$(mode).dump: $(tmpdir)/prefixes.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_prefixes) --in=$< --out=$@ --auto-test

$(tmpdir)/segs.fsa.$(mode).dump: $(tmpdir)/segs.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_segs) --in=$< --out=$@ --auto-test

$(tmpdir)/tag.dict.fsm.$(mode).dump: $(tmpdir)/tag.dict.fsm.txt
	fa_fsm2fsm_pack $(opt_pack_dict_fsm) --in=$< --out=$@ --auto-test

$(tmpdir)/tag.dict.k2i.$(mode).dump: $(tmpdir)/tag.dict.k2i.txt
	fa_fsm2fsm_pack $(opt_pack_dict_k2i) --in=$< --out=$@ --auto-test

$(tmpdir)/tag.dict.i2t.$(mode).dump: $(tmpdir)/tag.dict.i2t.txt
	fa_fsm2fsm_pack $(opt_pack_dict_i2t) --in=$< --out=$@ --auto-test

$(tmpdir)/w2h.fsm.$(mode).dump: $(tmpdir)/w2h.fsm.txt
	fa_fsm2fsm_pack $(opt_pack_w2h_fsm) --in=$< --out=$@ --auto-test

$(tmpdir)/w2h.i2h.$(mode).dump: $(tmpdir)/w2h.i2h.txt
	fa_fsm2fsm_pack $(opt_pack_w2h_i2h) --in=$< --out=$@ --auto-test

$(tmpdir)/w2h.acts.$(mode).dump: $(tmpdir)/w2h.acts.txt
	fa_fsm2fsm_pack $(opt_pack_w2h_acts) --in=$< --out=$@ --auto-test

$(tmpdir)/dom.fsa.$(mode).dump: $(tmpdir)/dom.fsa.txt
	fa_fsm2fsm_pack $(opt_pack_dom) --in=$< --out=$@ --auto-test

$(tmpdir)/wbd.fsa.$(mode).dump: $(tmpdir)/wbd.rules.fsa.txt $(tmpdir)/wbd.rules.fsa.iwmap.txt
	fa_fsm2fsm_pack $(opt_pack_wbd_fsa) --in=$(tmpdir)/wbd.rules.fsa.txt --iw-map=$(tmpdir)/wbd.rules.fsa.iwmap.txt --out=$(tmpdir)/wbd.fsa.$(mode).dump

$(tmpdir)/wbd.mmap.$(mode).dump: $(tmpdir)/wbd.rules.map.txt
	fa_fsm2fsm_pack $(opt_pack_wbd_mmap) --in=$(tmpdir)/wbd.rules.map.txt --out=$(tmpdir)/wbd.mmap.$(mode).dump --auto-test

$(tmpdir)/charmap.mmap.$(mode).dump: $(tmpdir)/charmap.mmap.txt
	fa_fsm2fsm_pack $(opt_pack_charmap) --in=$(tmpdir)/charmap.mmap.txt --out=$(tmpdir)/charmap.mmap.$(mode).dump --auto-test


#
# Configuration compilation
#

$(tmpdir)/ldb.mmap.$(mode).txt: $(srcdir)/ldb.conf.$(mode)
	fa_build_conf \
	  --in=$(srcdir)/ldb.conf.$(mode) \
	  --out=$(tmpdir)/ldb.mmap.$(mode).txt

$(tmpdir)/ldb.conf.$(mode).dump: $(tmpdir)/ldb.mmap.$(mode).txt
	fa_fsm2fsm_pack --type=mmap \
	  --in=$(tmpdir)/ldb.mmap.$(mode).txt \
	  --out=$(tmpdir)/ldb.conf.$(mode).dump \
	  --auto-test


#
# Creation of testing dictionaries
#

$(tmpdir)/wtbt.dict.utf8 \
$(tmpdir)/test.wtbt.dict.utf8: $(srcdir)/wtbt.dict.utf8.zip
	unzip -p $(srcdir)/wtbt.dict.utf8.zip | \
	fa_wtbt2wtbt_split $(opt_test_wtbt_dict) \
	  --data=$(tmpdir)/test.wtbt.dict.utf8 > $(tmpdir)/wtbt.dict.utf8

$(tmpdir)/test.comp.dict.utf8: $(srcdir)/sf.utf8.zip
	unzip -p $(srcdir)/sf.utf8.zip | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[0]\n"' | \
	fa_segs2comps $(opt_test_comp_dict) > $(tmpdir)/test.comp.dict.utf8

$(tmpdir)/w2h.dict.utf8 \
$(tmpdir)/test.w2h.dict.utf8: $(srcdir)/w2h.dict.utf8.zip
	unzip -p $(srcdir)/w2h.dict.utf8.zip | \
	fa_split $(opt_test_w2h_dict) \
	  --out-test=$(tmpdir)/test.w2h.dict.utf8 > $(tmpdir)/w2h.dict.utf8


#
# Creation of performance dictionaries
#

$(tmpdir)/w2b.perf.utf8: $(srcdir)/wtbt.dict.utf8.zip
	unzip -p $(srcdir)/wtbt.dict.utf8.zip | \
	perl -ne 'chomp; @f = split(/[\t]/); print "w2b $$f[0]\n"; if($$f[2] ne $$prev) { print "w2b $$f[2]\n"; $$prev = $$f[2]; }' | \
	fa_cat_ratio $(opt_w2b_perf_dict) >$(tmpdir)/w2b.perf.utf8

$(tmpdir)/b2w.perf.utf8: $(srcdir)/wtbt.dict.utf8.zip
	unzip -p $(srcdir)/wtbt.dict.utf8.zip | \
	perl -ne 'chomp; @f = split(/[\t]/); if($$f[2] ne $$prev) { print "b2w $$f[2]\n"; $$prev = $$f[2]; }' | \
	fa_cat_ratio $(opt_b2w_perf_dict) >$(tmpdir)/b2w.perf.utf8

$(tmpdir)/w2t.perf.utf8: $(srcdir)/wtbt.dict.utf8.zip
	unzip -p $(srcdir)/wtbt.dict.utf8.zip | \
	perl -ne 'chomp; @f = split(/[\t]/); print "w2t $$f[0]\n"; if($$f[2] ne $$prev) { print "w2t $$f[2]\n"; $$prev = $$f[2]; }' | \
	fa_cat_ratio $(opt_w2t_perf_dict) >$(tmpdir)/w2t.perf.utf8

$(tmpdir)/w2s.perf.utf8: $(srcdir)/sf.utf8.zip
	unzip -p $(srcdir)/sf.utf8.zip | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[0]\n"' | \
	fa_segs2comps $(opt_test_comp_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "w2s $$f[0]\n"' >$(tmpdir)/w2s.perf.utf8

$(tmpdir)/tag.dict.perf.utf8: $(srcdir)/tag.dict.utf8.zip
	unzip -p $(srcdir)/tag.dict.utf8.zip | \
	perl -ne 'chomp; @f = split(/[\t]/); print "tag-dict $$f[0]\n"' | \
	fa_cat_ratio $(opt_td_perf_dict) >$(tmpdir)/tag.dict.perf.utf8

$(tmpdir)/w2h.perf.utf8: $(srcdir)/wtbt.dict.utf8.zip
	unzip -p $(srcdir)/wtbt.dict.utf8.zip | \
	perl -ne 'chomp; @f = split(/[\t]/); print "w2h $$f[0]\n"; if($$f[2] ne $$prev) { print "w2h $$f[2]\n"; $$prev = $$f[2]; }' | \
	fa_cat_ratio $(opt_w2h_perf_dict) >$(tmpdir)/w2h.perf.utf8


#
# Intermediate resources <-- Resources
#

$(tmpdir)/wt2b.suffs.utf8: $(WTBT_DICT) $(srcdir)/tagset.txt $(built_prefix)
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[0]\t$$f[2]\t$$f[1]\n"; if($$f[2] ne $$prev) { print "$$f[2]\t$$f[2]\t$$f[3]\n"; $$prev = $$f[2]; }' | \
	fa_dict2suff $(opt_dict2suff) $(opt_dict2suff_wt2b) \
	  --out=$(tmpdir)/wt2b.suffs.utf8

$(tmpdir)/wt2b.suff.fsa.txt \
$(tmpdir)/wt2b.suff.acts.txt: $(tmpdir)/wt2b.suffs.utf8 \
                               $(srcdir)/tagset.txt
	fa_build_suff $(opt_build_suff) $(opt_build_suff_wt2b) \
	  --in=$(tmpdir)/wt2b.suffs.utf8 \
	  --tagset=$(srcdir)/tagset.txt \
	  --out1=$(tmpdir)/wt2b.suff.fsa.txt \
	  --out2=$(tmpdir)/wt2b.suff.acts.txt

$(tmpdir)/b2wt.suffs.utf8: $(WTBT_DICT) $(srcdir)/tagset.txt $(built_prefix)
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[2]\t$$f[0]\t$$f[1]\n"; if($$f[2] ne $$prev) { print "$$f[2]\t$$f[2]\t$$f[3]\n"; $$prev = $$f[2]; }' | \
	fa_dict2suff $(opt_dict2suff) $(opt_dict2suff_b2wt) \
	  --out=$(tmpdir)/b2wt.suffs.utf8

$(tmpdir)/b2wt.suff.fsa.txt \
$(tmpdir)/b2wt.suff.acts.txt: $(tmpdir)/b2wt.suffs.utf8 \
                               $(srcdir)/tagset.txt
	fa_build_suff $(opt_build_suff) $(opt_build_suff_b2wt) \
	  --in=$(tmpdir)/b2wt.suffs.utf8 \
	  --tagset=$(srcdir)/tagset.txt \
	  --out1=$(tmpdir)/b2wt.suff.fsa.txt \
	  --out2=$(tmpdir)/b2wt.suff.acts.txt

$(tmpdir)/wt2bt.suffs.utf8: $(WTBT_DICT) $(srcdir)/tagset.txt $(built_prefix)
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[0]\t$$f[2]\t$$f[1]\t$$f[3]\n"; if($$f[2] ne $$prev) { print "$$f[2]\t$$f[2]\t$$f[3]\t$$f[3]\n"; $$prev = $$f[2]; }' | \
	fa_dict2suff $(opt_dict2suff) $(opt_dict2suff_wt2bt) \
	  --out=$(tmpdir)/wt2bt.suffs.utf8

$(tmpdir)/wt2bt.suff.fsa.txt \
$(tmpdir)/wt2bt.suff.acts.txt: $(tmpdir)/wt2bt.suffs.utf8 \
                               $(srcdir)/tagset.txt
	fa_build_suff $(opt_build_suff) $(opt_build_suff_wt2bt) \
	  --in=$(tmpdir)/wt2bt.suffs.utf8 \
	  --tagset=$(srcdir)/tagset.txt \
	  --out1=$(tmpdir)/wt2bt.suff.fsa.txt \
	  --out2=$(tmpdir)/wt2bt.suff.acts.txt

$(tmpdir)/bt2wt.suffs.utf8: $(WTBT_DICT) $(srcdir)/tagset.txt $(built_prefix)
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[2]\t$$f[0]\t$$f[3]\t$$f[1]\n"; if($$f[2] ne $$prev) { print "$$f[2]\t$$f[2]\t$$f[3]\t$$f[3]\n"; $$prev = $$f[2]; }' | \
	fa_dict2suff $(opt_dict2suff) $(opt_dict2suff_bt2wt) \
	  --out=$(tmpdir)/bt2wt.suffs.utf8

$(tmpdir)/bt2wt.suff.fsa.txt \
$(tmpdir)/bt2wt.suff.acts.txt: $(tmpdir)/bt2wt.suffs.utf8 \
                               $(srcdir)/tagset.txt
	fa_build_suff $(opt_build_suff) $(opt_build_suff_bt2wt) \
	  --in=$(tmpdir)/bt2wt.suffs.utf8 \
	  --tagset=$(srcdir)/tagset.txt \
	  --out1=$(tmpdir)/bt2wt.suff.fsa.txt \
	  --out2=$(tmpdir)/bt2wt.suff.acts.txt

$(tmpdir)/w2b.suffs.utf8: $(WTBT_DICT) $(built_prefix)
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[0]\t$$f[2]\n"; if($$f[2] ne $$prev) { print "$$f[2]\t$$f[2]\n"; $$prev = $$f[2]; }' | \
	fa_dict2suff $(opt_dict2suff) $(opt_dict2suff_w2b) \
	  --out=$(tmpdir)/w2b.suffs.utf8

$(tmpdir)/w2b.suff.fsa.txt \
$(tmpdir)/w2b.suff.acts.txt: $(tmpdir)/w2b.suffs.utf8
	fa_build_suff $(opt_build_suff) $(opt_build_suff_w2b) \
	  --in=$(tmpdir)/w2b.suffs.utf8 \
	  --out1=$(tmpdir)/w2b.suff.fsa.txt \
	  --out2=$(tmpdir)/w2b.suff.acts.txt

$(tmpdir)/b2w.suffs.utf8: $(WTBT_DICT) $(built_prefix)
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[2]\t$$f[0]\n"; if($$f[2] ne $$prev) { print "$$f[2]\t$$f[2]\n"; $$prev = $$f[2]; }' | \
	fa_dict2suff $(opt_dict2suff) $(opt_dict2suff_b2w) \
	  --out=$(tmpdir)/b2w.suffs.utf8

$(tmpdir)/b2w.suff.fsa.txt \
$(tmpdir)/b2w.suff.acts.txt: $(tmpdir)/b2w.suffs.utf8
	fa_build_suff $(opt_build_suff) $(opt_build_suff_b2w) \
	  --in=$(tmpdir)/b2w.suffs.utf8 \
	  --out1=$(tmpdir)/b2w.suff.fsa.txt \
	  --out2=$(tmpdir)/b2w.suff.acts.txt

$(tmpdir)/wg.fsa.txt: $(WTBT_DICT) $(srcdir)/tagset.txt
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[0]\t$$f[1]\n"; $$b = "$$f[2]\t$$f[3]"; if($$b ne $$prev) { print "$$b\n"; $$prev = $$b; }' | \
	fa_build_word_guesser $(opt_build_word_guesser) \
	  --tagset=$(srcdir)/tagset.txt --out=$(tmpdir)/wg.fsa.txt

$(tmpdir)/prefixes.fsa.txt: $(srcdir)/prefixes.utf8
	fa_line2chain_unicode --input-enc=UTF-8 < $(srcdir)/prefixes.utf8 | \
	sort | uniq | fa_chains2mindfa > $(tmpdir)/prefixes.fsa.txt

$(tmpdir)/segs.fsa.txt: $(srcdir)/sf.utf8.zip
	unzip -p $(srcdir)/sf.utf8.zip | \
	fa_line2chain_unicode $(opt_build_segs) --use-keys --key-base=65536 | \
	sort | uniq | fa_chains2mindfa | \
	fa_fsm_renum --alg=remove-gaps --fsm-type=rs-dfa | \
	fa_fsm2fsm --in-type=rs-dfa --out-type=moore-dfa \
	  --ow-base=65536 --ow-max=1000000 > $(tmpdir)/segs.fsa.txt

$(tmpdir)/tag.dict.fsm.txt \
$(tmpdir)/tag.dict.k2i.txt \
$(tmpdir)/tag.dict.i2t.txt: $(srcdir)/tag.dict.tagset.txt \
                             $(srcdir)/tag.dict.utf8.zip
	unzip -p $(srcdir)/tag.dict.utf8.zip | \
	fa_build_dict $(opt_build_dict) $(opt_build_tag_dict) \
	  --tagset=$(srcdir)/tag.dict.tagset.txt \
	  --out-fsm=$(tmpdir)/tag.dict.fsm.txt \
	  --out-k2i=$(tmpdir)/tag.dict.k2i.txt \
	  --out-i2info=$(tmpdir)/tag.dict.i2t.txt

$(tmpdir)/w2h.file.utf8: $(W2H_DICT)
	$(cat_w2h_dict) > $(tmpdir)/w2h.file.utf8

$(tmpdir)/w2h.pats.utf8: $(tmpdir)/w2h.file.utf8
	fa_build_pats $(opt_dict2pats) $(opt_dict2pats_w2h) \
	  --in=$(tmpdir)/w2h.file.utf8  \
	  --out=$(tmpdir)/w2h.pats.utf8 \
	  --out-actions=$(tmpdir)/w2h.acts.txt \
	  --out-unsolved=$(tmpdir)/w2h.unsolved.utf8

$(tmpdir)/w2h.fsm.txt \
$(tmpdir)/w2h.i2h.txt: $(tmpdir)/w2h.pats.utf8
	fa_build_dict --type=moore --raw \
	  --in=$(tmpdir)/w2h.pats.utf8 \
	  --out-fsm=$(tmpdir)/w2h.fsm.txt \
	  --out-i2info=$(tmpdir)/w2h.i2h.txt

$(tmpdir)/dom.fsa.txt: $(WTBT_DICT)
	$(cat_wtbt_dict) | \
	perl -ne 'chomp; @f = split(/[\t]/); print "$$f[0]\n$$f[2]\n";' | \
	fa_line2chain_unicode $(opt_build_dom) | \
	sort | uniq | fa_chains2mindfa | \
	fa_fsm_renum --alg=remove-gaps --fsm-type=rs-dfa >$(tmpdir)/dom.fsa.txt

$(tmpdir)/wbd.rules.fsa.txt \
$(tmpdir)/wbd.rules.fsa.iwmap.txt \
$(tmpdir)/wbd.rules.map.txt: $(srcdir)/wbd.lex.utf8 $(srcdir)/wbd.tagset.txt
	fa_build_lex $(opt_build_wbd) --in=$(srcdir)/wbd.lex.utf8 \
	  --tagset=$(srcdir)/wbd.tagset.txt --out-fsa=$(tmpdir)/wbd.rules.fsa.txt \
	  --out-fsa-iwmap=$(tmpdir)/wbd.rules.fsa.iwmap.txt \
	  --out-map=$(tmpdir)/wbd.rules.map.txt

$(tmpdir)/charmap.mmap.txt: $(srcdir)/charmap.utf8
	fa_charmap2mmap < $(srcdir)/charmap.utf8 > $(tmpdir)/charmap.mmap.txt


#
# auto-test targets
#

test: test_w2b \
      test_unknown_w2b \
      test_b2w \
      test_unknown_b2w \
      test_w2t \
      test_unknown_w2t \
      test_w2s \
      test_tag_dict \
      test_w2h \
      test_unknown_w2h


test_w2b: $(OUTPUT) $(WTBT_DICT)
	$(cat_wtbt_dict) | fa_wtbt2wbs $(opt_auto_test) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.w2b.kn.utf8 \
	  $(opt_auto_test) --auto-test=1

test_unknown_w2b: $(OUTPUT) $(TEST_WTBT_DICT)
	$(cat_test_wtbt_dict) | fa_wtbt2wbs $(opt_auto_test) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.w2b.un.utf8 \
	  $(opt_auto_test) --auto-test=1


test_b2w: $(OUTPUT) $(WTBT_DICT)
	$(cat_wtbt_dict) | fa_wtbt2bws $(opt_auto_test) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.b2w.kn.utf8 \
	  $(opt_auto_test) --auto-test=2

test_unknown_b2w: $(OUTPUT) $(TEST_WTBT_DICT)
	$(cat_test_wtbt_dict) | fa_wtbt2bws $(opt_auto_test) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.b2w.un.utf8 \
	  $(opt_auto_test) --auto-test=2


test_w2t: $(OUTPUT) $(WTBT_DICT)
	$(cat_wtbt_dict) | fa_wtbt2wts $(opt_auto_test) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.w2t.kn.utf8 \
	  --tagset=$(srcdir)/tagset.txt $(opt_auto_test) --auto-test=4

test_unknown_w2t: $(OUTPUT) $(TEST_WTBT_DICT)
	$(cat_test_wtbt_dict) | fa_wtbt2wts $(opt_auto_test) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.w2t.un.utf8 \
	  --tagset=$(srcdir)/tagset.txt $(opt_auto_test) --auto-test=4


test_w2s: $(OUTPUT) $(TEST_COMP_DICT)
	$(cat_test_comp_dict) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.w2s.utf8 \
	  $(opt_auto_test) --auto-test=3


test_tag_dict: $(OUTPUT) $(srcdir)/tag.dict.utf8.zip
	unzip -p $(srcdir)/tag.dict.utf8.zip | \
	fa_wts2wts $(opt_auto_test) $(opt_dict_auto_test) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.tag.dict.utf8 \
	  --tagset=$(srcdir)/tag.dict.tagset.txt --auto-test=5


test_w2h: $(OUTPUT) $(W2H_DICT)
	$(cat_w2h_dict) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.w2h.kn.utf8 \
	  $(opt_auto_test) --auto-test=6

test_unknown_w2h: $(OUTPUT) $(TEST_W2H_DICT)
	$(cat_test_w2h_dict) | \
	test_ldb --ldb=$(OUTPUT) --error-log=$(tmpdir)/errors.w2h.un.utf8 \
	  $(opt_auto_test) --auto-test=6


#
# Performance measurement targets
#

perf: perf_w2b \
      perf_b2w \
      perf_w2t \
      perf_w2s \
      perf_tag_dict \
      perf_w2h


perf_w2b: $(OUTPUT) $(tmpdir)/w2b.perf.utf8
	test_perf --verbose --hires $(opt_auto_perf) --cmd test_ldb \
	--ldb=$(OUTPUT) $(opt_auto_test) --data=$(tmpdir)/w2b.perf.utf8 && \
	perl -ne 'chomp; @f = split(/[ ]/); if($$#f > 0) { print "$$f[1]\n" }' < $(tmpdir)/w2b.perf.utf8 | \
	fa_line2chain_unicode --input-enc=UTF-8 | \
	perl -ne 'chomp; @f = split(/[ ]/); $$c += (1 + $$#f); END { print "Count $$c\n" }'

perf_b2w: $(OUTPUT) $(tmpdir)/b2w.perf.utf8
	test_perf --verbose --hires $(opt_auto_perf) --cmd test_ldb \
	--ldb=$(OUTPUT) $(opt_auto_test) --data=$(tmpdir)/b2w.perf.utf8 && \
	perl -ne 'chomp; @f = split(/[ ]/); if($$#f > 0) { print "$$f[1]\n" }' < $(tmpdir)/b2w.perf.utf8 | \
	fa_line2chain_unicode --input-enc=UTF-8 | \
	perl -ne 'chomp; @f = split(/[ ]/); $$c += (1 + $$#f); END { print "Count $$c\n" }'

perf_w2t: $(OUTPUT) $(tmpdir)/w2t.perf.utf8
	test_perf --verbose --hires $(opt_auto_perf) --cmd test_ldb \
	--ldb=$(OUTPUT) $(opt_auto_test) --data=$(tmpdir)/w2t.perf.utf8 && \
	perl -ne 'chomp; @f = split(/[ ]/); if($$#f > 0) { print "$$f[1]\n" }' < $(tmpdir)/w2t.perf.utf8 | \
	fa_line2chain_unicode --input-enc=UTF-8 | \
	perl -ne 'chomp; @f = split(/[ ]/); $$c += (1 + $$#f); END { print "Count $$c\n" }'

perf_w2s: $(OUTPUT) $(tmpdir)/w2s.perf.utf8
	test_perf --verbose --hires $(opt_auto_perf) --cmd test_ldb \
	--ldb=$(OUTPUT) $(opt_auto_test) --data=$(tmpdir)/w2s.perf.utf8 && \
	perl -ne 'chomp; @f = split(/[ ]/); if($$#f > 0) { print "$$f[1]\n" }' < $(tmpdir)/w2s.perf.utf8 | \
	fa_line2chain_unicode --input-enc=UTF-8 | \
	perl -ne 'chomp; @f = split(/[ ]/); $$c += (1 + $$#f); END { print "Count $$c\n" }'

perf_tag_dict: $(OUTPUT) $(tmpdir)/tag.dict.perf.utf8
	test_perf --verbose --hires $(opt_auto_perf) --cmd test_ldb \
	--ldb=$(OUTPUT) $(opt_auto_test) --data=$(tmpdir)/tag.dict.perf.utf8 && \
	perl -ne 'chomp; @f = split(/[ ]/); if($$#f > 0) { print "$$f[1]\n" }' < $(tmpdir)/tag.dict.perf.utf8 | \
	fa_line2chain_unicode --input-enc=UTF-8 | \
	perl -ne 'chomp; @f = split(/[ ]/); $$c += (1 + $$#f); END { print "Count $$c\n" }'

perf_w2h: $(OUTPUT) $(tmpdir)/w2h.perf.utf8
	test_perf --verbose --hires $(opt_auto_perf) --cmd test_ldb \
	--ldb=$(OUTPUT) $(opt_auto_test) --data=$(tmpdir)/w2h.perf.utf8 && \
	perl -ne 'chomp; @f = split(/[ ]/); if($$#f > 0) { print "$$f[1]\n" }' < $(tmpdir)/w2h.perf.utf8 | \
	fa_line2chain_unicode --input-enc=UTF-8 | \
	perl -ne 'chomp; @f = split(/[ ]/); $$c += (1 + $$#f); END { print "Count $$c\n" }'
