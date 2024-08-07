/*
  An example:

#include "deque.h"
int main() {
	kdq_t(int) array;
	kdq_init(array);
	kdq_push(int, array, 10); // append
	kdq_a(int, array, 20) = 5; // dynamic
	kdq_A(array, 20) = 4; // static
	kdq_destroy(array);
	return 0;
}
*/

#ifndef _DEQUE_H
#define _DEQUE_H

#include <stdlib.h>

#define kdq_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

#define kdq_t(type) struct { size_t s, n, m; type *a; }
#define kdq_init(v) ((v).s = (v).n = (v).m = 0, (v).a = 0)
#define kdq_destroy(v) free((v).a)
#define kdq_empty(v) (((v).s) = ((v).n) = 0)
#define kdq_A(v, i) ((v).a[(v).s + (i)])
#define kdq_pop(v) ((v).a[--(v).n])

#define kdq_unshift(v) do {	\
		if ((v).n > 0) {	\
			(--(v).n);		\
			(++(v).s);		\
		}					\
	} while (0)

#define kdq_size(v) ((v).n)
#define kdq_max(v) ((v).m)

#define kdq_resize(type, v, s)  ((v).m = (s), (v).a = (type*)realloc((v).a, sizeof(type) * (v).m))

#define kdq_copy(type, v1, v0) do {							\
		if ((v1).m < (v0).n) kdq_resize(type, v1, (v0).n);	\
		(v1).n = (v0).n;									\
		memcpy((v1).a, (v0).a, sizeof(type) * (v0).n);		\
	} while (0)												\

#define kdq_push(type, v, x) do {									\
		if ((v).s + (v).n == (v).m) {								\
			(v).m = (v).m? (v).m<<1 : 2;							\
			(v).a = (type*)realloc((v).a, sizeof(type) * (v).m);	\
		}															\
		(v).a[(v).s + (v).n++] = (x);								\
	} while (0)

#define kdq_pushp(type, v) (((v).n == (v).m)?							\
						   ((v).m = ((v).m? (v).m<<1 : 2),				\
							(v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0)	\
						   : 0), ((v).a + ((v).n++))

#endif
