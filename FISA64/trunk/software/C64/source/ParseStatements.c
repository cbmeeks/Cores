// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2014  Robert Finch, Stratford
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@finitron.ca
//       ||
//
// C64 - Raptor64 'C' derived language compiler
//  - 64 bit CPU
//
// This source file is free software: you can redistribute it and/or modify 
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or     
// (at your option) any later version.                                      
//                                                                          
// This source file is distributed in the hope that it will be useful,      
// but WITHOUT ANY WARRANTY; without even the implied warranty of           
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            
// GNU General Public License for more details.                             
//                                                                          
// You should have received a copy of the GNU General Public License        
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    
//                                                                          
// ============================================================================
//
#include <stdio.h> 
#include <string.h>
#include "c.h" 
#include "expr.h" 
#include "Statement.h"
#include "gen.h"
#include "cglbdec.h" 

extern TYP *head, *tail;
extern TYP stdbyte;
extern char *declid;
extern int catchdecl;
Statement *ParseStatement();   /* forward declararation */ 
Statement *ParseCatchStatement();
int iflevel;
Statement *currentStmt;
char *llptr;
extern char *lptr;
extern char inpline[132];

Statement *NewStatement(int typ, int gt) {
	Statement *s = (Statement *)xalloc(sizeof(Statement));
	memset(s, '\0', sizeof(Statement));
	s->stype = typ;
	s->predreg = -1;
	s->outer = currentStmt;
	s->s1 = (Statement *)NULL;
	s->s2 = (Statement *)NULL;
	s->ssyms.head = (SYM *)NULL;
	s->ssyms.tail = (SYM *)NULL;
	s->lptr = litlate(inpline);
	//memset(s->ssyms,0,sizeof(s->ssyms));
	if (gt) NextToken();
	return s;
};


int GetTypeHash(TYP *p)
{
	int n;
	TYP *p1;

	n = 0;
	do {
		if (p->type==bt_pointer)
			n+=20000;
		p1 = p;
		p = p->btp;
	} while (p);
	n += p1->typeno;
	return n;
}


//struct snode    *interrupt_stmt()
//{       struct snode    *snp; 
//        SYM             *sp; 
//	       TYP     *temp1;
//
//	   NextToken(); 
//		if( lastst != id ) { 
//			error(ERR_IDEXPECT); 
//			return 0; 
//        } 
//        sp = allocSYM(); 
//        sp->name = litlate(lastid); 
//        sp->value.i = nextlabel++; 
//        sp->storage_class = sc_global;
//        sp->tp = temp1; 
//        temp1 = maketype(bt_long,0);
//        temp1->val_flag = 1;
//        insert(sp,&lsyms); 
//        NextToken();       /* get past label name */ 
//        needpunc( colon );
//		snp = (struct snode *)xalloc(sizeof(struct snode)); 
//        snp->stype = st_interrupt; 
//        snp->label = sp->name;
//        snp->next = 0; 
//		snp->s1 = statement(); 
//        return snp; 
//} 
//  

Statement *vortex_stmt()
{       Statement *snp; 
        SYM             *sp; 
	       TYP     *temp1;

	   NextToken(); 
		if( lastst != id ) { 
			error(ERR_IDEXPECT); 
			return 0; 
        } 
        temp1 = maketype(bt_long,0);
        temp1->val_flag = 1;
        sp = allocSYM(); 
        sp->name = litlate(lastid); 
        sp->value.i = nextlabel++; 
        sp->storage_class = sc_global;
        sp->tp = temp1; 
        insert(sp,&lsyms); 
        NextToken();       /* get past label name */ 
        needpunc( colon );
		snp = (struct snode *)xalloc(sizeof(struct snode)); 
        snp->stype = st_vortex; 
        snp->label = (int64_t *)sp->name;
        snp->next = 0; 
		snp->s1 = ParseStatement(); 
        return snp; 
} 
  
Statement *ParseCheckStatement() 
{       
	Statement *snp;
    snp = NewStatement(st_check, TRUE);
    if( expression(&(snp->exp)) == 0 ) 
        error(ERR_EXPREXPECT); 
    needpunc( semicolon );
    return snp; 
} 
  
