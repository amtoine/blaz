/*******************************************************
 * This file is part of the Blaz library
 * @Name ........ : dct.c
 * @Role ........ : discrete cosine transform and
                    quantization
 * @Author ...... : Matthieu Martel
 * @Version ..... : V1.1 06/30/2021
 * @Licence ..... : GPL V3
 * @Link ........ : https://github.com/mmartel66/blaz.git
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <blaz.h>

double dct_coef[64] = {
                        0.3535534, 0.3535534, 0.3535534, 0.3535534, 0.3535534, 0.3535534, 0.3535534, 0.3535534,
                        0.490393, 0.415735, 0.277785, 0.0975452, -0.0975452, -0.277785, -0.415735, -0.490393,
                        0.46194, 0.191342, -0.191342, -0.46194, -0.46194, -0.191342, 0.191342, 0.46194,
                        0.415735, -0.0975452, -0.490393, -0.277785, 0.277785, 0.490393, 0.0975452, -0.415735,
                        0.353553, -0.353553, -0.353553, 0.353553, 0.353553, -0.353553, -0.353553, 0.353553,
                        0.277785, -0.490393, 0.0975452, 0.415735, -0.415735, -0.0975452, 0.490393, -0.277785,
                        0.191342, -0.46194, 0.46194, -0.191342, -0.191342, 0.46194, -0.46194, 0.191342,
                        0.0975452, -0.277785, 0.415735, -0.490393, 0.490393, -0.415735, 0.277785, -0.0975452
                      };

double dct_coef_transpose[64] = {
                        0.353553, 0.490393, 0.461940, 0.415735, 0.353553, 0.277785, 0.191342, 0.097545,
                        0.353553, 0.415735, 0.191342, -0.097545, -0.353553, -0.490393, -0.461940, -0.277785,
                        0.353553, 0.277785, -0.191342, -0.490393, -0.353553, 0.097545, 0.461940, 0.415735,
                        0.353553, 0.097545, -0.461940, -0.277785, 0.353553, 0.415735, -0.191342, -0.490393,
                        0.353553, -0.097545, -0.461940, 0.277785, 0.353553, -0.415735, -0.191342, 0.490393,
                        0.353553, -0.277785, -0.191342, 0.490393, -0.353553, -0.097545, 0.461940, -0.415735,
                        0.353553, -0.415735, 0.191342, 0.097545, -0.353553, 0.490393, -0.461940, 0.277785,
                        0.353553, -0.490393, 0.461940, -0.415735, 0.353553, -0.277785, 0.191342, -0.097545
                      };


double *dct_block_matrix_multiply(double *block, double *matrix) {
  double *result_block;
  double res;
  int i,j,k;

  result_block = (double*)malloc(BLOCK_SIZE * BLOCK_SIZE * sizeof(double));

  for(i=0; i<BLOCK_SIZE; i++) {
    for(j=0; j<BLOCK_SIZE; j++) {
      res = 0.0;
      for(k=0; k<BLOCK_SIZE; k++) {
        res += block[POS(k, i, BLOCK_SIZE)] * matrix[POS(j, k, BLOCK_SIZE)];
      }
      result_block[POS(j, i, BLOCK_SIZE)] = res;
    }
  }
  return result_block;
}


void dct_matrix_block_multiply(double *block, double *matrix, s_8 *quantize_vector, int offset) {
  s_8 *result_block;
  s_8 quantize_coef;
  double *tmp_block;
  double max_elt, res;
  int i, j, k;

  tmp_block = (double*)malloc(BLOCK_SIZE * BLOCK_SIZE * sizeof(double));
  result_block = (s_8*)malloc(BLOCK_SIZE * BLOCK_SIZE * sizeof(s_8));
  max_elt = DBL_MIN;

  for(i=0; i<BLOCK_SIZE; i++) {
    for(j=0; j<BLOCK_SIZE; j++) {
      res = 0.0;
      for(k=0; k<BLOCK_SIZE; k++) {
        res += matrix[POS(k, i, BLOCK_SIZE)] * block[POS(j, k, BLOCK_SIZE)];
      }
      tmp_block[POS(j, i, BLOCK_SIZE)] = res;
      if (fabs(res)>max_elt) max_elt = fabs(res);
    }
  }

  quantize_coef = (s_8)(127.0 / max_elt);

  for(i=0; i<BLOCK_SIZE; i++) {
    for(j=0; j<BLOCK_SIZE; j++) {
      result_block[POS(j, i, BLOCK_SIZE)] = (s_8)(tmp_block[POS(j, i, BLOCK_SIZE)] * (double)quantize_coef);
    }
  }

  quantize_vector[offset] = quantize_coef;
  for(i=0; i<BLOCK_SIZE; i++) {
    quantize_vector[offset+i+1] = result_block[POS(i, 0, BLOCK_SIZE)];
    quantize_vector[offset+i+9] = result_block[POS(i, 1, BLOCK_SIZE)];
  }
  for(i=0; i<BLOCK_SIZE - 2; i++) {
    quantize_vector[offset+i+17] = result_block[POS(0, i+2, BLOCK_SIZE)];
    quantize_vector[offset+i+23] = result_block[POS(1, i+2, BLOCK_SIZE)];
  }
}


void idct_block_matrix_multiply(double *block, double *matrix, double *result_block, double quantize_coef) {
  double res;
  int i, j, k;

  for(i=0; i<BLOCK_SIZE; i++) {
    for(j=0; j<BLOCK_SIZE; j++) {
      res = 0.0;
      for(k=0; k<BLOCK_SIZE; k++) {
        res += block[POS(k, i, BLOCK_SIZE)] * matrix[POS(j, k, BLOCK_SIZE)];
      }
      result_block[POS(j, i, BLOCK_SIZE)] = res / quantize_coef;
    }
  }
}


double *idct_matrix_block_multiply(double *block, double *matrix, double *result_matrix) {
  int i, j, k;
  double res;

  for(i=0; i<BLOCK_SIZE; i++) {
    for(j=0; j<BLOCK_SIZE; j++) {
      res = 0.0;
      for(k=0; k<BLOCK_SIZE; k++) {
        res += matrix[POS(k, i, BLOCK_SIZE)] * block[POS(j, k, BLOCK_SIZE)];
      }
      result_matrix[POS(j, i, BLOCK_SIZE)] = res;
    }
  }
}


void dct(double *matrix, s_8 *quantized_vector, int offset) {
  double *block_1;

  block_1 = dct_block_matrix_multiply(dct_coef, matrix);
  dct_matrix_block_multiply(dct_coef_transpose, block_1, quantized_vector, offset);
}


void idct(double *matrix, s_8 *quantized_vector, int offset) {
  double * block_0, *block_1, *block_2;
  double quantize_coef;
  int i, j, k;

  block_0 = (double*)malloc(BLOCK_SIZE * BLOCK_SIZE * sizeof(double));
  block_1 = (double*)malloc(BLOCK_SIZE * BLOCK_SIZE * sizeof(double));

  quantize_coef = (double)quantized_vector[offset];

  k = 1;
  for(i=0; i<BLOCK_SIZE; i++) {
      block_0[POS(i, 0, BLOCK_SIZE)] = (double)quantized_vector[offset + k];
      block_0[POS(i, 1, BLOCK_SIZE)] = (double)quantized_vector[offset + k + BLOCK_SIZE];
      k++;
  }

  k = 2 * BLOCK_SIZE + 1;
  for(i=2; i<BLOCK_SIZE; i++) {
    block_0[POS(0, i, BLOCK_SIZE)] = (double)quantized_vector[offset + k];
    block_0[POS(1, i, BLOCK_SIZE)] = (double)quantized_vector[offset + k + BLOCK_SIZE - 2];
    k++;
  }

  for(i=2; i<BLOCK_SIZE; i++) {
    for(j=2; j<BLOCK_SIZE; j++) {
      block_0[POS(j, i, BLOCK_SIZE)] = 0.0;
    }
  }

  idct_block_matrix_multiply(dct_coef_transpose, block_0, block_1, quantize_coef);
  idct_matrix_block_multiply(dct_coef, block_1, matrix);
}

/*
int main() {
  double *a,*b,*e;
  double *a2,*b2,*c2,*d2,*e2,*c3,*d3,*e3;
  double *add,ms,ss,*f;
  double *d, *s, *g,*g2,*g3;
  s_8 *c, quant;

  double afe,afe2,x,y,err,abs_err,rel_err;
  int ia,ja,ir,jr;
  int i,j;
    printf("\n");
  a = (double*)malloc(64*sizeof(double));
  b = (double*)malloc(64*sizeof(double));
  e = (double*)malloc(64*sizeof(double));
  f = (double*)malloc(64*sizeof(double));

  c3 = (double*)malloc(64*sizeof(double));
  e3 = (double*)malloc(64*sizeof(double));

  a2 = (double*)malloc(64*sizeof(double));
  b2 = (double*)malloc(64*sizeof(double));
  e2 = (double*)malloc(64*sizeof(double));
  add = (double*)malloc(64*sizeof(double));

  s = (double*)malloc(64 * sizeof(double));

  for(i=0;i<8;i++) {
    for(j=0;j<8;j++) {
      x = 0.1 * (double)i;
      y = 0.1 * (double)j;
      a[i*8+j] = x * x * x -  y *y *y ;
    }
  }

  for(i=0;i<8;i++) {
    for(j=0;j<8;j++) {
      x = 0.1 * (double)i;
      y = 0.1 * (double)j;
      a2[i*8+j] = x + y ;
    }
  }

    afe = block_delta(matrix, b, 0, 0, 8, 8);
  block_slope(b, s, &ms, &ss, 0, 0, 8, 8);

  c = dct(s,&quant,0,0,8);


  for(i=2;i<8;i++) {
    for(j=2;j<8;j++) {
      c[POS(j,i,8)] = 0;
    }
  }

  d = idct(c,quant,0,0,8);

  block_unslope(d, e, ms, ss, 0, 0, 8, 8);
  block_undelta(e, f, afe, 0, 0, 8, 8);

  for(i=0;i<8;i++) {
    for(j=0;j<8;j++) {
      printf("%i ",c[POS(j,i,8)]);
    }
    printf("\n");
  }
  printf("====dddddddddd===============\n");

  for(i=0;i<8;i++) {
    for(j=0;j<8;j++) {
      printf("%f ",d[POS(j,i,8)]);
    }
    printf("\n");
  }
  printf("\n");

  printf("============slope=======\n");

  for(i=0;i<8;i++) {
    for(j=0;j<8;j++) {
      printf("%f ",s[POS(j,i,8)]);
    }
    printf("\n");
  }
  printf("\n");

  for(i=0;i<8;i++) {
    for(j=0;j<8;j++) {
      printf("%f ",a[POS(j,i,8)]);
    }
    printf("\n");
  }
  printf("\n");


    for(i=0;i<8;i++) {
      for(j=0;j<8;j++) {
        printf("%f ",f[POS(j,i,8)]);
      }
      printf("\n");
    }
    printf("\n");


  abs_err = rel_err = -111111.1;

  for(i=0; i<8; i++) {
    for(j=0; j<8; j++) {
      //printf("%f ",matrix2[POS(j,i,width)]);
      err = fabs(a[POS(j,i,8)] - f[POS(j,i,8)]);
      if (err>abs_err) { abs_err = err; ia =i; ja = j; };
      err /= a[POS(j,i,8)];
      if ((err>rel_err)&(a[POS(j,i,8)]!=0)) { rel_err = err; ir = i; jr = j; };
//      printf("%f ",e);
    }
  }
  printf("\nAdd normal:\n");
  printf("Worst absolute error: %f (%d,%d)\n",abs_err,ja,ia);
  printf("Worst relative error: %f (%d,%d)\n",rel_err,jr,ir);
}
/*
      abs_err = rel_err = -111111.1;

      for(i=0; i<8; i++) {
        for(j=0; j<8; j++) {
          //printf("%f ",matrix2[POS(j,i,width)]);
          err = fabs(add[POS(j,i,8)] - e3[POS(j,i,8)]);
          if (err>abs_err) { abs_err = err; ia =i; ja = j; };
          err /= add[POS(j,i,8)];
          if ((err>rel_err)&(add[POS(j,i,8)]!=0)) { rel_err = err; ir = i; jr = j; };
    //      printf("%f ",e);
        }
      }
      printf("\nAdd compressed:\n");
      printf("Worst absolute error: %f (%d,%d)\n",abs_err,ja,ia);
      printf("Worst relative error: %f (%d,%d)\n",rel_err,jr,ir);

}

*/

