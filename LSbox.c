//
//  LSbox.c
//  L-S-model
//
//  Created by benny on 16/1/5.
//
//

#include "LSbox.h"

/*=========================================================*/
/*   MARK: Declarition     */
/*=========================================================*/

/* round_constant , key_round, matrix L*/
static
Mat  *rdConst[ROUNDS],  *key_r, *matL;

#if MASK

#if DIVIDE_SLICES != 1
static
//Mat  *rdCLeft[ROUNDS], *rdCRight[ROUNDS], *Lleft, *Lright, *Tleft, *Tright, **keyRoundLR;
Mat **keyRoundSlices, **rdConstSlices[ROUNDS], **LSlices, *TSlices[DIVIDE_SLICES];
#endif

#if DIM_A != 0
extern
Mat *matA, *matInvA, *matTransA;
static
Mat *matT;
#endif

#endif /* MASK */

BYTE matLV[L_SIZE] = MAT_LV;
BYTE keyRV[KEY_SIZE] = KEY_RV;

BYTE rdConst1V[CONST_SIZE] = CONSTR1;
BYTE rdConst2V[CONST_SIZE] = CONSTR2;
BYTE rdConst3V[CONST_SIZE] = CONSTR3;
BYTE rdConst4V[CONST_SIZE] = CONSTR4;
BYTE rdConst5V[CONST_SIZE] = CONSTR5;
BYTE rdConst6V[CONST_SIZE] = CONSTR6;
BYTE rdConst7V[CONST_SIZE] = CONSTR7;
BYTE rdConst8V[CONST_SIZE] = CONSTR8;

BYTE rdConst9V[CONST_SIZE] = CONSTR9;
BYTE rdConst10V[CONST_SIZE] = CONSTR10;
BYTE rdConst11V[CONST_SIZE] = CONSTR11;
BYTE rdConst12V[CONST_SIZE] = CONSTR12;
BYTE rdConst13V[CONST_SIZE] = CONSTR13;
BYTE rdConst14V[CONST_SIZE] = CONSTR14;
BYTE rdConst15V[CONST_SIZE] = CONSTR15;
BYTE rdConst16V[CONST_SIZE] = CONSTR16;

/*=========================================================*/
/*   MARK: Private   Functions      */
/*=========================================================*/


#if MASK
/* A 4-bit sbox (masked)*/
static
Mat **sbox4b(
             const Mat **mats4b
             )
{
    if (mats4b[0]->dim_row != 4) return NULL;

    Mat **matsTem[MASKD];
    Mat *rvectWithMask[4][MASKD], **imdWithMask[4];

    int indexOfMask;
    for (indexOfMask = 0; indexOfMask < MASKD; ++indexOfMask){
        matsTem[indexOfMask] = split(mats4b[indexOfMask], 4, 1);
        int i;
        for (i = 0; i < 4; ++i){
            rvectWithMask[i][indexOfMask] = matsTem[indexOfMask][i];
        }
    }

    // a = 1 x 2 + 0
    // b = 2 x a + 3
    // c = 1 x b + 2
    // d = b x c + 1
    Mat **product;

    product = bitAndWithMask(rvectWithMask[1], rvectWithMask[2]);
    imdWithMask[0] = addWithMask(product, rvectWithMask[0]);
    deMats(product, MASKD);

    product = bitAndWithMask(rvectWithMask[2], imdWithMask[0]);
    imdWithMask[1] = addWithMask(product, rvectWithMask[3]);
    deMats(product, MASKD);

    product = bitAndWithMask(rvectWithMask[1], imdWithMask[1]);
    imdWithMask[2] = addWithMask(product, rvectWithMask[2]);
    deMats(product, MASKD);

    product = bitAndWithMask(imdWithMask[1], imdWithMask[2]);
    imdWithMask[3] = addWithMask(product, rvectWithMask[1]);
    deMats(product, MASKD);

    /* Generate the correct order */
    /* d a b c */
    Mat **ordered[] = { imdWithMask[3], imdWithMask[0], imdWithMask[1], imdWithMask[2] };

    Mat **retMat = (Mat **)malloc(MASKD * sizeof(Mat *));
    for (indexOfMask = 0; indexOfMask < MASKD; ++indexOfMask){
        int i;
        for (i = 0; i < 4; ++i){
            deMat(matsTem[indexOfMask][i]);
            matsTem[indexOfMask][i] = ordered[i][indexOfMask];
        }
        retMat[indexOfMask] = cat(matsTem[indexOfMask], 4, 1);
    }


    /* Deallocate all  */
    int i;
    for(i = 0; i < 4; ++i)
    {
        deMats(imdWithMask[i], MASKD);
    }

    return retMat;
}