Statement *ParseWhileStatement() 
{       
	Statement *snp;

	currentFn->UsesPredicate = TRUE;
    snp = NewStatement(st_while, TRUE);
	snp->predreg = iflevel;
	iflevel++;
    if( lastst != openpa ) 
        error(ERR_EXPREXPECT); 
    else { 
        NextToken(); 
        if( expression(&(snp->exp)) == 0 ) 
            error(ERR_EXPREXPECT); 
        needpunc( closepa ); 
		if (lastst==kw_do)
			NextToken();
        snp->s1 = ParseStatement(); 
		// Empty statements return NULL
		if (snp->s1)
			snp->s1->outer = snp;
    } 
	iflevel--;
    return snp; 
} 
  
Statement *ParseUntilStatement()
{
	Statement *snp; 

	currentFn->UsesPredicate = TRUE;
    snp = NewStatement(st_until, TRUE);
	snp->predreg = iflevel;
	iflevel++;
    if( lastst != openpa ) 
        error(ERR_EXPREXPECT); 
    else { 
        NextToken(); 
        if( expression(&(snp->exp)) == 0 ) 
            error(ERR_EXPREXPECT); 
        needpunc( closepa ); 
        snp->s1 = ParseStatement(); 
		// Empty statements return NULL
		if (snp->s1)
			snp->s1->outer = snp;
   } 
	iflevel--;
    return snp; 
} 
  
Statement *ParseDoStatement() 
{       
	Statement *snp; 

	currentFn->UsesPredicate = TRUE;
	snp = NewStatement(st_do, TRUE); 
	snp->predreg = iflevel;
	iflevel++;
    snp->s1 = ParseStatement(); 
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
	if (lastst == kw_until)
		snp->stype = st_dountil;
	else if (lastst== kw_loop)
		snp->stype = st_doloop;
    if( lastst != kw_while && lastst != kw_until && lastst != kw_loop ) 
        error(ERR_WHILEXPECT); 
    else { 
        NextToken(); 
		if (snp->stype!=st_doloop) {
        if( expression(&(snp->exp)) == 0 ) 
            error(ERR_EXPREXPECT); 
		}
        if( lastst != end )
            needpunc( semicolon );
    } 
	iflevel--;
    return snp; 
} 
  
Statement *ParseForStatement() 
{
	Statement *snp; 

	currentFn->UsesPredicate = TRUE;
	snp = NewStatement(st_for, TRUE);
	snp->predreg = iflevel;
	iflevel++;
    needpunc(openpa); 
    if( expression(&(snp->initExpr)) == NULL ) 
        snp->initExpr = (ENODE *)NULL; 
    needpunc(semicolon); 
    if( expression(&(snp->exp)) == NULL ) 
        snp->exp = (ENODE *)NULL; 
    needpunc(semicolon); 
    if( expression(&(snp->incrExpr)) == NULL ) 
        snp->incrExpr = (ENODE *)NULL; 
    needpunc(closepa); 
    snp->s1 = ParseStatement(); 
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
	iflevel--;
    return snp; 
} 
  
Statement *ParseForeverStatement() 
{
	Statement *snp; 
    snp = NewStatement(st_forever, TRUE);
    snp->stype = st_forever; 
    snp->s1 = ParseStatement(); 
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
    return snp; 
} 

struct snode *ParseCriticalStatement()
{
	struct snode    *snp; 
	SYM *sp;
    snp = (struct snode*)xalloc(sizeof(struct snode)); 
	snp->outer = currentStmt;
    NextToken(); 
	if (lastst==openpa)
		NextToken();
    if( lastst != id ) { 
        error(ERR_IDEXPECT); 
        return 0; 
    }
    if( (sp = search(lastid,&gsyms[0])) == NULL ) { 
            error( ERR_UNDEFINED ); 
			return 0;
            }
	NextToken();
	if (lastst==closepa)
		NextToken();
    snp->stype = st_critical; 
    snp->label = (int64_t *)sp->name; 
    snp->next = 0; 
	snp->s1 = ParseStatement();
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
	return snp;
}
  
