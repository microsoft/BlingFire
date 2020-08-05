/*
 * C++ Parser created by Microsoft VCBU Internal YACC from "FARegexpParser_msyacc.y"
 */

#line 2 "FARegexpParser_msyacc.y"
/* (c) Copyright 2003-2004 by Sergei Olonichev */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpParser_msyacc.h"
#include "FARegexpTree.h"
#include "FAToken.h"

namespace BlingFire
{

#line 12 "FARegexpParser_msyacc.y"

#define UNION 1
typedef union {
  int node_id;
  const FAToken* value;
} YYSTYPE;
# define FA_SYMBOL 257 
# define FA_ANY 258 
# define FA_LBR 259 
# define FA_RBR 260 
# define FA_CONCAT 261 
# define FA_DISJUNCTION 262 
# define FA_ITERATION 263 
# define FA_NON_EMPTY_ITERATION 264 
# define FA_OPTIONAL 265 
# define FA_BOUND_LBR 266 
# define FA_COMMA 267 
# define FA_BOUND_RBR 268 
# define FA_EPSILON 269 
# define FA_INTERVAL_LBR 270 
# define FA_INTERVAL_RBR 271 
# define FA_L_ANCHOR 272 
# define FA_R_ANCHOR 273 
# define FA_LTRBR 274 
# define FA_RTRBR 275 
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

#ifndef YYFARDATA
#define	YYFARDATA	/*nothing*/
#endif
#if ! defined YYSTATIC
#define	YYSTATIC	/*nothing*/
#endif
#if ! defined YYCONST
#define	YYCONST	/*nothing*/
#endif
#ifndef	YYACT
#define	YYACT	yyact
#endif
#ifndef	YYPACT
#define	YYPACT	yypact
#endif
#ifndef	YYPGO
#define	YYPGO	yypgo
#endif
#ifndef	YYR1
#define	YYR1	yyr1
#endif
#ifndef	YYR2
#define	YYR2	yyr2
#endif
#ifndef	YYCHK
#define	YYCHK	yychk
#endif
#ifndef	YYDEF
#define	YYDEF	yydef
#endif
#ifndef	YYV
#define	YYV	yyv
#endif
#ifndef	YYS
#define	YYS	yys
#endif
#ifndef	YYLOCAL
#define	YYLOCAL
#endif
#ifndef YYR_T
#define	YYR_T	int
#endif
typedef	YYR_T	yyr_t;
#ifndef YYEXIND_T
#define	YYEXIND_T	unsigned int
#endif
typedef	YYEXIND_T	yyexind_t;
#ifndef YYOPTTIME
#define	YYOPTTIME	0
#endif
# define YYERRCODE 256
# define YYNPROD 15
# define YYLAST 32
/* Microsoft VCBU Internal C++ YACC Parser v1.00.4101, Sundeep Bhatia */
#include "FARegexpParser_msyacc.h"
#line 4 "d:\\src\\indexgen\\private\\shared\\msfsa\\ext\\yypars.cxx"

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
int FARegexpParser_msyacc::Parse()
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
		YYAPI_TOKENNAME = yylex();
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
			YYAPI_TOKENNAME = yylex();
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
			
case 1:
#line 51 "FARegexpParser_msyacc.y"
{
  DebugLogAssert (m_pTree);

  m_parse_failed = false;
  m_pTree->SetRoot (yypvt[-0].node_id);
  m_pTree->SetParent (yypvt[-0].node_id, -1);
} break;
case 2:
#line 63 "FARegexpParser_msyacc.y"
{

  yyval.node_id = yypvt[-0].node_id;
} break;
case 3:
#line 68 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (yypvt[-1].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-1].value->GetType (), yypvt[-1].value->GetOffset (), yypvt[-1].value->GetLength ());

