!include $(SRCROOT)\nlg\src\inc\autobuild.all

#################### YACC rules #########################

.SUFFIXES : .y

.y{$(O)}.cpp:
	copy $< $(O)\$<
	$(YACC) -cxx $(YFLAGS) $(O)\$<
	$(PERL) -p -e "s/yypars.h/$(<B).h/;\
		       s/YYLEX\(\)/yylex\(\)/;\
		       s/YYSTYPE yyval;//;\
		       s/YYPARSER/$(<B)/g"\
		$(O)\$(<B).cxx > $(O)\$(<B).cpp
	del $(O)\$(<B).cxx