/*
void dct(u_64 *vector, double *result_vector, int len) {
  int i, j;
  double sum;
	double factor = M_PI / len;

	for (i = 0; i < len; i++) {
		sum = 0.0;
		for (j = 0; j < len; j++)
			sum += vector[j] * cos((j + 0.5) * i * factor);
    //if (i == 0) sum = sum / sqrt(2.0);
		result_vector[i] = sum;
	}
}


void idct(double *vector, u_64 *result_vector, int len) {
  int i, j;
  double sum, factor;

	factor = M_PI / len;

	for (i = 0; i < len; i++) {
		sum = vector[0] / 2.0;
		for (j = 1; j < len; j++)
			sum += vector[j] * cos(j * (i + 0.5) * factor);
		result_vector[i] = (u_64)(sum / len * 2.0);
	}
}


*/
/*
void dct(double *vector, double *result_vector, int len) {
  int i, j;
  double sum;
	double factor = M_PI / len;
printf("\n");
	for (i=0; i<len; i++) {
		sum = 0.0;
		for (j=0; j<len; j++)
			sum += vector[j] * cos((j + 0.5) * i * factor);
		result_vector[i] = sum;
    printf("%d %f\n",i,sum);
	}
}


void idct(double *vector, double *result_vector, int len) {
  int i, j;
  double sum, factor;

	factor = M_PI / len;

	for (i=0; i<len; i++) {
		sum = vector[0] / 2.0;
		for (j=1; j<len; j++)
			sum += vector[j] * cos(j * (i + 0.5) * factor);
		result_vector[i] = sum / len * 2.0;
	}
}

int cmpfct(double *x,double *y) {
  if (*x>*y) return 1;
  else if (*x<*y) return -1; else return 0;
}

void quantize(double *vector, float *result_vector, int offset, int len) {
  int i;

  //qsort(vector,16,sizeof(double),cmpfct);
  for(i=0; i<len; i++) {
    result_vector[i + offset] = (float)vector[i];
  }
}


void unquantize(float *vector, double *result_vector, int offset, int quantized_len, int unquantized_len) {
  int i;
  for(i=0; i<quantized_len; i++) {
    result_vector[i] = (double)vector[i + offset];
  }
  for(i=quantized_len; i<unquantized_len; i++) {
    result_vector[i] = 0.0;
  }
}
*/
