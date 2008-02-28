#ifndef EXCEPT_MACROS_H
#define EXCEPT_MACROS_H

/* The variable is daddr */
#define cmp_ip(ip) \
  ((daddr == (ip)))
#define cmp_ip_and_mask(ip, mask) \
  ((daddr & (mask)) == (ip & (mask)))

/* lazy */
#define op0(x) cmp_ip(x)
#define op1(x,y) cmp_ip_and_mask(x,y)
#define op2(a,b,c,d) n_to_ip(a,b,c,d)

#endif