Statement *ParseSpinlockStatement()
{
	Statement *snp; 
	SYM *sp;

	snp = NewStatement(st_spinlock, TRUE); 
	if (lastst==openpa)
		NextToken();
    if( NonCommaExpression(&(snp->exp)) == 0 ) 
        error(ERR_EXPREXPECT); 
	snp->incrExpr = (ENODE *)1;
	snp->initExpr = (ENODE *)0;
	//if( lastst != id ) { 
 //       error(ERR_IDEXPECT); 
 //       return 0; 
 //   }
 //   if( (sp = search(lastid,&gsyms[0])) == NULL ) { 
 //           error( ERR_UNDEFINED ); 
	//		return 0;
 //           }
//	NextToken();
//	if (lastst==comma) {
//		NextToken();
//		snp->incrExpr = (ENODE *)GetIntegerExpression((ENODE **)NULL);
//		if ((int64_t)snp->incrExpr < 1 || (int64_t)snp->incrExpr > 15)
//			error(ERR_SEMA_INCR);
//		snp->incrExpr = (ENODE *)((int64_t)snp->incrExpr);
//	}
	if (lastst==comma) {
		NextToken();
		snp->initExpr = (ENODE *)GetIntegerExpression((ENODE **)NULL);
	}
	if (lastst==closepa)
		NextToken();
//    snp->label = sp->name; 
    snp->next = 0; 
	snp->s1 = ParseStatement();
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
	if (lastst==kw_lockfail) {
		NextToken();
		snp->s2 = ParseStatement();
		// Empty statements return NULL
		if (snp->s2)
			snp->s2->outer = snp;
	}
	return snp;
}
  
Statement *ParseSpinunlockStatement()
{
	Statement *snp; 
	SYM *sp;

    snp = NewStatement(st_spinunlock, TRUE); 
	snp->incrExpr = (ENODE *)1;
	if (lastst==openpa)
		NextToken();
    if( expression(&(snp->exp)) == 0 ) 
        error(ERR_EXPREXPECT); 
	if (lastst==comma) {
		NextToken();
		snp->incrExpr = (ENODE *)GetIntegerExpression((ENODE **)NULL);
		if ((int64_t)snp->incrExpr < 1 || (int64_t)snp->incrExpr > 15)
			error(ERR_SEMA_INCR);
		snp->incrExpr = (ENODE *)((int64_t)snp->incrExpr & 15);
	}
    //if( lastst != id ) { 
    //    error(ERR_IDEXPECT); 
    //    return 0; 
    //}
  //  if( (sp = search(lastid,&gsyms[0])) == NULL ) { 
  //      error( ERR_UNDEFINED ); 
		//return 0;
  //  }
	NextToken();
	if (lastst==closepa)
		NextToken();
    //snp->label = sp->name; 
    snp->next = 0; 
	return snp;
}
  
Statement *ParseFirstcallStatement() 
{
	Statement *snp; 
	SYM *sp;
	int st;

    snp = NewStatement(st_firstcall, TRUE); 
    sp = allocSYM();
	sp->name = snp->fcname;
    sp->storage_class = sc_static;
    sp->value.i = nextlabel++;
    sp->tp = &stdbyte;
    st = lastst;
    lastst = kw_firstcall;       // fake out doinit()
    doinit(sp);
    lastst = st;
    snp->fcname = litlate(sp->realname);
    snp->s1 = ParseStatement(); 
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
    return snp; 
} 
  
Statement *ParseIfStatement() 
{
	Statement *snp; 

	NextToken();
	if (lastst == kw_firstcall)
	   return ParseFirstcallStatement();
	currentFn->UsesPredicate = TRUE;
    snp = NewStatement(st_if, FALSE);
	snp->predreg = iflevel;
	iflevel++;
    if( lastst != openpa ) 
        error(ERR_EXPREXPECT); 
    else {
        NextToken(); 
        if( expression(&(snp->exp)) == 0 ) 
            error(ERR_EXPREXPECT); 
        needpunc( closepa ); 
		if (lastst==kw_then)
			NextToken();
        snp->s1 = ParseStatement(); 
		if (snp->s1)
			snp->s1->outer = snp;
        if( lastst == kw_else ) { 
            NextToken(); 
            snp->s2 = ParseStatement(); 
			if (snp->s2)
				snp->s2->outer = snp;
        } 
		else if (lastst == kw_elsif) {
            snp->s2 = ParseIfStatement(); 
			if (snp->s2)
				snp->s2->outer = snp;
		}
        else 
            snp->s2 = 0; 
    } 
	iflevel--;
    return snp; 
} 

