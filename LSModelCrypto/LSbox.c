//
//  Lbox.c
//  L-S-model
//
//  Created by benny on 16/1/5.
//
//

#include "LSbox.h"




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
	Mat *retMat;

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


    /* generate the correct order */
    /* d a b c */
    Mat *ordered[] = {imd[3], imd[0], imd[1], imd[2]};

    retMat =  cat(ordered, 4, 1);

    /* deallocate all pointers */
    int i;
    for(i = 0; i < 4; ++i)
    {
        deMat(imd[i]);
        deMat(rvect[i]);
    }

	return retMat;
}




static
Mat *lboxes(
        const Mat *lin
)
{
	Mat *retMat;
	retMat = multiply(lin, matL);
    return retMat;
}




static
Mat *sboxes(
        const Mat *sin
)
{
    /* split matrix to rowsUpper and rowsLower*/
    Mat **rows;
    rows = split(sin, 2, 1);

	Mat *left, *right;
	Mat *retMat;

    /* 2 intermediate matrices */
    left = sbox4b(rows[0]);
	right = rows[1];
	
    /* combine each 4-bit*/
    int i;
    Mat *sum, *fout4b;
    for(i = 0; i < 3; ++i)
    {
        sum = add(left, key_r);

        fout4b = sbox4b(sum);
        deMat(sum);

        sum = add(right, fout4b);
        deMat(fout4b);
        deMat(right);

        right = left;
        left = sum;
    }

	deMat(rows[0]);
    rows[0] = left;
    rows[1] = right;

    retMat = cat(rows, 2, 1);

    deMat(left);
    deMat(right);
    free(rows);

    return retMat;
}



/* ============= */
/* public funcs */
/* ============= */



/* before encrypt, do some pre-work to get the constant matrices */
void newPreCal()
{
	matL = newMat(DIM_S, DIM_L, matLV, 0x03);
	key_r = newMat(DIM_S/2, DIM_L, keyRV, 0x03);
	rdConst[0] = newMat(DIM_S, DIM_L, rdConst1V, 0x03);
	rdConst[1] = newMat(DIM_S, DIM_L, rdConst2V, 0x03);
	rdConst[2] = newMat(DIM_S, DIM_L, rdConst3V, 0x03);
}

/* after encrypt, deconstruct those matrices */
void dePostCal()
{
	deMat(matL);
	deMat(key_r);
	int i;
	for (i = 0; i < ROUNDS; ++i){
		deMat(rdConst[i]);
	}
}





/* Encryption */
Mat *encryp(
        const Mat *plain,
		const Mat *key

)
{
    if(plain == NULL || key == NULL || ROUNDS < 0) return NULL;


    Mat  *roundIn, *sum, *sout, *lout;
	

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


    return roundIn;
}






