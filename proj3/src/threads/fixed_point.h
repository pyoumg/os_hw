#ifndef FIXED_POINT_H
#define FIXED_POINT_H
typedef long long int64_t;

#define F (1 << 14) 
#define INT_MAX ((1 << 31) - 1)
#define INT_MIN (-(1 << 31))

int int_to_fp(int n); 
int fp_to_int(int x);
int fp_to_int_r(int x); 

int mul_fp(int x, int y);
int mul_mix(int x, int n);
int div_fp(int x, int y);
int div_mix(int x, int n);

int add_fp(int x, int y); 
int add_mix(int x, int n); 
int sub_fp(int x, int y); 
int sub_mix(int x, int n); 

int int_to_fp(int n) {
	return n * F;
}
int fp_to_int(int x) {
	return x / F;
}
int fp_to_int_r(int x) {
	if (x >= 0) {
		return (x + F / 2) / F;
	}
	else {
		return (x - F / 2) / F;
	}
}


int mul_fp(int x, int y) {
	return (int64_t)x * y / F;
}
int mul_mix(int x, int n) {
	return x * n;
}
int div_fp(int x, int y) {
	return (int64_t)x * F / y;

}
int div_mix(int x, int n) {
	return x / n;
}


int add_fp(int x, int y) {
	return x + y;
}
int add_mix(int x, int n) {
	return x + n * F;
}

//x-y
int sub_fp(int x, int y) {
	return x - y;
}
//x-n
int sub_mix(int x, int n) {
	return x - n * F;
}


#endif