Statement *ParseCatchStatement()
{
	Statement *snp;
	SYM *sp;
	TYP *tp,*tp1,*tp2;
    ENODE *node;
    static char buf[200];
    
	snp = NewStatement(st_catch, TRUE);
	currentStmt = snp;
	if (lastst != openpa) {
		snp->label = (int64_t *)NULL;
		snp->s2 = (Statement *)99999;
		snp->s1 = ParseStatement();
		// Empty statements return NULL
		if (snp->s1)
			snp->s1->outer = snp;
		return snp;
	}
    needpunc(openpa);
	tp = head;
	tp1 = tail;
	catchdecl = TRUE;
	ParseAutoDeclarations(&snp->ssyms);
	cseg();
	catchdecl = FALSE;
	tp2 = head;
	head = tp;
	tail = tp1;
    needpunc(closepa);
    
	if( (sp = search(declid,&snp->ssyms)) == NULL)
        sp = makeint(declid);
    node = makenode(sp->storage_class==sc_static ? en_labcon : en_autocon,NULL,NULL);
    // nameref looks up the symbol using lastid, so we need to back it up and
    // restore it.
    strcpy(buf,lastid);
    strcpy(lastid, declid);
    nameref(&node);
    strcpy(lastid,buf);
	snp->s1 = ParseStatement();
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
	snp->label = (int64_t *)node;	// save name reference
	if (sp->tp->typeno >= bt_last)
		error(ERR_CATCHSTRUCT);
	snp->s2 = (Statement *)GetTypeHash(sp->tp);
	// Empty statements return NULL
//	if (snp->s2)
//		snp->s2->outer = snp;
	return snp;
}

Statement *ParseCaseStatement()
{
	Statement *snp; 
    Statement *head, *tail;
	int buf[256];
	int nn;
	int *bf;

    snp = NewStatement(st_case, FALSE); 
	if (lastst == kw_fallthru)	// ignore "fallthru"
		NextToken();
    if( lastst == kw_case ) { 
        NextToken(); 
        snp->s2 = 0;
		nn = 0;
		do {
			buf[nn] = GetIntegerExpression((ENODE **)NULL);
			nn++;
			if (lastst != comma)
				break;
			NextToken();
		} while (nn < 256);
		if (nn==256)
			error(ERR_TOOMANYCASECONSTANTS);
		bf = (int *)xalloc(sizeof(int)*(nn+1));
		bf[0] = nn;
		for (; nn > 0; nn--)
			bf[nn]=buf[nn-1];
		snp->label = (int64_t *)bf;
    }
    else if( lastst == kw_default) { 
        NextToken(); 
        snp->s2 = (Statement *)1; 
		snp->stype = st_default;
    } 
    else { 
        error(ERR_NOCASE); 
        return (Statement *)NULL;
    } 
    needpunc(colon); 
    head = (Statement *)NULL; 
    while( lastst != end && lastst != kw_case && lastst != kw_default ) { 
		if( head == NULL ) {
			head = tail = ParseStatement(); 
			if (head)
				head->outer = snp;
		}
		else { 
			tail->next = ParseStatement(); 
			if( tail->next != NULL )  {
				tail->next->outer = snp;
				tail = tail->next;
			}
		}
        tail->next = 0; 
    } 
    snp->s1 = head; 
    return snp; 
} 
  
int CheckForDuplicateCases(Statement *head) 
{     
	Statement *top, *cur;

	cur = top = head;
	while( top != (Statement *)NULL )
	{
		cur = top->next;
		while( cur != (Statement *)NULL )
		{
			if( (!(cur->s1 || cur->s2) && cur->label == top->label)
				|| (cur->s2 && top->s2) )
			{
				printf(" duplicate case value %d\n",cur->label);
				return TRUE;
			}
			cur = cur->next;
		}
		top = top->next;
	}
	return FALSE;
} 
  
Statement *ParseSwitchStatement() 
{       
	Statement *snp; 
    Statement *head, *tail; 

	currentFn->UsesPredicate = TRUE;
    snp = NewStatement(st_switch, TRUE);
	snp->predreg = iflevel;
	iflevel++;
    if( expression(&(snp->exp)) == NULL ) 
        error(ERR_EXPREXPECT); 
    needpunc(begin); 
    head = 0; 
    while( lastst != end ) { 
		if( head == (Statement *)NULL ) {
			head = tail = ParseCaseStatement(); 
			if (head)
				head->outer = snp;
		}
		else { 
			tail->next = ParseCaseStatement(); 
			if( tail->next != (Statement *)NULL ) {
				tail->next->outer = snp;
				tail = tail->next;
			}
		}
		if (tail==(Statement *)NULL) break;	// end of file in switch
        tail->next = (Statement *)NULL; 
    } 
    snp->s1 = head; 
    NextToken(); 
    if( CheckForDuplicateCases(head) ) 
        error(ERR_DUPCASE); 
	iflevel--;
    return snp; 
} 
  
Statement *ParseReturnStatement() 
{       
	Statement *snp;

	snp = NewStatement(st_return, TRUE);
    expression(&(snp->exp));
    if( lastst != end )
        needpunc( semicolon );
    return snp; 
} 
  