#else /* Unmask */
/* A 4-bit sbox (unmask) */
static
Mat *sbox4b(
            const Mat *mat4b
            )
{
    if(mat4b->dim_row != 4) return NULL;
    
    Mat **rvect;
    rvect = split(mat4b, 4, 1);
    
    // a = 1 x 2 + 0
    // b = 2 x a + 3
    // c = 1 x b + 2
    // d = b x c + 1
    
    Mat *imd[4];
    Mat *product;
    
    product = bitAnd(rvect[1], rvect[2]);
    imd[0] = add(product, rvect[0]);
    deMat(product);
    
    product = bitAnd(rvect[2], imd[0]);
    imd[1] = add(product, rvect[3]);
    deMat(product);
    
    product = bitAnd(rvect[1], imd[1]);
    imd[2] = add(product, rvect[2]);
    deMat(product);
    
    product = bitAnd(imd[1], imd[2]);
    imd[3] = add(product, rvect[1]);
    deMat(product);


    /* Generate the correct order */
    /* d a b c */
    Mat *ordered[] = {imd[3], imd[0], imd[1], imd[2]};

    Mat *retMat;
    retMat =  cat(ordered, 4, 1);

    /* Deallocate all  */
    int i;
    for(i = 0; i < 4; ++i)
    {
        deMat(imd[i]);
        deMat(rvect[i]);
    }

    return retMat;
}
#endif /* MASK */


/* Using matT, matL */
/* A Lboxes (masked) */
#if MASK
#if DIVIDE_SLICES != 1
static
void lboxes(
            Mat **matsLin,
            int direction /* direction == 0, means the left part */
            )
{
    Mat *matTem = matsLin[0];
#if DIM_A
	matsLin[0] = multiply(matTem, TSlices[direction]);
#else
    matsLin[0] = multiply(matTem, LSlices[direction]);
#endif
	deMat(matTem);
 
	int indexOfMask;
	for (indexOfMask = 1; indexOfMask < MASKD; ++indexOfMask){
		matTem = matsLin[indexOfMask];
		matsLin[indexOfMask] = multiply(matTem, LSlices[direction]);
        deMat(matTem);
    }
}

#else /* DIVIDE_SLICES == 1 */
static
void lboxes(
            Mat **matsLin
            )
{
    /* z[0] = T  *  x[0] */
    Mat *matTem = matsLin[0];
#if DIM_A
    matsLin[0] = multiply(matTem, matT);
#else /* DIM_A == 0 */
    matsLin[0] = multiply(matTem, matL);
#endif
    deMat(matTem);

    int i;
    for(i = 1; i < MASKD; ++i){
        matTem = matsLin[i];
        matsLin[i] = multiply(matTem,matL);
        deMat(matTem);
    }
}
#endif /* DIVIDE_SLICES */

#else /* Unmask  */
/* DIM_L-bit L-box (unmask) */
static
Mat *lboxes(
            const Mat *lin
            )
{
    Mat *retMat;
    retMat = multiply(lin, matL);
    return retMat;
}
#endif /* MASK */


#if MASK
/* DIM_S-bit S-box (masked)*/
static
void sboxes(
            Mat **matsSin,
			Mat *keyRound
            )
{
    /* Split matrices to rowsUpper and rowsLower */
    Mat **rows[MASKD], *left[MASKD], *right[MASKD];
    
    int indexOfMask;
    for (indexOfMask = 0; indexOfMask < MASKD; ++indexOfMask){
        rows[indexOfMask] = split(matsSin[indexOfMask], 2, 1);
        
        /* Get the Left  and the Right parts */
        left[indexOfMask] = rows[indexOfMask][0];
        right[indexOfMask] = rows[indexOfMask][1];
    }
    
    
    Mat **ptrOfL = left, **ptrOfR = right;
    
    /* Combine a bigger sbox, from 4-bit to 8-bit */
    int i, theLast = MASKD - 1;
    Mat **sum = NULL, **fout4b = NULL, *matOlder = NULL;

    /* Feistel Struct Begins */
    for (i = 0; i < FEISTEL; ++i)
    {
        /* Firstly, add key_r to the last masked component */

        matOlder = ptrOfL[theLast];
        ptrOfL[theLast] = add(ptrOfL[theLast], keyRound);
        
        
        /* Secondly, through a 4-bit sbox */
        fout4b = sbox4b(ptrOfL);
        
        /* Then do 'XOR' with the right matrix */
        /* Get the NEXT LEFT part  */
        
        sum = addWithMask(ptrOfR, fout4b);
        
        deMats(fout4b, MASKD);
        deMats(ptrOfR, MASKD);
        
        /* Finally, recover the left matrices */
        deMat(ptrOfL[theLast]);
        ptrOfL[theLast] = matOlder;
        ptrOfR = ptrOfL;
        ptrOfL = sum;
    }
    
    
    /* Catenate those vectors to a matrix to return */
    
    
    for (indexOfMask = 0; indexOfMask < MASKD; ++indexOfMask){
        rows[indexOfMask][0] = ptrOfL[indexOfMask];
        rows[indexOfMask][1] = ptrOfR[indexOfMask];
        
        deMat(matsSin[indexOfMask]);
        matsSin[indexOfMask] = cat(rows[indexOfMask], 2, 1);

        /* Deallocate all  */
        deMat(ptrOfL[indexOfMask]);
        deMat(ptrOfR[indexOfMask]);
        free(rows[indexOfMask]);
    }

}

