%{
/* (c) Copyright 2003-2004 by Sergei Olonichev */

#include "FAConfig.h"
#include "FARegexpParser_msyacc.h"
#include "FARegexpTree.h"
#include "FAToken.h"

%}


%union{
  int node_id;
  const FAToken* value;
}

/* terminals description
   !!! BEWARE !!!
   These values must be synced with FARegexpTree::TYPE_ values */
%token  <value> FA_SYMBOL              257
%token  <value> FA_ANY                 258
%token  <value> FA_LBR                 259
%token  <value> FA_RBR                 260
%token  <value> FA_CONCAT              261
%token  <value> FA_DISJUNCTION         262
%token  <value> FA_ITERATION           263
%token  <value> FA_NON_EMPTY_ITERATION 264
%token  <value> FA_OPTIONAL            265
%token  <value> FA_BOUND_LBR           266
%token  <value> FA_COMMA               267
%token  <value> FA_BOUND_RBR           268
%token  <value> FA_EPSILON             269
%token  <value> FA_INTERVAL_LBR        270
%token  <value> FA_INTERVAL_RBR        271
%token  <value> FA_L_ANCHOR            272
%token  <value> FA_R_ANCHOR            273
%token  <value> FA_LTRBR               274
%token  <value> FA_RTRBR               275


/* non-terminals description */
%type <node_id> regexp disj conc re


/* grammar section */
%%


regexp: 

disj {
  assert (m_pTree);

  m_parse_failed = false;
  m_pTree->SetRoot ($1);
  m_pTree->SetParent ($1, -1);
}
;


disj:

conc {

  $$ = $1;
}
|
disj FA_DISJUNCTION conc {

  assert ($2);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($2->GetType (), $2->GetOffset (), $2->GetLength ());

  /* adjust links */
  m_pTree->SetLeft ($$, $1);
  m_pTree->SetParent ($1, $$);
  m_pTree->SetRight ($$, $3);
  m_pTree->SetParent ($3, $$);
}
;


conc:

re {
  $$ = $1;
}
|
conc re {

  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode (FARegexpTree::TYPE_CONCAT, -1, 0);

  /* adjust links */
  m_pTree->SetLeft ($$, $1);
  m_pTree->SetParent ($1, $$);
  m_pTree->SetRight ($$, $2);
  m_pTree->SetParent ($2, $$);
}
;


re:

FA_SYMBOL {

  assert ($1);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($1->GetType (), $1->GetOffset (), $1->GetLength ());
}
|
FA_L_ANCHOR {

  assert ($1);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($1->GetType (), $1->GetOffset (), $1->GetLength ());
}
|
FA_R_ANCHOR {

  assert ($1);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($1->GetType (), $1->GetOffset (), $1->GetLength ());
}
|
FA_ANY {

  assert ($1);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($1->GetType (), $1->GetOffset (), $1->GetLength ());
}
|
FA_LBR disj FA_RBR {

  $$ = $2;
}
|
FA_LTRBR disj FA_RTRBR {

  assert (m_pRegexp);

  const int Offset = $1->GetOffset ();
  const int Length = $1->GetLength ();

  assert (0 <= Offset);
  assert (Offset + Length + 1 <= m_RegexpLen);

  if (0 < Length) {

    const int TrBr = atoi (m_pRegexp + Offset + 1);
    m_pTree->SetTrBr ($2, TrBr);

  } else {

    m_pTree->SetTrBr ($2, 0);
  }

  m_pTree->SetTrBrOffset ($2, Offset);

  $$ = $2;
}
|
re FA_ITERATION {

  assert ($2);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($2->GetType (), $2->GetOffset (), $2->GetLength ());

  /* adjust links */
  m_pTree->SetLeft ($$, $1);
  m_pTree->SetParent ($1, $$);
}
|
re FA_NON_EMPTY_ITERATION {

  assert ($2);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($2->GetType (), $2->GetOffset (), $2->GetLength ());

  /* adjust links */
  m_pTree->SetLeft ($$, $1);
  m_pTree->SetParent ($1, $$);
}
|
re FA_OPTIONAL {

  /* newly added */
  assert ($2);
  assert (m_pTree);

  /* create a new node */
  $$ = m_pTree->AddNode ($2->GetType (), $2->GetOffset (), $2->GetLength ());

  /* adjust links */
  m_pTree->SetLeft ($$, $1);
  m_pTree->SetParent ($1, $$);
}
;


%%


/**************************** service methods ********************************/

void YYPARSER::yyerror (const char * /*szMsg*/)
{
    m_parse_failed = true;
}


int YYPARSER::yylex ()
{
    assert (m_pTokens);

    /* if there are no more tokens for processing, finish parsing */
    if (m_token_count <= m_token_pos) {

        m_token_pos = 0;
        return 0;
    }

    /* get token id and semantic value */
    const FAToken * pToken = & m_pTokens [m_token_pos];
    assert (pToken);

    m_token_pos++;
    yylval.value = pToken;
    const int Id = pToken->GetType ();

    return Id;
}

/**************************** public methods *********************************/

YYPARSER::YYPARSER ()
{
   m_pTree = NULL;
   m_pTokens = NULL;
   m_token_count = -1;
   m_token_pos = 0;
   m_parse_failed = false;
   m_pRegexp = NULL;
   m_RegexpLen = 0;
}

YYPARSER::~YYPARSER ()
{
  Clear ();
}

void YYPARSER::SetTokens (const FAToken * pTokens, const int TokensCount)
{
  m_pTokens = pTokens;
  m_token_count = TokensCount;
}

void YYPARSER::SetRegexp (const char * pRegexp, const int RegexpLen)
{
    m_pRegexp = pRegexp;
    m_RegexpLen = RegexpLen;
}

void YYPARSER::SetTree (FARegexpTree * pTree)
{
  m_pTree = pTree;
}

const bool YYPARSER::GetStatus () const
{
  return m_parse_failed;
}

const int YYPARSER::GetTokenIdx () const
{
  return m_token_pos;
}

void YYPARSER::Clear ()
{
  m_token_pos = 0;
  m_parse_failed = false;

  ResetState ();
}

void YYPARSER::Process ()
{
  assert (m_pTree);
  assert (m_pTokens);
  assert (0 <= m_token_count);

  Clear ();

  m_pTree->Clear ();

  m_parse_failed = true;

  Parse();
}
