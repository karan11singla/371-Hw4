#include <stdio.h>
#include "bstr.h"
#include "comp.h"
#include <stdlib.h>

void COMP_Init(Computer *cmp) {
    int r, m;
    BSTR_SetValue(&(cmp->pc),0,16);
    BSTR_SetValue(&(cmp->ir),0,16);
    BSTR_SetValue(&(cmp->cc),0,3);
    for (r = 0; r < 8; r++) {
        BSTR_SetValue(&(cmp->reg[r]),r,16);  /* put some interesting data in registers */
    }
    for (m = 0; m < MAXMEM; m++) {
        BSTR_SetValue(&(cmp->mem[m]),0,16);
    }
}

void COMP_LoadWord(Computer* comp, int addr, BitString word) {
    comp->mem[addr] = word;
}


// TODO - Missing Piece: The Condition Code is not set.
void COMP_ExecuteNot(Computer *comp) {
    BitString drBS, srBS;
    BSTR_Substring(&drBS,comp->ir,4,3);
    BSTR_Substring(&srBS,comp->ir,7,3);
    comp->reg[ BSTR_GetValue(drBS) ] = comp->reg[ BSTR_GetValue(srBS) ];
    BSTR_Invert( & comp->reg[ BSTR_GetValue(drBS)  ]  );
    
    int condition = BSTR_GetValueTwosComp(comp->reg[BSTR_GetValue(drBS)]);
    /* Setting condition code */
    if (condition > 0) {
        BSTR_SetValue(&(comp->cc),1,3);
    } else if (condition < 0) {
        BSTR_SetValue(&(comp->cc),4, 3);
    } else {
        BSTR_SetValue(&(comp->cc),2, 3);
    }
}
// Load instruction
void COMP_ExecuteLoad(Computer *comp) {
    BitString drBS, pcOffset;
    int offset;
    
    BSTR_Substring(&drBS,comp->ir,4,3);
    BSTR_Substring(&pcOffset,comp->ir,7,9);
    offset = BSTR_GetValueTwosComp(pcOffset) + BSTR_GetValue(comp->pc);
    
    //Grab the value at the mem address at the offset and load to the register
    BSTR_SetValue(&(comp->reg[BSTR_GetValue(drBS)]), BSTR_GetValueTwosComp(comp->mem[offset]), 16);
    
    int condition = BSTR_GetValueTwosComp(comp->reg[BSTR_GetValue(drBS)]);
    /* Setting condition code */
    if (condition > 0) {
        BSTR_SetValue(&(comp->cc),1,3);
    } else if (condition < 0) {
        BSTR_SetValue(&(comp->cc),4, 3);
    } else {
        BSTR_SetValue(&(comp->cc),2, 3);
    }
}