Statement *ParseThrowStatement() 
{  
	Statement *snp;
	TYP *tp;

	currentFn->DoesThrow = TRUE;
	snp = NewStatement(st_throw, TRUE);
    tp = expression(&(snp->exp));
	snp->label = (int64_t *)GetTypeHash(tp);
    if( lastst != end )
        needpunc( semicolon );
    return snp;
} 
  
Statement *ParseBreakStatement() 
{     
	Statement *snp; 

	snp = NewStatement(st_break, TRUE);
    if( lastst != end )
        needpunc( semicolon );
    return snp; 
} 
  
Statement *ParseContinueStatement() 
{
	Statement *snp; 

    snp = NewStatement(st_continue, TRUE);
    if( lastst != end )
        needpunc( semicolon );
    return snp;
} 
  
Statement *ParseIntoffStatement() 
{       
	Statement *snp; 
    snp = NewStatement(st_intoff, TRUE); 
    if( lastst != end )
        needpunc( semicolon );
    return snp;
} 
  
Statement *ParseIntonStatement() 
{       
	Statement *snp; 

    snp = NewStatement(st_inton, TRUE); 
    if( lastst != end )
        needpunc( semicolon );
    return snp;
} 
  
Statement *ParseStopStatement() 
{
	Statement *snp; 

	snp = NewStatement(st_stop, TRUE); 
	if( lastst != end )
		needpunc( semicolon );
	return snp;
} 
  
Statement *ParseAsmStatement() 
{
	static char buf[3501];
	int nn;

	Statement *snp; 
    snp = NewStatement(st_asm, FALSE); 
    while( my_isspace(lastch) )
		getch(); 
    NextToken();
	if (lastst != begin)
		error(ERR_PUNCT);
	nn = 0;
	do {
		getch();
		if (lastch=='}')
			break;
		buf[nn++] = lastch;
	}
	while(lastch!=-1 && nn < 3500);
	if (nn >= 3500)
		error(ERR_ASMTOOLONG);
	buf[nn] = '\0';
	snp->label = (int64_t *)litlate(buf);
    return snp;
} 

Statement *ParseTryStatement()
{
	Statement *snp;
	TYP *tp,*tp1,*tp2;
	SYM *sp;
	Statement *hd, *tl;

	hd = (Statement *)NULL;
	tl = (Statement *)NULL;
	snp = NewStatement(st_try, TRUE);
    snp->s1 = ParseStatement();
	// Empty statements return NULL
	if (snp->s1)
		snp->s1->outer = snp;
	if (lastst != kw_catch)
        error(ERR_CATCHEXPECT);
    while( lastst == kw_catch ) {
		if( hd == NULL ) {
			hd = tl = ParseCatchStatement(); 
			if (hd)
				hd->outer = snp;
		}
		else { 
			tl->next = ParseCatchStatement(); 
			if( tl->next != NULL ) {
				tl->next->outer = snp;
				tl = tl->next;
			}
		}
		if (tl==(Statement *)NULL) break;	// end of file in try
        tl->next = (Statement *)NULL; 
    } 
    snp->s2 = hd;
    return snp;
} 
  
Statement *ParseExpressionStatement() 
{       
	Statement *snp;
    snp = NewStatement(st_expr, FALSE); 
    if( expression(&(snp->exp)) == NULL ) { 
        error(ERR_EXPREXPECT); 
        NextToken(); 
    } 
    if( lastst != end )
        needpunc( semicolon );
    return snp; 
} 

// Parse a compound statement.

Statement *ParseCompoundStatement()
{  
	Statement *snp;
	Statement *head, *tail; 

    snp = NewStatement(st_compound, FALSE); 
	currentStmt = snp;
	head = 0;
	if (lastst==colon) {
		NextToken();
		TRACE(printf("Compound <%s>\r\n",lastid);)
		if (strcmp(lastid,"clockbug")==0)
			printf("clockbug\r\n");
		NextToken();
	}
	ParseAutoDeclarations(&snp->ssyms);
	cseg();
	// Add the first statement at the head of the list.
	if (lastst != end) {
		head = tail = ParseStatement(); 
		if (head)
			head->outer = snp;
	}
	//else {
 //       head = tail = NewStatement(st_empty,1);
	//	if (head)
	//		head->outer = snp;
	//}
	// Add remaining statements onto the tail of the list.
	while( lastst != end) {
		tail->next = ParseStatement(); 
		if( tail->next != NULL ) {
			tail->next->outer = snp;
			tail = tail->next;
		}
	}
    NextToken();
	snp->s1 = head;
    return snp;
} 
  
