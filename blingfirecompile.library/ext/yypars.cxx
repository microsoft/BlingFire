/* Microsoft VCBU Internal C++ YACC Parser v1.00.4101, Sundeep Bhatia */
#include "yypars.h"
#line 4 "$Pyypars.cxx"

#pragma warning(disable : 4244)

# define YYFLAG 				-1000
# define YYERROR 				goto yyerrlab
# define YYACCEPT 				return(0)
# define YYABORT				return(1)

#ifdef YYDEBUG
# ifndef YYDBFLG
#  define YYDBFLG				(yydebug)
# endif
# define yyprintf				if (YYDBFLG) YYPRINT
#else
# define yyprintf
#endif

#ifndef YYPRINT
# define YYPRINT				printf
#endif



#pragma warning(disable:102)
int YYPARSER::Parse()
{
	short yyj, yym, yyn;	//local variables that hold states

yystack:
//put a state and value onto the stack
#ifdef YYDEBUG
	if(YYAPI_TOKENNAME == YYAPI_TOKENNONE) {
		Trace("token # '%s' in state %2d\n", "-1", yystate);
	}
	else {
		Trace("token '%s', in state %2d\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), yystate);
	}
#endif
	if(++yyps >= &yys[YYMAXDEPTH - 1]) { // -1 is to avoid reference of uninitialized yypv[1]
#ifdef UNICODE_PARSER
		yyerror(L"yacc stack overflow");
#else
		yyerror("yacc stack overflow");
#endif
		return(1);
	}
	*yyps = yystate;		//put state on stack
	++yypv;
	*yypv = yyval;			//put value on stack

yynewstate:
//shift
	yyn = yypact[yystate];

	if (yyn <= YYFLAG) {	//simple state, no lookahead
#pragma prefast(suppress:5469, "kevinhum: verified goto")
		goto yydefault;
	}
	if (YYAPI_TOKENNAME == YYAPI_TOKENNONE) {	//need a lookahead
		YYAPI_TOKENNAME = YYLEX();
		YYAPI_CALLAFTERYYLEX(YYAPI_TOKENNAME);
	}
	if (((yyn += (short)YYAPI_TOKENEME(YYAPI_TOKENNAME)) < 0) || (yyn >= YYLAST)) {
#pragma prefast(suppress:5469, "kevinhum: verified goto")
		goto yydefault;
	}
	if (yychk[yyn = yyact[yyn]] == YYAPI_TOKENEME(YYAPI_TOKENNAME)) {	//valid shift
		yyval = YYAPI_VALUEOF(YYAPI_VALUENAME);
		yystate = yyn;
#ifdef YYDEBUG
		Trace("SHIFT: saw token '%s', now in state %2d\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), yystate);
#endif
		YYAPI_TOKENNAME = YYAPI_TOKENNONE;
		if( yyerrflag > 0 ) {
			--yyerrflag;
		}
#pragma prefast(suppress:5461 5469, "kevinhum: verified goto")
		goto yystack;
	}

yydefault:
//default state action
	if ((yyn = yydef[yystate]) == -2) {
		const short *yyxi;

		if (YYAPI_TOKENNAME == YYAPI_TOKENNONE) {
			YYAPI_TOKENNAME = YYLEX();
			YYAPI_CALLAFTERYYLEX(YYAPI_TOKENNAME);
#ifdef YYDEBUG
			Trace("LOOKAHEAD: token '%s'\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME));
#endif
		}

		//Search exception table, we find a -1 followed by the current state.
		//if we find one, we'll look through (terminal, state) pairs. if we find
		//a terminal which matches the current one, we have a match.
		//The exception table is used when we have a reduce on a terminal.

#if YYOPTTIME
		yyxi = yyexca + yyexcaind[yystate];
		while ((*yyxi != YYAPI_TOKENEME(YYAPI_TOKENNAME)) && (*yyxi >= 0)) {
			yyxi += 2;
		}
#else
		for (yyxi = yyexca; (*yyxi != (-1)) || (yyxi[1] != yystate); yyxi += 2) {
			; //void
		}

		while (*(yyxi += 2) >= 0) {
			if (*yyxi == YYAPI_TOKENEME(YYAPI_TOKENNAME)) {
				break;
			}
		}
#endif
		if((yyn = yyxi[1]) < 0) {
			return(0);		//accept
		}
	}

	if (yyn == 0) { //error

		//error ... attempt to resume parsing
		switch ( yyerrflag ) {
			case 0:	// brand new error
#ifdef YYRECOVER
			{
				int	i,j;
				for (i = 0; (yyrecover[i] != -1000) && (yystate > yyrecover[i]); i += 3) {
					; //void
				}
				if( yystate == yyrecover[i]) {
#ifdef YYDEBUG
					Trace("recovered, from state %2d to state %2d on token # %2d\n", yystate, yyrecover[i+2], yyrecover[i+1]);
#endif
					j = yyrecover[i + 1];
					if (j < 0) {
						//here we have one of the injection set, so we're not quite
						//sure that the next valid thing will be a shift. so we'll
						//count it as an error and continue.
						//actually we're not absolutely sure that the next token
						//we were supposed to get is the one when j > 0. for example,
						//for(+) {;} error recovery with yyerrflag always set, stops
						//after inserting one ; before the +. at the point of the +,
						//we're pretty sure the guy wants a 'for' loop. without
						//setting the flag, when we're almost absolutely sure, we'll
						//give him one, since the only thing we can shift on this
						//error is after finding an expression followed by a +
						yyerrflag++;
						j = -j;
					}
					if (yyerrflag <= 1) {	//only on first insertion
						yyrecerr(YYAPI_TOKENNAME, j);	//what was, what should be first
					}
					yyval = yyeval(j);
					yystate = yyrecover[i + 2];
					goto yystack;
				}
			}
#endif
yyerrlab:
//syntax error
#ifdef UNICODE_PARSER
			yyerror(L"syntax error");
#else
			yyerror("syntax error");
#endif
			++yynerrs;
                        __fallthrough;
                        
			case 1:
                        __fallthrough;

			case 2: //incompletely recovered error ... try again
				yyerrflag = 3;

				//find a state where "error" is a legal shift action
				while (yyps >= yys) {
					yyn = yypact[*yyps] + YYERRCODE;
					if (yyn >= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE) {
						yystate = yyact[yyn];	//simulate a shift of "error"
#ifdef YYDEBUG
						Trace("SHIFT 'error': now in state %2d\n", yystate);
#endif
#pragma prefast(suppress:5461 5469, "kevinhum: verified goto")
						goto yystack;
					}
					yyn = yypact[*yyps];
					//the current yyps has no shift on "error", pop stack
#ifdef YYDEBUG
					Trace("error recovery pops state %2d, uncovers %2d\n", *yyps, yyps[-1]);
#endif
					--yyps;
					--yypv;
				}

yyabort:			
//there is no state on the stack with an error shift ... abort
				return(1);

			case 3:  //no shift yet; clobber input char
#ifdef YYDEBUG
				Trace("error recovery discards token '%s'\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME));
#endif
#pragma prefast(suppress:5461 5469, "kevinhum: verified goto")
				if (YYAPI_TOKENEME(YYAPI_TOKENNAME) == 0) goto yyabort; //don't discard EOF, quit
				YYAPI_TOKENNAME = YYAPI_TOKENNONE;
#pragma prefast(suppress:5461 5469, "kevinhum: verified goto")
				goto yynewstate;		//try again in the same state
		}
	}

yyreduce:
//reduction by production yyn
	{
		YYSTYPE *yypvt;
		yypvt = yypv;
		yyps -= yyr2[yyn];
		yypv -= yyr2[yyn];
		yyval = yypv[1];
#ifdef YYDEBUG
		Trace("REDUCE: rule %4d, popped %2d tokens, uncovered state %2d, ", yyn, (short)yyr2[yyn], *yyps);
#endif
		yym = yyn;
		yyn = (short)yyr1[yyn];				//consult goto table to find next state
		yyj = yypgo[yyn] + *yyps + 1;
		if((yyj >= YYLAST) || (yychk[ yystate = yyact[yyj] ] != -yyn)) {
			yystate = yyact[yypgo[yyn]];
		}
#ifdef YYDEBUG
		Trace("goto state %2d\n", yystate);
#endif
		switch (yym) {
			$A
#line 239 "$Pyypars.cxx"
		}
	}
#pragma prefast(suppress:5461 5469, "kevinhum: verified goto")
	goto yystack;	// stack new state and value
}
#pragma warning(default:102)

#ifdef YYDUMP
void YYPARSER::DumpYYS()
{
	short stackindex;

	yyprintf("short yys[%d] {\n", YYMAXDEPTH);
	for (stackindex = 0; stackindex < YYMAXDEPTH; stackindex++) {
		if (stackindex)
			yyprintf(", %s", stackindex % 10 ? "\0" : "\n");
		yyprintf("%6d", yys[stackindex]);
		}
	yyprintf("\n};\n");
}

void YYPARSER::DumpYYV()
{
	short valindex;

	yyprintf("YYSTYPE yyv[%d] {\n", YYMAXDEPTH);
	for (valindex = 0; valindex < YYMAXDEPTH; valindex++) {
		if (valindex)
			yyprintf(", %s", valindex % 5 ? "\0" : "\n");
		yyprintf("%#*x", 3+sizeof(YYSTYPE), yyv[valindex]);
		}
	yyprintf("\n};\n");
}
#endif

int YYPARSER::NoOfErrors() const
{
	return yynerrs;
}

int YYPARSER::ErrRecoveryState() const
{
	return yyerrflag;
}

void YYPARSER::ClearErrRecoveryState()
{
	yyerrflag = 0;
}

YYAPI_TOKENTYPE YYPARSER::GetCurrentToken() const
{
	return YYAPI_TOKENNAME;
}

void YYPARSER::SetCurrentToken(YYAPI_TOKENTYPE newToken)
{
	YYAPI_TOKENNAME = newToken;
}


#ifdef YYDEBUG
void YYPARSER::Trace(const char *message, const char *tokname, short state /*= 0*/)
{
#pragma prefast(suppress:5441, "kevinhum: (yydebug) test")
	yyprintf(message, tokname, state);
}

void YYPARSER::Trace(const char *message, int state, short tostate /*= 0*/, short token /*= 0*/)
{
#pragma prefast(suppress:5441, "kevinhum: (yydebug) test")
	yyprintf(message, state, tostate, token);
}
#endif