// Branch instruction, not fully working
void COMP_ExecuteBranch(Computer *comp) {
    BitString condCode, nzpCode, pcOffset;
    int offset, nzpInt;
    
    BSTR_Substring(&pcOffset,comp->ir,7,9);
    //Supposed to GetValueTwosComp, but produces weird result on console
    offset = BSTR_GetValueTwosComp(pcOffset) + BSTR_GetValue(comp->pc);
    
    BSTR_Substring(&condCode,comp->cc,0,3);
    int cc = BSTR_GetValue(condCode);
    
    BSTR_Substring(&nzpCode,comp->ir,4,3);
    
    nzpInt = BSTR_GetValue(nzpCode);
    
    if ((cc & nzpInt)) {
        BSTR_SetValue(& comp->pc,offset,16);
    }
}
// Trap instruction with 2 options, one for Halt and one for Out
void COMP_ExecuteTrap(Computer *comp, int* haltFlag) {
    BitString trapVect;
    int trapInt;
    
    BSTR_Substring(&trapVect,comp->ir,8,8);
    
    trapInt = BSTR_GetValue(trapVect);
    if (trapInt == 37) {    //if the trap vector is x25, then halt program
        *haltFlag = 0;   // 0 for halt the program; 1 continues the while loop
    } else if (trapInt == 33) { //if the trap vector is x21, then out
        printf("%c", BSTR_GetValue(comp->reg[0]));
    }
}
//Add instruction
void COMP_ExecuteAdd(Computer *comp) {
    
    BitString immCode;
    int immCodeInt;
    
    BSTR_Substring(&immCode,comp->ir,10,1);  /* isolate imm code */
    immCodeInt = BSTR_GetValue(immCode); /* get its value */
    
    BitString drBS, srBS1, srBS2, srIMM;
    BSTR_Substring(&drBS,comp->ir,4,3);
    BSTR_Substring(&srBS1,comp->ir,7,3);
    
    int firstSR = BSTR_GetValueTwosComp(comp->reg[BSTR_GetValue(srBS1)]);
    
    if (immCodeInt == 0) {  // if immediate code is 0,
        BSTR_Substring(&srBS2,comp->ir,13,3);
        
        int secondSR =  BSTR_GetValueTwosComp(comp->reg[BSTR_GetValue(srBS2)]);
        
        BSTR_SetValueTwosComp(&comp->reg[BSTR_GetValue(drBS)], firstSR + secondSR ,16);
    } else {    // if immediate code is not 0, thus, 1
        BSTR_Substring(&srIMM,comp->ir,11,5);
        
        int immValue = BSTR_GetValueTwosComp(srIMM);
        BSTR_SetValueTwosComp(&comp->reg[BSTR_GetValue(drBS)], firstSR + immValue ,16);
        
    }
    int condition = BSTR_GetValueTwosComp(comp->reg[BSTR_GetValue(drBS)]);
    /* Setting condition code */
    if (condition > 0) {
        BSTR_SetValue(&(comp->cc),1,3);
    } else if (condition < 0) {
        BSTR_SetValue(&(comp->cc),4, 3);
    } else {
        BSTR_SetValue(&(comp->cc),2, 3);
    }
}

void COMP_Execute(Computer* comp) {
    BitString opCode;
    int opCodeInt, haltFlag = 1;
    int* ptr;
    ptr = &haltFlag;
    
    /* use the PC to load current instruction from memory into IR */
    // comp->ir = comp->mem[BSTR_GetValue(comp->pc)];
    
    while (haltFlag && BSTR_GetValue(comp->pc) < 50) {
        /* use the PC to load current instruction from memory into IR */
        
        comp->ir = comp->mem[BSTR_GetValue(comp->pc)];
        
        BSTR_AddOne(&comp->pc);
        BSTR_Substring(&opCode,comp->ir,0,4);  /* isolate op code */
        opCodeInt = BSTR_GetValue(opCode); /* get its value */
        
        /*what kind of instruction is this? */
        if (opCodeInt == 9) {   // NOT
            COMP_ExecuteNot(comp);
        } else if (opCodeInt == 1) {    //ADD DR, SR1, SR2
            COMP_ExecuteAdd(comp);
        } else if (opCodeInt == 2) {
            COMP_ExecuteLoad(comp);
        } else if (opCodeInt == 0) {
            COMP_ExecuteBranch(comp);
        } else if (opCodeInt == 15) {
            COMP_ExecuteTrap(comp, ptr);
        }
    } //end while(1)
}

void COMP_Display(Computer cmp) {
    int r, m;
    printf("\n");
    
    printf("PC ");
    BSTR_Display(cmp.pc,1);
    printf("   ");
    
    
    printf("IR ");
    BSTR_Display(cmp.ir,1);
    printf("   ");
    
    
    printf("CC ");
    BSTR_Display(cmp.cc,1);
    printf("\n");
    
    
    for (r = 0; r < 8; r++) {
        printf("R%d ",r);
        BSTR_Display(cmp.reg[r], 1);
        if (r % 3 == 2)
            printf("\n");
        else
            printf("   ");
    }
    printf("\n");
    for (m = 0; m < MAXMEM; m++) {
        printf("%3d ",m);
        BSTR_Display(cmp.mem[m], 1);
        
        if (m % 3 == 2)
            printf("\n");
        else
            printf("    ");
    }
    printf("\n");
}