  /* adjust links */
  m_pTree->SetLeft (yyval.node_id, yypvt[-2].node_id);
  m_pTree->SetParent (yypvt[-2].node_id, yyval.node_id);
  m_pTree->SetRight (yyval.node_id, yypvt[-0].node_id);
  m_pTree->SetParent (yypvt[-0].node_id, yyval.node_id);
} break;
case 4:
#line 87 "FARegexpParser_msyacc.y"
{
  yyval.node_id = yypvt[-0].node_id;
} break;
case 5:
#line 91 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (FARegexpTree::TYPE_CONCAT, -1, 0);

  /* adjust links */
  m_pTree->SetLeft (yyval.node_id, yypvt[-1].node_id);
  m_pTree->SetParent (yypvt[-1].node_id, yyval.node_id);
  m_pTree->SetRight (yyval.node_id, yypvt[-0].node_id);
  m_pTree->SetParent (yypvt[-0].node_id, yyval.node_id);
} break;
case 6:
#line 109 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (yypvt[-0].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-0].value->GetType (), yypvt[-0].value->GetOffset (), yypvt[-0].value->GetLength ());
} break;
case 7:
#line 118 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (yypvt[-0].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-0].value->GetType (), yypvt[-0].value->GetOffset (), yypvt[-0].value->GetLength ());
} break;
case 8:
#line 127 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (yypvt[-0].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-0].value->GetType (), yypvt[-0].value->GetOffset (), yypvt[-0].value->GetLength ());
} break;
case 9:
#line 136 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (yypvt[-0].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-0].value->GetType (), yypvt[-0].value->GetOffset (), yypvt[-0].value->GetLength ());
} break;
case 10:
#line 145 "FARegexpParser_msyacc.y"
{

  yyval.node_id = yypvt[-1].node_id;
} break;
case 11:
#line 150 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (m_pRegexp);

  const int Offset = yypvt[-2].value->GetOffset ();
  const int Length = yypvt[-2].value->GetLength ();

  DebugLogAssert (0 <= Offset);
  DebugLogAssert (Offset + Length + 1 <= m_RegexpLen);

  if (0 < Length) {

    const int TrBr = atoi (m_pRegexp + Offset + 1);
    m_pTree->SetTrBr (yypvt[-1].node_id, TrBr);

  } else {

    m_pTree->SetTrBr (yypvt[-1].node_id, 0);
  }

  m_pTree->SetTrBrOffset (yypvt[-1].node_id, Offset);

  yyval.node_id = yypvt[-1].node_id;
} break;
case 12:
#line 175 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (yypvt[-0].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-0].value->GetType (), yypvt[-0].value->GetOffset (), yypvt[-0].value->GetLength ());

  /* adjust links */
  m_pTree->SetLeft (yyval.node_id, yypvt[-1].node_id);
  m_pTree->SetParent (yypvt[-1].node_id, yyval.node_id);
} break;
case 13:
#line 188 "FARegexpParser_msyacc.y"
{

  DebugLogAssert (yypvt[-0].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-0].value->GetType (), yypvt[-0].value->GetOffset (), yypvt[-0].value->GetLength ());

  /* adjust links */
  m_pTree->SetLeft (yyval.node_id, yypvt[-1].node_id);
  m_pTree->SetParent (yypvt[-1].node_id, yyval.node_id);
} break;
case 14:
#line 201 "FARegexpParser_msyacc.y"
{

  /* newly added */
  DebugLogAssert (yypvt[-0].value);
  DebugLogAssert (m_pTree);

  /* create a new node */
  yyval.node_id = m_pTree->AddNode (yypvt[-0].value->GetType (), yypvt[-0].value->GetOffset (), yypvt[-0].value->GetLength ());

  /* adjust links */
  m_pTree->SetLeft (yyval.node_id, yypvt[-1].node_id);
  m_pTree->SetParent (yypvt[-1].node_id, yyval.node_id);
} break;/* End of actions */
#line 239 "d:\\src\\indexgen\\private\\shared\\msfsa\\ext\\yypars.cxx"
		}
	}
#pragma prefast(suppress:5461 5469, "kevinhum: verified goto")
	goto yystack;	// stack new state and value
}
#pragma warning(default:102)

#ifdef YYDUMP
void FARegexpParser_msyacc::DumpYYS()
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

void FARegexpParser_msyacc::DumpYYV()
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

int FARegexpParser_msyacc::NoOfErrors() const
{
	return yynerrs;
}

int FARegexpParser_msyacc::ErrRecoveryState() const
{
	return yyerrflag;
}

void FARegexpParser_msyacc::ClearErrRecoveryState()
{
	yyerrflag = 0;
}

YYAPI_TOKENTYPE FARegexpParser_msyacc::GetCurrentToken() const
{
	return YYAPI_TOKENNAME;
}

void FARegexpParser_msyacc::SetCurrentToken(YYAPI_TOKENTYPE newToken)
{
	YYAPI_TOKENNAME = newToken;
}


#ifdef YYDEBUG
void FARegexpParser_msyacc::Trace(const char *message, const char *tokname, short state /*= 0*/)
{
#pragma prefast(suppress:5441, "kevinhum: (yydebug) test")
	yyprintf(message, tokname, state);
}