#else /* Unmask */

/* DIM_S-bit S-box (unmask)*/
static
Mat *sboxes(
            const Mat *sin
            )
{
    /* Split matrix to rowsUpper and rowsLower */
    Mat **rows;
    rows = split(sin, 2, 1);


    /* Get the Left one and Right one */
    Mat *left, *right;
    left = rows[0];
    right = rows[1];

    /* Combine a bigger sbox, from 4-bit to 8-bit */
    int i;
    Mat *sum, *fout4b;
    /* Feistel Struct */
    for(i = 0; i < FEISTEL; ++i)
    {
        /* Firstly, do 'XOR' with key_r */
        sum = add(left, key_r);

        /* Secondly, through a 4-bit sbox */
        fout4b = sbox4b(sum);
        deMat(sum);

        /* Then do 'XOR' with the right matrix */
        sum = add(right, fout4b);
        deMat(fout4b);
        deMat(right);

        /* Finally, exchange each side */
        right = left;
        left = sum;
    }


    /* Catenate those vectors to a matrix to return */
    rows[0] = left;
    rows[1] = right;
    Mat *retMat;
    retMat = cat(rows, 2, 1);

    /* Deallocate all  */
    deMat(left);
    deMat(right);
    free(rows);

    return retMat;
}
#endif /* MASK */



/* Before encryption, do some pre-work to get the constant matrices */
static
void newPreCal()

