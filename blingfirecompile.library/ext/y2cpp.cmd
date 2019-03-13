./yacc -cxx FARegexpParser_msyacc.y

perl -pe "s/yypars.h/FARegexpParser_msyacc.h/; s/YYLEX\(\)/yylex\(\)/; s/YYSTYPE yyval;//; s/YYPARSER/FARegexpParser_msyacc/g; s/assert/DebugLogAssert/g;" < FARegexpParser_msyacc.cxx > FARegexpParser_msyacc.cpp