void FARegexpParser_msyacc::Trace(const char *message, int state, short tostate /*= 0*/, short token /*= 0*/)
{
#pragma prefast(suppress:5441, "kevinhum: (yydebug) test")
	yyprintf(message, state, tostate, token);
}
#endif


#line 217 "FARegexpParser_msyacc.y"



/**************************** service methods ********************************/

void FARegexpParser_msyacc::yyerror (const char * /*szMsg*/)
{
    m_parse_failed = true;
}


int FARegexpParser_msyacc::yylex ()
{
    DebugLogAssert (m_pTokens);

    /* if there are no more tokens for processing, finish parsing */
    if (m_token_count <= m_token_pos) {

        m_token_pos = 0;
        return 0;
    }

    /* get token id and semantic value */
    const FAToken * pToken = & m_pTokens [m_token_pos];
    DebugLogAssert (pToken);

    m_token_pos++;
    yylval.value = pToken;
    const int Id = pToken->GetType ();

    return Id;
}

/**************************** public methods *********************************/

FARegexpParser_msyacc::FARegexpParser_msyacc ()
{
   m_pTree = NULL;
   m_pTokens = NULL;
   m_token_count = -1;
   m_token_pos = 0;
   m_parse_failed = false;
   m_pRegexp = NULL;
   m_RegexpLen = 0;
}

FARegexpParser_msyacc::~FARegexpParser_msyacc ()
{
  Clear ();
}

void FARegexpParser_msyacc::SetTokens (const FAToken * pTokens, const int TokensCount)
{
  m_pTokens = pTokens;
  m_token_count = TokensCount;
}

void FARegexpParser_msyacc::SetRegexp (const char * pRegexp, const int RegexpLen)
{
    m_pRegexp = pRegexp;
    m_RegexpLen = RegexpLen;
}

void FARegexpParser_msyacc::SetTree (FARegexpTree * pTree)
{
  m_pTree = pTree;
}

const bool FARegexpParser_msyacc::GetStatus () const
{
  return m_parse_failed;
}

const int FARegexpParser_msyacc::GetTokenIdx () const
{
  return m_token_pos;
}

void FARegexpParser_msyacc::Clear ()
{
  m_token_pos = 0;
  m_parse_failed = false;

  ResetState ();
}

void FARegexpParser_msyacc::Process ()
{
  DebugLogAssert (m_pTree);
  DebugLogAssert (m_pTokens);
  DebugLogAssert (0 <= m_token_count);

  Clear ();

  m_pTree->Clear ();

  m_parse_failed = true;

  Parse();
}
YYCONST short FARegexpParser_msyacc::yyexca[] = {
#if !(YYOPTTIME)
-1, 1,
#endif
	0, -1,
	-2, 0,

};

#if YYOPTTIME
YYCONST yyexind_t FARegexpParser_msyacc::yyexcaind[] = {
    0,    0
};
#endif

YYCONST short FARegexpParser_msyacc::yyact[] = {
    5,    8,    9,   13,   14,   15,   11,   19,   11,   11,
    4,    3,    1,    0,   12,    6,    7,   10,    0,   20,
    2,    0,    0,   18,    0,    0,    0,    0,    0,   12,
   16,   17
};

YYCONST short FARegexpParser_msyacc::yypact[] = {
 -257,-1000, -254, -257, -260,-1000,-1000,-1000,-1000, -257,
 -257, -257, -260,-1000,-1000,-1000, -253, -256, -257,-1000,
-1000
};

YYCONST short FARegexpParser_msyacc::yypgo[] = {
    0,   12,   20,   11,   10
};

YYCONST yyr_t FARegexpParser_msyacc::yyr1[] = {
    0,    1,    2,    2,    3,    3,    4,    4,    4,    4,
    4,    4,    4,    4,    4
};

YYCONST yyr_t FARegexpParser_msyacc::yyr2[] = {
    0,    1,    1,    3,    1,    2,    1,    1,    1,    1,
    3,    3,    2,    2,    2
};

YYCONST short FARegexpParser_msyacc::yychk[] = {
-1000,   -1,   -2,   -3,   -4,  257,  272,  273,  258,  259,
  274,  262,   -4,  263,  264,  265,   -2,   -2,   -3,  260,
  275
};

YYCONST short FARegexpParser_msyacc::yydef[] = {
    0,   -2,    1,    2,    4,    6,    7,    8,    9,    0,
    0,    0,    5,   12,   13,   14,    0,    0,    3,   10,
   11
};
#ifdef YYRECOVER
YYCONST short FARegexpParser_msyacc::yyrecover[] = {
-1000
};
#endif

}