{

	key_r = newMat(DIM_S / 2, DIM_L, keyRV, 0x03);

	rdConst[0] = newMat(DIM_S, DIM_L, rdConst1V, 0x03);
	rdConst[1] = newMat(DIM_S, DIM_L, rdConst2V, 0x03);
	rdConst[2] = newMat(DIM_S, DIM_L, rdConst3V, 0x03);
	rdConst[3] = newMat(DIM_S, DIM_L, rdConst4V, 0x03);
	rdConst[4] = newMat(DIM_S, DIM_L, rdConst5V, 0x03);
	rdConst[5] = newMat(DIM_S, DIM_L, rdConst6V, 0x03);
	rdConst[6] = newMat(DIM_S, DIM_L, rdConst7V, 0x03);
	rdConst[7] = newMat(DIM_S, DIM_L, rdConst8V, 0x03);

	rdConst[8] = newMat(DIM_S, DIM_L, rdConst9V, 0x03);
	rdConst[9] = newMat(DIM_S, DIM_L, rdConst10V, 0x03);
	rdConst[10] = newMat(DIM_S, DIM_L, rdConst11V, 0x03);
	rdConst[11] = newMat(DIM_S, DIM_L, rdConst12V, 0x03);
	rdConst[12] = newMat(DIM_S, DIM_L, rdConst13V, 0x03);
	rdConst[13] = newMat(DIM_S, DIM_L, rdConst14V, 0x03);
	rdConst[14] = newMat(DIM_S, DIM_L, rdConst15V, 0x03);
	rdConst[15] = newMat(DIM_S, DIM_L, rdConst16V, 0x03);

	matL = newMat(DIM_L, DIM_L, matLV, 0x03);


#if MASK

#if DIVIDE_SLICES != 1
	/* Get Matrix T- and L-*/
	LSlices = split(matL, DIVIDE_SLICES, 2);

#if DIM_A
	/* Get Matrix T  */
	/*  matDoubleInv looks like:
		| inv(A), 0      |
		|      0, inv(A) | */

	Mat *matWithInvA = newMat(DIM_A * DIVIDE_SLICES, DIM_A * DIVIDE_SLICES, NULL, 0x00);

	int btsOfRow = bytesOfRow(matWithInvA->dim_col);
	int btsOfMat = matInvA->dim_row * bytesOfRow(matInvA->dim_col);
	int indexOfSlices;
#if DIM_A == 4  &&  DIM_L == 8
	/* particular case*/
	memmove(matWithInvA->vect, matInvA->vect, btsOfMat * sizeof(BYTE));
	memmove(matWithInvA->vect + DIM_A, matInvA->vect, btsOfMat * sizeof(BYTE));
	/* Shift  bits  */
	int i;
	for(i = DIM_A; i != DIM_L; ++i) matWithInvA->vect[i] >>= 4;				
#else 
	/* general cases */
	int indexOfByte, indexOfDest = 0;
	for (indexOfSlices = 0; indexOfSlices != DIVIDE_SLICES; ++indexOfSlices){
#if DIM_A == 4
		BYTE oddFlag = (BYTE)indexOfSlices & 0x01; 
#endif
		for (indexOfByte = 0; indexOfByte != btsOfMat; ++indexOfByte){
#if DIM_A == 4
			matWithInvA->vect[indexOfDest] = oddFlag ? matInvA->vect[indexOfByte] >> 4
													 : matInvA->vect[indexOfByte];
#else
			matWithInvA->vect[indexOfDest] = matInvA->vect[indexOfByte];
#endif
			indexOfDest += btsOfRow;
		}
#if DIM_A == 4
		indexOfDest += oddFlag ? 1  : 0;
#else
		indexOfDest += 1;
#endif /* DIM_A == 4*/
	}
#endif/* get matWithInvA */
		
	for (indexOfSlices = 0; indexOfSlices != DIVIDE_SLICES; ++indexOfSlices){
		Mat *matRightPart = multiply(matTransA, LSlices[indexOfSlices]);
		TSlices[indexOfSlices] = multiply(matWithInvA, matRightPart);
		deMat(matRightPart);
	}
	deMat(matWithInvA);
#endif
	int indexOfRounds;
	for (indexOfRounds = 0; indexOfRounds != ROUNDS; ++indexOfRounds)
	{
		rdConstSlices[indexOfRounds] = split(rdConst[indexOfRounds], DIVIDE_SLICES, 2);
	}
	keyRoundSlices = split(key_r, DIVIDE_SLICES, 2);

	

 /*   splitHorizonParts(rdConst, rdCLeft, rdCRight, ROUNDS);
    keyRoundLR = split(key_r, 2, 2);*/

#elif DIM_A /* DIVIDE_SLICES == 1 && DIM_A != 0*/
    /* Get Matrix T  */
    Mat *matRight = multiply(matTransA, matL);
    matT = multiply(matInvA, matRight);

    deMat(matRight);

#endif /* DIVIDE_SLICES */

#endif /* MASK */
}



/* After encryption, deconstruct those matrices */
static
void dePostCal()
{

    deMat(key_r);
	deMats(rdConst, ROUNDS);
#if MASK  &&  DIM_A
/* USING A */
    deMat(matT);
#endif

}



/*=========================================================*/
/*   MARK: Public   Functions      */
/*=========================================================*/

Mat *encrypto(
                const Mat *plain,
                const Mat *key
            )
