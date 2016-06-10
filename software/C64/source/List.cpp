// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2016  Robert Finch, Stratford
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@finitron.ca
//       ||
//
// C64 - 'C' derived language compiler
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
#include "stdafx.h"

/*
 *	68000 C compiler
 *
 *	Copyright 1984, 1985, 1986 Matthew Brandt.
 *  all commercial rights reserved.
 *
 *	This compiler is intended as an instructive tool for personal use. Any
 *	use for profit without the written consent of the author is prohibited.
 *
 *	This compiler may be distributed freely for non-commercial use as long
 *	as this notice stays intact. Please forward any enhancements or questions
 *	to:
 *
 *		Matthew Brandt
 *		Box 920337
 *		Norcross, Ga 30092
 */

void ListTable(TABLE *t, int i);

void put_typedef(int td)
{
	lfs.printf(td ? (char *)"   1   " : (char *)"   -    ");
}

void put_sc(int scl)
{       switch(scl) {
                case sc_static:
                        lfs.printf("Static      ");
                        break;
                case sc_thread:
                        lfs.printf("Thread      ");
                        break;
                case sc_auto:
                        lfs.printf("Auto        ");
                        break;
                case sc_global:
                        lfs.printf("Global      ");
                        break;
                case sc_external:
                        lfs.printf("External    ");
                        break;
                case sc_type:
                        lfs.printf("Type        ");
                        break;
                case sc_const:
                        lfs.printf("Constant    ");
                        break;
                case sc_member:
                        lfs.printf("Member      ");
                        break;
                case sc_label:
                        lfs.printf("Label");
                        break;
                case sc_ulabel:
                        lfs.printf("Undefined label");
                        break;
                }
}

void put_ty(TYP *tp)
{
		if(tp == 0)
                return;
        switch(tp->type) {
                case bt_exception:
                        lfs.printf("Exception");
                        break;
				case bt_byte:
                        lfs.printf("Byte");
                        break;
				case bt_ubyte:
                        lfs.printf("Unsigned Byte");
                        break;
                case bt_char:
                        lfs.printf("Char");
                        break;
                case bt_short:
                        lfs.printf("Short");
                        break;
                case bt_enum:
                        lfs.printf("enum ");
                        goto ucont;
                case bt_long:
                        lfs.printf("Long");
                        break;
                case bt_unsigned:
                        lfs.printf("unsigned long");
                        break;
                case bt_float:
                        lfs.printf("Float");
                        break;
                case bt_double:
                        lfs.printf("Double");
                        break;
                case bt_pointer:
                        if( tp->val_flag == 0)
                                lfs.printf("Pointer to ");
                        else
                                lfs.printf("Array of ");
                        put_ty(tp->GetBtp());
                        break;
                case bt_class:
                        lfs.printf("class ");
                        goto ucont;
                case bt_union:
                        lfs.printf("union ");
                        goto ucont;
                case bt_struct:
                        lfs.printf("struct ");
ucont:                  if(tp->sname->length() == 0)
                                lfs.printf("<no name> ");
                        else
                                lfs.printf("%s ",(char *)tp->sname->c_str());
                        break;
                case bt_ifunc:
                case bt_func:
                        lfs.printf("Function returning ");
                        put_ty(tp->GetBtp());
                        break;
                }
}

void list_var(SYM *sp, int i)
{
	TypeArray *ta;
	int nn;
		int     j;
        for(j = i; j; --j)
                lfs.printf("    ");
		if (sp->name->length()== 0)
			lfs.printf("%-10s =%06x ","<unnamed>",sp->value.u);
		else
			lfs.printf("%-10s =%06x ",(char *)sp->name->c_str(),sp->value.u);
//			if (sp->IsPascal) ofs.printf("\tpascal ");
        if( sp->storage_class == sc_external)
                ofs.printf("\textern\t%s\n",(char *)sp->name->c_str());
        else if( sp->storage_class == sc_global )
                ofs.printf(";\tglobal\t%s\n",(char *)sp->name->c_str());
		put_typedef(sp->storage_class==sc_typedef);
        put_sc(sp->storage_class);
        put_ty(sp->tp);
        lfs.printf("\n");
        if(sp->tp == 0)
                return;
    if (sp->tp) {
  		if (sp->tp->type==bt_ifunc || sp->tp->type==bt_func) {
  			lfs.printf("\t\tParameters:\n\t\t\t");
  			ta = sp->GetProtoTypes();
  			ta->Print(&lfs);
  			if (ta)
  				delete ta;
  		}
	  }
	  if (sp->tp) {
        if((sp->tp->type == bt_struct || sp->tp->type == bt_union || sp->tp->type==bt_class) &&
                sp->storage_class == sc_type)
                ListTable(&(sp->tp->lst),i+1);
    }
}

void ListTable(TABLE *t, int i)
{
	SYM *sp;
	int nn;

	if (t==&gsyms[0]) {
		for (nn = 0; nn < 257; nn++) {
			t = &gsyms[nn];
			sp = SYM::GetPtr(t->GetHead());
			while(sp != NULL) {
				list_var(sp,i);
				sp = sp->GetNextPtr();
      }
		}
	}
	else {
		sp = SYM::GetPtr(t->GetHead());
		while(sp != NULL) {
			list_var(sp,i);
			sp = sp->GetNextPtr();
    }
	}
}


// Recursively list the vars contained in compound statements.

void ListCompound(Statement *stmt)
{
	Statement *s1;

	ListTable(&stmt->ssyms,0);
	for (s1 = stmt->s1; s1; s1 = s1->next) {
		if (s1->stype == st_compound)
			ListCompound(s1);
		if (s1->s1) {
			if (s1->s1->stype==st_compound)
				ListCompound(s1->s1);
		}
		if (s1->s2) {
			if (s1->s2->stype==st_compound)
				ListCompound(s1->s2);
		}
	}
}
