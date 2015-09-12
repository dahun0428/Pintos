#include"fixed-point.h"
#include<stdint.h>

fixed_p fixed_convert = 16384;

fixed_p n2f (int n){
  return n*fixed_convert;
}

int f2n_t0 ( fixed_p x){
  return x/fixed_convert;
}

int f2n_near ( fixed_p x){

  int ret;


  if( x>=0 ) 
    ret = ( x + fixed_convert/2 ) / fixed_convert;
  else ret = ( x - fixed_convert/2 ) / fixed_convert;



  return ret;
}

fixed_p add_xy(fixed_p x, fixed_p y){
  return x+y;
}

fixed_p sub_xy(fixed_p x, fixed_p y){
  return x-y;
}


fixed_p add_xn(fixed_p x, int n){
  return x + n*fixed_convert;
}

fixed_p sub_xn(fixed_p x, int n){
  return x - n*fixed_convert;
}

fixed_p mul_xy(fixed_p x, fixed_p y){

  return ((int64_t) x) * y / fixed_convert;
}
fixed_p mul_xn(fixed_p x, int n){

  return x * n ;
}

fixed_p div_xy(fixed_p x, fixed_p y){

  return ((int64_t) x )* fixed_convert / y ;
}

fixed_p div_xn(fixed_p x, int n){

  return x / n;
}