Statement *ParseLabelStatement()
{      
	Statement *snp;
    SYM *sp;

    snp = NewStatement(st_label, FALSE); 
    if( (sp = search(lastid,&lsyms)) == NULL ) { 
        sp = allocSYM(); 
        sp->name = litlate(lastid); 
        sp->storage_class = sc_label; 
        sp->tp = 0; 
        sp->value.i = nextlabel++; 
        insert(sp,&lsyms); 
    } 
    else { 
        if( sp->storage_class != sc_ulabel ) 
            error(ERR_LABEL); 
        else 
            sp->storage_class = sc_label; 
    } 
    NextToken();       /* get past id */ 
    needpunc(colon); 
    if( sp->storage_class == sc_label ) { 
        snp->label = (int64_t *)sp->value.i; 
        snp->next = (Statement *)NULL; 
        return snp; 
    } 
    return 0; 
} 
  
Statement *ParseGotoStatement() 
{       
	Statement *snp; 
    SYM *sp;

    NextToken(); 
    if( lastst != id ) { 
        error(ERR_IDEXPECT); 
        return (Statement *)NULL;
    } 
    snp = NewStatement(st_goto, FALSE);
    if( (sp = search(lastid,&lsyms)) == NULL ) { 
        sp = allocSYM(); 
        sp->name = litlate(lastid); 
        sp->value.i = nextlabel++; 
        sp->storage_class = sc_ulabel; 
        sp->tp = 0; 
        insert(sp,&lsyms); 
    }
    NextToken();       /* get past label name */
    if( lastst != end )
        needpunc( semicolon );
    if( sp->storage_class != sc_label && sp->storage_class != sc_ulabel)
        error( ERR_LABEL );
    else { 
        snp->stype = st_goto;
        snp->label = (int64_t *)sp->value.i;
        snp->next = (Statement *)NULL;
        return snp; 
    } 
    return (Statement *)NULL;
} 
  
Statement *ParseStatement() 
{
	Statement *snp; 
    switch( lastst ) { 
    case semicolon: 
        snp = NewStatement(st_empty,1);
        break; 
    case begin: 
		NextToken(); 
        snp = ParseCompoundStatement();
        return snp; 
    case kw_check:
         snp = ParseCheckStatement();
         break;
    case kw_prolog:
         snp = NewStatement(st_empty,1);
         currentFn->prolog = ParseStatement(); break;
    case kw_epilog:
         snp = NewStatement(st_empty,1);
         currentFn->epilog = ParseStatement(); break;
    case kw_if: snp = ParseIfStatement(); break; 
    case kw_while: snp = ParseWhileStatement(); break; 
    case kw_until: snp = ParseUntilStatement(); break; 
    case kw_for:   snp = ParseForStatement();   break; 
    case kw_forever: snp = ParseForeverStatement(); break; 
    case kw_firstcall: snp = ParseFirstcallStatement(); break; 
    case kw_return: snp = ParseReturnStatement(); break; 
    case kw_break: snp = ParseBreakStatement(); break; 
    case kw_goto: snp = ParseGotoStatement(); break; 
    case kw_continue: snp = ParseContinueStatement(); break; 
    case kw_do:
	case kw_loop: snp = ParseDoStatement(); break; 
    case kw_switch: snp = ParseSwitchStatement(); break;
	case kw_try: snp = ParseTryStatement(); break;
	case kw_throw: snp = ParseThrowStatement(); break;
	case kw_vortex:
			snp = vortex_stmt();
			break;
	case kw_intoff: snp = ParseIntoffStatement(); break;
	case kw_inton: snp = ParseIntonStatement(); break;
	case kw_stop: snp = ParseStopStatement(); break;
	case kw_asm:
		snp = ParseAsmStatement(); break;
	case kw_critical: snp = ParseCriticalStatement(); break;
	case kw_spinlock: snp = ParseSpinlockStatement(); break;
	case kw_spinunlock: snp = ParseSpinunlockStatement(); break;
    case id:
			SkipSpaces();
            if( lastch == ':' ) 
                return ParseLabelStatement(); 
            // else fall through to parse expression
    default: 
            snp = ParseExpressionStatement(); 
            break; 
    } 
	if( snp != NULL ) {
        snp->next = (Statement *)NULL;
	}
    return snp; 
} 