#if MASK
#if DIVIDE_SLICES != 1
{
#if DIM_A /* Using matrix A */
    setup();
#endif
    newPreCal();
    /* Split to slices in vertical dimension */

    Mat **keySlices = split(key, DIVIDE_SLICES, 2);
    Mat **plainSlices = split(plain, DIVIDE_SLICES, 2);

    Mat *cipherSlices[DIVIDE_SLICES];

    /* Encoded Plain */
	int indexOfSlices;
	int theLast = MASKD - 1;
	Mat **maskedPlain[DIVIDE_SLICES] = { 0 };
	for (indexOfSlices = 0; indexOfSlices != DIVIDE_SLICES; ++indexOfSlices){
		maskedPlain[indexOfSlices] = encode(plainSlices[indexOfSlices]);
		if (maskedPlain[indexOfSlices] == NULL) return NULL;
		
		/* Add Key */
		Mat *matTem = maskedPlain[indexOfSlices][theLast];
		maskedPlain[indexOfSlices][theLast] = add(maskedPlain[indexOfSlices][theLast], keySlices[indexOfSlices]);
		deMat(matTem);
	}
    
    int indexOfRound;
	Mat **matsMix = NULL;
	Mat **matsTem = NULL;
    for (indexOfRound = 0; indexOfRound != ROUNDS; ++indexOfRound)
    {
		for (indexOfSlices = 0; indexOfSlices != DIVIDE_SLICES; ++indexOfSlices){
			/* S-box */
			sboxes(maskedPlain[indexOfSlices], keyRoundSlices[indexOfSlices]);
			/* L-box */
			lboxes(maskedPlain[indexOfSlices], indexOfSlices);
			/* Mix the slices */
			if (!indexOfSlices) matsMix = maskedPlain[indexOfSlices];
			else {
				matsTem = matsMix;
				matsMix = addWithMask(maskedPlain[indexOfSlices], matsTem);
				deMats(maskedPlain[indexOfSlices], MASKD);
				deMats(matsTem, MASKD);
			}
			//maskedPlain[indexOfSlices] = (Mat **)malloc(MASKD * sizeof(Mat *));
		}  
		/* Split it */
		int indexOfMask;
		for (indexOfMask = 0; indexOfMask != MASKD; ++indexOfMask){
			matsTem = split(matsMix[indexOfMask], DIVIDE_SLICES, 2);
			for (indexOfSlices = 0; indexOfSlices != DIVIDE_SLICES; ++indexOfSlices){
				maskedPlain[indexOfSlices][indexOfMask] = matsTem[indexOfSlices];
			}			
		}
        

		/* Add Key And Round Constant */
		for (indexOfSlices = 0; indexOfSlices != DIVIDE_SLICES; ++indexOfSlices){
			Mat *roundKeySlice = add(rdConstSlices[indexOfRound][indexOfSlices], keySlices[indexOfSlices]);
			Mat *matTem = maskedPlain[indexOfSlices][theLast];
			maskedPlain[indexOfSlices][theLast] = add(roundKeySlice, matTem);
			deMat(matTem);
			deMat(roundKeySlice);
		}
    }

	for (indexOfSlices = 0; indexOfSlices != DIVIDE_SLICES; ++indexOfSlices){
		cipherSlices[indexOfSlices] = decode(maskedPlain[indexOfSlices]);
	}

	Mat *cipher = cat(cipherSlices, DIVIDE_SLICES, 2);

    dePostCal();
    return cipher;
}

#else /* DIVIDE_SLICES == 1 */
{
    if (plain == NULL || key == NULL || ROUNDS < 0) return NULL;
    Mat  *matRoundKey,  *matTem;

#if DIM_A /* Using matrix A */
    setup();
#endif
    newPreCal();
    /* Encoded Plain */
    Mat **matsMasked = encode(plain);
    if (matsMasked == NULL) return NULL;
    /* Add Key */
    int theLast = MASKD - 1;
    matTem = matsMasked[theLast];
    matsMasked[theLast] = add(matsMasked[theLast], key);
    deMat(matTem);

    int indexOfRound;
    for (indexOfRound = 0; indexOfRound < ROUNDS; ++indexOfRound)
    {
        sboxes(matsMasked, key_r);
		
        lboxes(matsMasked);
        
        //Res res =refreshing(matsMasked);

        matRoundKey = add(rdConst[indexOfRound], key);

        /* Add Key And Round Constant */
        matTem = matsMasked[theLast];
        matsMasked[theLast] = add(matsMasked[theLast], matRoundKey);

        deMat(matTem);
        deMat(matRoundKey);

    }

    /* Decode Cipher */
    Mat *cipher = decode(matsMasked);

    dePostCal();

    return cipher;

}
#endif /* DIVIDE_SLICES != 1 */

#else /* Unmask */
/* Encryption begins */
{
    if(plain == NULL || key == NULL || ROUNDS < 0) return NULL;

    Mat  *roundIn, *sum, *sout, *lout;	
    newPreCal();

    roundIn = add(plain, key);
    int round_i;
    for(round_i = 0; round_i < ROUNDS; ++round_i)
    {
        sout = sboxes(roundIn);
        lout = lboxes(sout);
        
        deMat(roundIn);
        deMat(sout);

        sum = add(lout, key);
        deMat(lout);

        roundIn = add(sum, rdConst[round_i]);
        deMat(sum);
    }

    dePostCal();
    return roundIn;
}
#endif /* MASK */
