

#include "SysDataCPU.h"


#define SAME  0

#define STR(x)   #x
#define XSTR(x)  STR(x)


#define MAX(l,r)            ((l) > (r) ? (l) : (r))

#define LENGTH(array, type) (sizeof(array) / sizeof(type))
#define STRLEN(s)           (LENGTH(s,char) - 1)

#define BPI  32
#define POWER2(power) \
   (1 << (power))
#define RIGHTMASK(width) \
   (((width) >= BPI) ? ~0 : POWER2(width)-1)
#define BIT_EXTRACT_LE(value, start, after) \
   (((value) & RIGHTMASK(after)) >> start)


/*
** Query macros are used in the synth tables to disambiguate multiple chips
** with the same family, model, and/or stepping.
*/

#define is_intel      (stash->vendor == VENDOR_INTEL)
#define is_amd        (stash->vendor == VENDOR_AMD)
#define is_transmeta  (stash->vendor == VENDOR_TRANSMETA)
#define is_mobile     (stash->br.mobile)

/*
** Intel major queries:
**
** d? = think "desktop"
** s? = think "server" (multiprocessor)
** M? = think "mobile"
** X? = think "Extreme Edition"
**
** ?P = think Pentium
** ?C = think Celeron
** ?X = think Xeon
** ?M = think Xeon MP / Pentium M
** ?c = think Core (or generic CPU)
** ?d = think Pentium D
*/
#define dP ((is_intel && stash->br.pentium) \
            || stash->bri.desktop_pentium)
#define dC ((is_intel && !is_mobile && stash->br.celeron) \
            || stash->bri.desktop_celeron)
#define dd (is_intel && stash->br.pentium_d)
#define dc (is_intel && !is_mobile && (stash->br.core || stash->br.generic))
#define sX ((is_intel && stash->br.xeon) || stash->bri.xeon)
#define sM ((is_intel && stash->br.xeon_mp) \
            || stash->bri.xeon_mp)
#define MP ((is_intel && is_mobile && stash->br.pentium) \
            || stash->bri.mobile_pentium)
#define MC ((is_intel && is_mobile && stash->br.celeron) \
            || stash->bri.mobile_celeron)
#define MM ((is_intel && stash->br.pentium_m)\
            || stash->bri.mobile_pentium_m)
#define Mc (is_intel && is_mobile && stash->br.core)
#define Xc (is_intel && stash->br.extreme)



/* 
** Intel special cases 
*/
/* Pentium II Xeon (Deschutes), distinguished from Pentium II (Deschutes) */
#define xD (stash->L2_4w_1Mor2M)
/* Mobile Pentium II (Deschutes), distinguished from Pentium II (Deschutes) */
#define mD (stash->L2_4w_256K)
/* Intel Celeron (Deschutes), distinguished from  Pentium II (Deschutes) */
#define cD (!stash->L2_4w_1Mor2M && !stash->L2_4w_512K && !stash->L2_4w_256K)
/* Pentium III Xeon (Katmai), distinguished from Pentium III (Katmai) */
#define xK (stash->L2_4w_1Mor2M || stash->L2_8w_1Mor2M)
/* Pentium II (Katmai), verified, so distinguished from fallback case */
#define pK ((stash->L2_4w_512K || stash->L2_8w_256K || stash->L2_8w_512K) \
            && !stash->L2_4w_1Mor2M && !stash->L2_8w_1Mor2M)
/* Irwindale, distinguished from Nocona */
#define sI (sX && stash->L3)
/* Potomac, distinguished from Cranford */
#define sP (sM && stash->L3)
/* Allendale, distinguished from Conroe */
#define da (dc && stash->L2_2M)
/* Dual-Core Xeon Processor 5100 (Woodcrest B1) pre-production,
   distinguished from Core 2 Duo (Conroe B1) */
#define QW (dc && stash->br.generic \
            && (stash->mp.cores == 4 \
                || (stash->mp.cores == 2 && stash->mp.hyperthreads == 2)))
/* Core Duo (Yonah), distinguished from Core Solo (Yonah) */
#define Dc (dc && stash->mp.cores == 2)
/* Core 2 Quad, distinguished from Core 2 Duo */
#define Qc (dc && stash->mp.cores == 4)
/* Core 2 Extreme (Conroe B1), distinguished from Core 2 Duo (Conroe B1) */
#define XE (dc && strstr(stash->brand, " E6800") != NULL)
/* Quad-Core Xeon, distinguished from Xeon; and
   Xeon Processor 3300, distinguished from Xeon Processor 3100 */
#define sQ (sX && stash->mp.cores == 4)
/* Xeon Processor 7000, distinguished from Xeon */
#define IS_VMX(val_1_ecx)  (BIT_EXTRACT_LE((val_1_ecx), 5, 6))
#define s7 (sX && IS_VMX(stash->val_1_ecx))
/* Wolfdale C0/E0, distinguished from Wolfdale M0/R0 */
#define de (dc && stash->L2_6M)
/* Penryn C0/E0, distinguished from Penryn M0/R0 */
#define Me (Mc && stash->L2_6M)
/* Yorkfield C1/E0, distinguished from Yorkfield M1/E0 */
#define Qe (Qc && stash->L2_6M)
/* Yorkfield E0, distinguished from Yorkfield R0 */
#define se (sQ && stash->L2_6M)



#define __F(v)     ((v) & 0x0ff00f00)
#define __M(v)     ((v) & 0x000f00f0)
#define __FM(v)    ((v) & 0x0fff0ff0)
#define __FMS(v)   ((v) & 0x0fff0fff)

#define __TF(v)    ((v) & 0x0ff03f00)
#define __TFM(v)   ((v) & 0x0fff3ff0)
#define __TFMS(v)  ((v) & 0x0fff3fff)

#define _T(v)      ((v) << 12)
#define _F(v)      ((v) << 8)
#define _M(v)      ((v) << 4)
#define _S(v)      (v)
#define _XF(v)     ((v) << 20)
#define _XM(v)     ((v) << 16)

#define __B(v)     ((v) & 0x000000ff)
#define _B(v)      (v)

#define _FM(xf,f,xm,m)     (_XF(xf) + _F(f) + _XM(xm) + _M(m))
#define _FMS(xf,f,xm,m,s)  (_XF(xf) + _F(f) + _XM(xm) + _M(m) + _S(s))

#define START \
   if (0)
#define F(xf,f,str) \
   else if (__F(val)    ==       _XF(xf)        +_F(f)                              ) return(str)
#define FM(xf,f,xm,m,str) \
   else if (__FM(val)   ==       _XF(xf)+_XM(xm)+_F(f)+_M(m)                        ) return(str)
#define FMS(xf,f,xm,m,s,str) \
   else if (__FMS(val)  ==       _XF(xf)+_XM(xm)+_F(f)+_M(m)+_S(s)                  ) return(str)
#define TF(t,xf,f,str) \
   else if (__TF(val)   == _T(t)+_XF(xf)        +_F(f)                              ) return(str)
#define TFM(t,xf,f,xm,m,str) \
   else if (__TFM(val)  == _T(t)+_XF(xf)+_XM(xm)+_F(f)+_M(m)                        ) return(str)
#define TFMS(t,xf,f,xm,m,s,str) \
   else if (__TFMS(val) == _T(t)+_XF(xf)+_XM(xm)+_F(f)+_M(m)+_S(s)                  ) return(str)
#define FQ(xf,f,q,str) \
   else if (__F(val)    ==       _XF(xf)        +_F(f)             && (stash) && (q)) return(str)
#define FMQ(xf,f,xm,m,q,str) \
   else if (__FM(val)   ==       _XF(xf)+_XM(xm)+_F(f)+_M(m)       && (stash) && (q)) return(str)
#define FMSQ(xf,f,xm,m,s,q,str) \
   else if (__FMS(val)  ==       _XF(xf)+_XM(xm)+_F(f)+_M(m)+_S(s) && (stash) && (q)) return(str)
#define DEFAULT(str) \
   else                                                                               return(str)
#define FALLBACK(code) \
   else code





char* SysDataCPU::get_intel_name(const char* name, unsigned int val,  /* val_1_eax */const code_stash_t*  stash)
{

   //family ex family model ex model
 
   START;
   FM  (    0, 4,  0, 0,         "Intel i80486DX-25/33");
   FM  (    0, 4,  0, 1,         "Intel i80486DX-50");
   FM  (    0, 4,  0, 2,         "Intel i80486SX");
   FM  (    0, 4,  0, 3,         "Intel i80486DX/2");
   FM  (    0, 4,  0, 4,         "Intel i80486SL");
   FM  (    0, 4,  0, 5,         "Intel i80486SX/2");
   FM  (    0, 4,  0, 7,         "Intel i80486DX/2-WB");
   FM  (    0, 4,  0, 8,         "Intel i80486DX/4");
   FM  (    0, 4,  0, 9,         "Intel i80486DX/4-WB");
   F   (    0, 4,                "Intel i80486 (unknown model)");
   FM  (    0, 5,  0, 0,         "Intel Pentium 60/66 A-step");
   TFM (1,  0, 5,  0, 1,         "Intel Pentium 60/66 OverDrive for P5");
   FMS (    0, 5,  0, 1,  3,     "Intel Pentium 60/66 (B1)");
   FMS (    0, 5,  0, 1,  5,     "Intel Pentium 60/66 (C1)");
   FMS (    0, 5,  0, 1,  7,     "Intel Pentium 60/66 (D1)");
   FM  (    0, 5,  0, 1,         "Intel Pentium 60/66");
   TFM (1,  0, 5,  0, 2,         "Intel Pentium 75 - 200 OverDrive for P54C");
   FMS (    0, 5,  0, 2,  1,     "Intel Pentium P54C 75 - 200 (B1)");
   FMS (    0, 5,  0, 2,  2,     "Intel Pentium P54C 75 - 200 (B3)");
   FMS (    0, 5,  0, 2,  4,     "Intel Pentium P54C 75 - 200 (B5)");
   FMS (    0, 5,  0, 2,  5,     "Intel Pentium P54C 75 - 200 (C2/mA1)");
   FMS (    0, 5,  0, 2,  6,     "Intel Pentium P54C 75 - 200 (E0)");
   FMS (    0, 5,  0, 2, 11,     "Intel Pentium P54C 75 - 200 (cB1)");
   FMS (    0, 5,  0, 2, 12,     "Intel Pentium P54C 75 - 200 (cC0)");
   FM  (    0, 5,  0, 2,         "Intel Pentium P54C 75 - 200");
   TFM (1,  0, 5,  0, 3,         "Intel Pentium OverDrive for i486 (P24T)");
   TFM (1,  0, 5,  0, 4,         "Intel Pentium OverDrive for P54C");
   FMS (    0, 5,  0, 4,  3,     "Intel Pentium MMX P55C (B1)");
   FMS (    0, 5,  0, 4,  4,     "Intel Pentium MMX P55C (A3)");
   FM  (    0, 5,  0, 4,         "Intel Pentium MMX P55C");
   FMS (    0, 5,  0, 7,  0,     "Intel Pentium MMX P54C 75 - 200 (A4)");
   FM  (    0, 5,  0, 7,         "Intel Pentium MMX P54C 75 - 200");
   FMS (    0, 5,  0, 8,  1,     "Intel Pentium MMX P55C (A0), .25um");
   FMS (    0, 5,  0, 8,  2,     "Intel Pentium MMX P55C (B2), .25um");
   FM  (    0, 5,  0, 8,         "Intel Pentium MMX P55C, .25um");
   F   (    0, 5,                "Intel Pentium (unknown model)");
   FM  (    0, 6,  0, 0,         "Intel Pentium Pro A-step");
   FMS (    0, 6,  0, 1,  1,     "Intel Pentium Pro (B0)");
   FMS (    0, 6,  0, 1,  2,     "Intel Pentium Pro (C0)");
   FMS (    0, 6,  0, 1,  6,     "Intel Pentium Pro (sA0)");
   FMS (    0, 6,  0, 1,  7,     "Intel Pentium Pro (sA1)");
   FMS (    0, 6,  0, 1,  9,     "Intel Pentium Pro (sB1)");
   FM  (    0, 6,  0, 1,         "Intel Pentium Pro");
   TFM (1,  0, 6,  0, 3,         "Intel Pentium II OverDrive");
   FMS (    0, 6,  0, 3,  3,     "Intel Pentium II (Klamath C0), .28um");
   FMS (    0, 6,  0, 3,  4,     "Intel Pentium II (Klamath C1), .28um");
   FM  (    0, 6,  0, 3,         "Intel Pentium II (Klamath), .28um");
   FM  (    0, 6,  0, 4,         "Intel Pentium P55CT OverDrive (Deschutes)");
   FMSQ(    0, 6,  0, 5,  0, xD, "Intel Pentium II Xeon (Deschutes A0), .25um");
   FMSQ(    0, 6,  0, 5,  0, mD, "Intel Mobile Pentium II (Deschutes A0), .25um");
   FMSQ(    0, 6,  0, 5,  0, cD, "Intel Celeron (Deschutes A0), .25um");
   FMS (    0, 6,  0, 5,  0,     "Intel Pentium II / Pentium II Xeon / Mobile Pentium II (Deschutes A0), .25um");
   FMSQ(    0, 6,  0, 5,  1, xD, "Intel Pentium II Xeon (Deschutes A1), .25um");
   FMSQ(    0, 6,  0, 5,  1, cD, "Intel Celeron (Deschutes A1), .25um");
   FMS (    0, 6,  0, 5,  1,     "Intel Pentium II / Pentium II Xeon (Deschutes A1), .25um");
   FMSQ(    0, 6,  0, 5,  2, xD, "Intel Pentium II Xeon (Deschutes B0), .25um");
   FMSQ(    0, 6,  0, 5,  2, mD, "Intel Mobile Pentium II (Deschutes B0), .25um");
   FMSQ(    0, 6,  0, 5,  2, cD, "Intel Celeron (Deschutes B0), .25um");
   FMS (    0, 6,  0, 5,  2,     "Intel Pentium II / Pentium II Xeon / Mobile Pentium II (Deschutes B0), .25um");
   FMSQ(    0, 6,  0, 5,  3, xD, "Intel Pentium II Xeon (Deschutes B1), .25um");
   FMSQ(    0, 6,  0, 5,  3, cD, "Intel Celeron (Deschutes B1), .25um");
   FMS (    0, 6,  0, 5,  3,     "Intel Pentium II / Pentium II Xeon (Deschutes B1), .25um");
   FMQ (    0, 6,  0, 5,     xD, "Intel Pentium II Xeon (Deschutes), .25um");
   FMQ (    0, 6,  0, 5,     mD, "Intel Mobile Pentium II (Deschutes), .25um");
   FMQ (    0, 6,  0, 5,     cD, "Intel Celeron (Deschutes), .25um");
   FM  (    0, 6,  0, 5,         "Intel Pentium II / Pentium II Xeon / Mobile Pentium II (Deschutes), .25um");
   FMSQ(    0, 6,  0, 6,  0, dP, "Intel Pentium II (Mendocino A0), L2 cache");
   FMSQ(    0, 6,  0, 6,  0, dC, "Intel Celeron (Mendocino A0), L2 cache");
   FMS (    0, 6,  0, 6,  0,     "Intel Pentium II / Celeron (Mendocino A0), L2 cache");
   FMSQ(    0, 6,  0, 6,  5, dC, "Intel Celeron (Mendocino B0), L2 cache");
   FMSQ(    0, 6,  0, 6,  5, dP, "Intel Pentium II (Mendocino B0), L2 cache");
   FMS (    0, 6,  0, 6,  5,     "Intel Pentium II / Celeron (Mendocino B0), L2 cache");
   FMS (    0, 6,  0, 6, 10,     "Intel Mobile Pentium II (Mendocino A0), L2 cache");
   FM  (    0, 6,  0, 6,         "Intel Pentium II (Mendocino), L2 cache");
   FMSQ(    0, 6,  0, 7,  2, pK, "Intel Pentium III (Katmai B0), .25um");
   FMSQ(    0, 6,  0, 7,  2, xK, "Intel Pentium III Xeon (Katmai B0), .25um");
   FMS (    0, 6,  0, 7,  2,     "Intel Pentium III / Pentium III Xeon (Katmai B0), .25um");
   FMSQ(    0, 6,  0, 7,  3, pK, "Intel Pentium III (Katmai C0), .25um");
   FMSQ(    0, 6,  0, 7,  3, xK, "Intel Pentium III Xeon (Katmai C0), .25um");
   FMS (    0, 6,  0, 7,  3,     "Intel Pentium III / Pentium III Xeon (Katmai C0), .25um");
   FMQ (    0, 6,  0, 7,     pK, "Intel Pentium III (Katmai), .25um");
   FMQ (    0, 6,  0, 7,     xK, "Intel Pentium III Xeon (Katmai), .25um");
   FM  (    0, 6,  0, 7,         "Intel Pentium III / Pentium III Xeon (Katmai), .25um");
   FMSQ(    0, 6,  0, 8,  1, sX, "Intel Pentium III Xeon (Coppermine A2), .18um");
   FMSQ(    0, 6,  0, 8,  1, MC, "Intel Mobile Celeron (Coppermine A2), .18um");
   FMSQ(    0, 6,  0, 8,  1, dC, "Intel Celeron (Coppermine A2), .18um");
   FMSQ(    0, 6,  0, 8,  1, MP, "Intel Mobile Pentium III (Coppermine A2), .18um");
   FMSQ(    0, 6,  0, 8,  1, dP, "Intel Pentium III (Coppermine A2), .18um");
   FMS (    0, 6,  0, 8,  1,     "Intel Pentium III / Pentium III Xeon / Celeron / Mobile Pentium III / Mobile Celeron (Coppermine A2), .18um");
   FMSQ(    0, 6,  0, 8,  3, sX, "Intel Pentium III Xeon (Coppermine B0), .18um");
   FMSQ(    0, 6,  0, 8,  3, MC, "Intel Mobile Celeron (Coppermine B0), .18um");
   FMSQ(    0, 6,  0, 8,  3, dC, "Intel Celeron (Coppermine B0), .18um");
   FMSQ(    0, 6,  0, 8,  3, MP, "Intel Mobile Pentium III (Coppermine B0), .18um");
   FMSQ(    0, 6,  0, 8,  3, dP, "Intel Pentium III (Coppermine B0), .18um");
   FMS (    0, 6,  0, 8,  3,     "Intel Pentium III / Pentium III Xeon / Celeron / Mobile Pentium III / Mobile Celeron (Coppermine B0), .18um");
   FMSQ(    0, 6,  0, 8,  6, sX, "Intel Pentium III Xeon (Coppermine C0), .18um");
   FMSQ(    0, 6,  0, 8,  6, MC, "Intel Mobile Celeron (Coppermine C0), .18um");
   FMSQ(    0, 6,  0, 8,  6, dC, "Intel Celeron (Coppermine C0), .18um");
   FMSQ(    0, 6,  0, 8,  6, MP, "Intel Mobile Pentium III (Coppermine C0), .18um");
   FMSQ(    0, 6,  0, 8,  6, dP, "Intel Pentium III (Coppermine C0), .18um");
   FMS (    0, 6,  0, 8,  6,     "Intel Pentium III / Pentium III Xeon / Celeron / Mobile Pentium III / Mobile Celeron (Coppermine C0), .18um");
   FMSQ(    0, 6,  0, 8, 10, sX, "Intel Pentium III Xeon (Coppermine D0), .18um");
   FMSQ(    0, 6,  0, 8, 10, MC, "Intel Mobile Celeron (Coppermine D0), .18um");
   FMSQ(    0, 6,  0, 8, 10, dC, "Intel Celeron (Coppermine D0), .18um");
   FMSQ(    0, 6,  0, 8, 10, MP, "Intel Mobile Pentium III (Coppermine D0), .18um");
   FMSQ(    0, 6,  0, 8, 10, dP, "Intel Pentium III (Coppermine D0), .18um");
   FMS (    0, 6,  0, 8, 10,     "Intel Pentium III / Pentium III Xeon / Celeron / Mobile Pentium III / Mobile Celeron (Coppermine D0), .18um");
   FMQ (    0, 6,  0, 8,     sX, "Intel Pentium III Xeon (Coppermine), .18um");
   FMQ (    0, 6,  0, 8,     MC, "Intel Mobile Celeron (Coppermine), .18um");
   FMQ (    0, 6,  0, 8,     dC, "Intel Celeron (Coppermine), .18um");
   FMQ (    0, 6,  0, 8,     MP, "Intel Mobile Pentium III (Coppermine), .18um");
   FMQ (    0, 6,  0, 8,     dP, "Intel Pentium III (Coppermine), .18um");
   FM  (    0, 6,  0, 8,         "Intel Pentium III / Pentium III Xeon / Celeron / Mobile Pentium III / Mobile Celeron (Coppermine), .18um");
   FMSQ(    0, 6,  0, 9,  5, dC, "Intel Celeron M (Banias B1), .13um");
   FMSQ(    0, 6,  0, 9,  5, dP, "Intel Pentium M (Banias B1), .13um");
   FMS (    0, 6,  0, 9,  5,     "Intel Pentium M / Celeron M (Banias B1), .13um");
   FMQ (    0, 6,  0, 9,     dC, "Intel Celeron M (Banias), .13um");
   FMQ (    0, 6,  0, 9,     dP, "Intel Pentium M (Banias), .13um");
   FM  (    0, 6,  0, 9,         "Intel Pentium M / Celeron M (Banias), .13um");
   FMS (    0, 6,  0,10,  0,     "Intel Pentium III Xeon (Cascades A0), .18um");
   if(0);
   FMS (    0, 6,  0,10,  1,     "Intel Pentium III Xeon (Cascades A1), .18um");
   FMS (    0, 6,  0,10,  4,     "Intel Pentium III Xeon (Cascades B0), .18um");
   FM  (    0, 6,  0,10,         "Intel Pentium III Xeon (Cascades), .18um");
   FMSQ(    0, 6,  0,11,  1, dC, "Intel Celeron (Tualatin A1), .13um");
   FMSQ(    0, 6,  0,11,  1, MC, "Intel Mobile Celeron (Tualatin A1), .13um");
   FMSQ(    0, 6,  0,11,  1, dP, "Intel Pentium III (Tualatin A1), .13um");
   FMS (    0, 6,  0,11,  1,     "Intel Pentium III / Celeron / Mobile Celeron (Tualatin A1), .13um");
   FMSQ(    0, 6,  0,11,  4, dC, "Intel Celeron (Tualatin B1), .13um");
   FMSQ(    0, 6,  0,11,  4, MC, "Intel Mobile Celeron (Tualatin B1), .13um");
   FMSQ(    0, 6,  0,11,  4, dP, "Intel Pentium III (Tualatin B1), .13um");
   FMS (    0, 6,  0,11,  4,     "Intel Pentium III / Celeron / Mobile Celeron (Tualatin B1), .13um");
   FMQ (    0, 6,  0,11,     dC, "Intel Celeron (Tualatin), .13um");
   FMQ (    0, 6,  0,11,     MC, "Intel Mobile Celeron (Tualatin), .13um");
   FMQ (    0, 6,  0,11,     dP, "Intel Pentium III (Tualatin), .13um");
   FM  (    0, 6,  0,11,         "Intel Pentium III / Celeron / Mobile Celeron (Tualatin), .13um");
   FMSQ(    0, 6,  0,13,  6, dC, "Intel Celeron M (Dothan B1), 90nm");
   FMSQ(    0, 6,  0,13,  6, dP, "Intel Pentium M (Dothan B1), 90nm");
   FMS (    0, 6,  0,13,  6,     "Intel Pentium M (Dothan B1) / Celeron M (Dothan B1), 90nm");
   FMSQ(    0, 6,  0,13,  8, dC, "Intel Celeron M (Dothan C0), 90nm");
   FMSQ(    0, 6,  0,13,  8, MP, "Intel Processor A100/A110 (Stealey C0), 90nm");
   FMSQ(    0, 6,  0,13,  8, dP, "Intel Pentium M (Dothan C0), 90nm");
   FMS (    0, 6,  0,13,  8,     "Intel Pentium M (Dothan C0) / Celeron M (Dothan C0) / A100/A110 (Stealey C0), 90nm");
   FMQ (    0, 6,  0,13,     dC, "Intel Celeron M (Dothan), 90nm");
   FMQ (    0, 6,  0,13,     MP, "Intel Processor A100/A110 (Stealey), 90nm");
   FMQ (    0, 6,  0,13,     dP, "Intel Pentium M (Dothan), 90nm");
   FM  (    0, 6,  0,13,         "Intel Pentium M (Dothan) / Celeron M (Dothan), 90nm");
   FMSQ(    0, 6,  0,14,  8, sX, "Intel Xeon Processor LV (Sossaman C0), 65nm");
   FMSQ(    0, 6,  0,14,  8, dC, "Intel Celeron (Yonah C0), 65nm");
   FMSQ(    0, 6,  0,14,  8, Dc, "Intel Core Duo (Yonah C0), 65nm");
   FMSQ(    0, 6,  0,14,  8, dc, "Intel Core Solo (Yonah C0), 65nm");
   FMS (    0, 6,  0,14,  8,     "Intel Core Solo (Yonah C0) / Core Duo (Yonah C0) / Xeon Processor LV (Sossaman C0) / Celeron (Yonah C0), 65nm");
   FMSQ(    0, 6,  0,14, 12, sX, "Intel Xeon Processor LV (Sossaman D0), 65nm");
   FMSQ(    0, 6,  0,14, 12, dC, "Intel Celeron M (Yonah D0), 65nm");
   FMSQ(    0, 6,  0,14, 12, MP, "Intel Pentium Dual-Core Mobile T2000 (Yonah D0), 65nm");
   FMSQ(    0, 6,  0,14, 12, Dc, "Intel Core Duo (Yonah D0), 65nm");
   FMSQ(    0, 6,  0,14, 12, dc, "Intel Core Solo (Yonah D0), 65nm");
   FMS (    0, 6,  0,14, 12,     "Intel Core Solo (Yonah D0) / Core Duo (Yonah D0) / Xeon Processor LV (Sossaman D0) / Pentium Dual-Core Mobile T2000 (Yonah D0) / Celeron M (Yonah D0), 65nm");
   FMS (    0, 6,  0,14, 13,     "Intel Pentium Dual-Core Mobile T2000 (Yonah M0), 65nm");
   FMQ (    0, 6,  0,14,     sX, "Intel Xeon Processor LV (Sossaman), 65nm");
   FMQ (    0, 6,  0,14,     dC, "Intel Celeron (Yonah), 65nm");
   FMQ (    0, 6,  0,14,     MP, "Intel Pentium Dual-Core Mobile (Yonah), 65nm");
   FMQ (    0, 6,  0,14,     Dc, "Intel Core Duo (Yonah), 65nm");
   FMQ (    0, 6,  0,14,     dc, "Intel Core Solo (Yonah), 65nm");
   FM  (    0, 6,  0,14,         "Intel Core Solo (Yonah) / Core Duo (Yonah) / Xeon Processor LV (Sossaman) / Celeron (Yonah) / Pentium Dual-Core Mobile (Yonah), 65nm");
   FMSQ(    0, 6,  0,15,  2, sX, "Intel Dual-Core Xeon Processor 3000 (Conroe L2), 65nm");
   FMSQ(    0, 6,  0,15,  2, Mc, "Intel Core Duo Mobile (Merom L2), 65nm");
   FMSQ(    0, 6,  0,15,  2, dc, "Intel Core Duo (Conroe L2), 65nm");
   FMSQ(    0, 6,  0,15,  2, dP, "Intel Pentium Dual-Core Desktop Processor E2000 (Allendale L2), 65nm");
   FMS (    0, 6,  0,15,  2,     "Intel Core Duo (Conroe L2) / Core Duo Mobile (Merom L2) / Pentium Dual-Core Desktop Processor E2000 (Allendale L2) / Dual-Core Xeon Processor 3000 (Conroe L2), 65nm");
   FMS (    0, 6,  0,15,  4,     "Intel Core 2 Duo (Conroe B0) / Xeon Processor 5100 (Woodcrest B0) (pre-production), 65nm");
   FMSQ(    0, 6,  0,15,  5, QW, "Intel Dual-Core Xeon Processor 5100 (Woodcrest B1) (pre-production), 65nm");
   FMSQ(    0, 6,  0,15,  5, XE, "Intel Core 2 Extreme Processor (Conroe B1), 65nm");
   FMSQ(    0, 6,  0,15,  5, da, "Intel Core 2 Duo (Allendale B1), 65nm");
   FMSQ(    0, 6,  0,15,  5, dc, "Intel Core 2 Duo (Conroe B1), 65nm");
   FMS (    0, 6,  0,15,  5,     "Intel Core 2 Duo (Conroe/Allendale B1) / Core 2 Extreme Processor (Conroe B1), 65nm");
   FMSQ(    0, 6,  0,15,  6, Xc, "Intel Core 2 Extreme Processor (Conroe B2), 65nm");
   FMSQ(    0, 6,  0,15,  6, Mc, "Intel Core 2 Duo Mobile (Merom B2), 65nm");
   FMSQ(    0, 6,  0,15,  6, da, "Intel Core 2 Duo (Allendale B2), 65nm");
   FMSQ(    0, 6,  0,15,  6, dc, "Intel Core 2 Duo (Conroe B2), 65nm");
   FMSQ(    0, 6,  0,15,  6, dC, "Intel Celeron M (Conroe B2), 65nm");
   FMSQ(    0, 6,  0,15,  6, sX, "Intel Dual-Core Xeon Processor 3000 (Conroe B2) / Dual-Core Xeon Processor 5100 (Woodcrest B2), 65nm");
   FMS (    0, 6,  0,15,  6,     "Intel Core 2 Duo (Conroe/Allendale B2) / Core 2 Extreme Processor (Conroe B2) / Dual-Core Xeon Processor 3000 (Conroe B2) / Dual-Core Xeon Processor 5100 (Woodcrest B2) / Core 2 Duo Mobile (Conroe B2), 65nm");
   FMSQ(    0, 6,  0,15,  7, sX, "Intel Quad-Core Xeon Processor 3200 (Kentsfield B3) / Quad-Core Xeon Processor 5300 (Clovertown B3), 65nm");
   FMSQ(    0, 6,  0,15,  7, Xc, "Intel Core 2 Extreme Quad-Core Processor QX6xx0 (Kentsfield B3), 65nm");
   FMS (    0, 6,  0,15,  7,     "Intel Quad-Core Xeon Processor 3200 (Kentsfield B3) / Quad-Core Xeon Processor 5300 (Clovertown B3) / Core 2 Extreme Quad-Core Processor QX6700 (Clovertown B3)a, 65nm");
   FMSQ(    0, 6,  0,15, 10, Mc, "Intel Core 2 Duo Mobile (Merom E1), 65nm");
   FMSQ(    0, 6,  0,15, 10, dC, "Intel Celeron Processor 500 (Merom E1), 65nm");
   FMS (    0, 6,  0,15, 10,     "Intel Core 2 Duo Mobile (Merom E1) / Celeron Processor 500 (Merom E1), 65nm");
   FMSQ(    0, 6,  0,15, 11, sQ, "Intel Quad-Core Xeon Processor 5300 (Clovertown G0), 65nm");
   FMSQ(    0, 6,  0,15, 11, sX, "Intel Xeon Processor 3000 (Conroe G0) / Xeon Processor 3200 (Kentsfield G0) / Xeon Processor 7200/7300 (Tigerton G0), 65nm");
   FMSQ(    0, 6,  0,15, 11, Xc, "Intel Core 2 Extreme Quad-Core Processor QX6xx0 (Kentsfield G0), 65nm");
   FMSQ(    0, 6,  0,15, 11, Mc, "Intel Core 2 Duo Mobile (Merom G2), 65nm");
   FMSQ(    0, 6,  0,15, 11, Qc, "Intel Core 2 Quad (Conroe G0), 65nm");
   FMSQ(    0, 6,  0,15, 11, dc, "Intel Core 2 Duo (Conroe G0), 65nm");
   FMS (    0, 6,  0,15, 11,     "Intel Core 2 Duo (Conroe G0) / Xeon Processor 3000 (Conroe G0) / Xeon Processor 3200 (Kentsfield G0) / Xeon Processor 7200/7300 (Tigerton G0) / Quad-Core Xeon Processor 5300 (Clovertown G0) / Core 2 Extreme Quad-Core Processor QX6xx0 (Kentsfield G0) / Core 2 Duo Mobile (Merom G2), 65nm");
   FMSQ(    0, 6,  0,15, 13, Mc, "Intel Core 2 Duo Mobile (Merom M1) / Celeron Processor 500 (Merom E1), 65nm");
   FMSQ(    0, 6,  0,15, 13, Qc, "Intel Core 2 Quad (Conroe M0), 65nm");
   FMSQ(    0, 6,  0,15, 13, dc, "Intel Core 2 Duo (Conroe M0), 65nm");
   FMSQ(    0, 6,  0,15, 13, dP, "Intel Pentium Dual-Core Desktop Processor E2000 (Allendale M0), 65nm");
   FMSQ(    0, 6,  0,15, 13, dC, "Intel Celeron Dual-Core E1000 (Allendale M0) / Celeron Dual-Core T1000 (Merom M0)");
   FMS (    0, 6,  0,15, 13,     "Intel Core 2 Duo (Conroe M0) / Core 2 Duo Mobile (Merom M1) / Celeron Processor 500 (Merom E1) / Pentium Dual-Core Desktop Processor E2000 (Allendale M0) / Celeron Dual-Core E1000 (Allendale M0) / Celeron Dual-Core T1000 (Merom M0)");
   FMQ (    0, 6,  0,15,     sQ, "Intel Quad-Core Xeon (Woodcrest), 65nm");
   FMQ (    0, 6,  0,15,     sX, "Intel Dual-Core Xeon (Conroe / Woodcrest) / Quad-Core Xeon (Kentsfield / Clovertown) / Core 2 Extreme Quad-Core (Clovertown) / Xeon (Tigerton G0), 65nm");
   FMQ (    0, 6,  0,15,     Xc, "Intel Core 2 Extreme Processor (Conroe) / Core 2 Extreme Quad-Core (Kentsfield), 65nm");
   FMQ (    0, 6,  0,15,     Mc, "Intel Core Duo Mobile / Core 2 Duo Mobile (Merom) / Celeron (Merom), 65nm");
   FMQ (    0, 6,  0,15,     Qc, "Intel Core 2 Quad (Conroe), 65nm");
   FMQ (    0, 6,  0,15,     dc, "Intel Core Duo / Core 2 Duo (Conroe), 65nm");
   FMQ (    0, 6,  0,15,     dP, "Intel Pentium Dual-Core (Allendale), 65nm");
   FMQ (    0, 6,  0,15,     dC, "Intel Celeron M (Conroe) / Celeron (Merom) / Celeron Dual-Core (Allendale), 65nm");
   FM  (    0, 6,  0,15,         "Intel Core Duo / Core 2 Duo (Conroe / Allendale) / Core Duo Mobile (Merom) / Core 2 Duo Mobile (Merom) / Celeron (Merom) / Core 2 Extreme (Conroe) / Core 2 Extreme Quad-Core (Kentsfield) / Pentium Dual-Core (Allendale) / Celeron M (Conroe) / Celeron (Merom) / Celeron Dual-Core (Allendale) / Quad-Core Xeon (Kentsfield / Clovertown / Woodcrest) / Core 2 Extreme Quad-Core (Clovertown) / Xeon (Tigerton) / Dual-Core Xeon (Conroe / Woodcrest), 65nm");
   FMS (    0, 6,  1, 5,  0,     "Intel EP80579 (Tolapai B0)");
   FMSQ(    0, 6,  1, 6,  1, MC, "Intel Celeron Processor 200/400/500 (Conroe-L/Merom-L A1), 65nm");
   FMSQ(    0, 6,  1, 6,  1, dC, "Intel Celeron M (Merom-L A1), 65nm");
   FMSQ(    0, 6,  1, 6,  1, Mc, "Intel Core 2 Duo Mobile (Merom A1), 65nm");
   FMS (    0, 6,  1, 6,  1,     "Intel Core 2 Duo Mobile (Merom A1) / Celeron 200/400/500 (Conroe-L/Merom-L A1) / Celeron M (Merom-L A1), 65nm");
   FMQ (    0, 6,  1, 6,     MC, "Intel Celeron Processor 200/400/500 (Conroe-L/Merom-L), 65nm");
   FMQ (    0, 6,  1, 6,     dC, "Intel Celeron M (Merom-L), 65nm");
   FMQ (    0, 6,  1, 6,     Mc, "Intel Core 2 Duo Mobile (Merom), 65nm");
   FM  (    0, 6,  1, 6,         "Intel Core 2 Duo Mobile (Merom) / Celeron (Conroe-L/Merom-L) / Celeron M (Merom-L), 65nm");
   FMSQ(    0, 6,  1, 7,  6, sQ, "Intel Xeon Processor 3300 (Yorkfield C0) / Xeon Processor 5200 (Wolfdale C0) / Xeon Processor 5400 (Harpertown C0), 45nm");
   FMSQ(    0, 6,  1, 7,  6, sX, "Intel Xeon Processor 3100 (Wolfdale C0) / Xeon Processor 5200 (Wolfdale C0) / Xeon Processor 5400 (Harpertown C0), 45nm");
   FMSQ(    0, 6,  1, 7,  6, Xc, "Intel Core 2 Extreme QX9000 (Yorkfield C0), 45nm");
   FMSQ(    0, 6,  1, 7,  6, Me, "Intel Mobile Core 2 Duo (Penryn C0), 45nm");
   FMSQ(    0, 6,  1, 7,  6, Mc, "Intel Mobile Core 2 Duo (Penryn M0), 45nm");
   FMSQ(    0, 6,  1, 7,  6, de, "Intel Core 2 Duo (Wolfdale C0), 45nm");
   FMSQ(    0, 6,  1, 7,  6, dc, "Intel Core 2 Duo (Wolfdale M0), 45nm");
   FMSQ(    0, 6,  1, 7,  6, dP, "Intel Pentium Dual-Core Processor E5000 (Wolfdale M0), 45nm");
   FMS (    0, 6,  1, 7,  6,     "Intel Core 2 Duo (Wolfdale C0/M0) / Mobile Core 2 Duo (Penryn C0/M0) / Core 2 Extreme QX9000 (Yorkfield C0) / Pentium Dual-Core Processor E5000 (Wolfdale M0) / Xeon Processor 3100 (Wolfdale C0) / Xeon Processor 3300 (Yorkfield C0) / Xeon Processor 5200 (Wolfdale C0) / Xeon Processor 5400 (Harpertown C0), 45nm");
   FMSQ(    0, 6,  1, 7,  7, sQ, "Intel Xeon Processor 3300 (Yorkfield C1), 45nm");
   FMSQ(    0, 6,  1, 7,  7, Xc, "Intel Core 2 Extreme QX9000 (Yorkfield C1), 45nm");
   FMSQ(    0, 6,  1, 7,  7, Qe, "Intel Core 2 Quad-Core Q9000 (Yorkfield C1), 45nm");
   FMSQ(    0, 6,  1, 7,  7, Qc, "Intel Core 2 Quad-Core Q9000 (Yorkfield M1), 45nm");
   FMS (    0, 6,  1, 7,  7,     "Intel Core 2 Quad-Core Q9000 (Yorkfield C1/M1) / Core 2 Extreme QX9000 (Yorkfield C1) / Xeon Processor 3300 (Yorkfield C1), 45nm");
   FMSQ(    0, 6,  1, 7, 10, Me, "Intel Mobile Core 2 (Penryn E0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, Mc, "Intel Mobile Core 2 (Penryn R0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, Qe, "Intel Core 2 Quad-Core Q9000 (Yorkfield E0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, Qc, "Intel Core 2 Quad-Core Q9000 (Yorkfield R0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, de, "Intel Core 2 Duo (Wolfdale E0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, dc, "Intel Core 2 Duo (Wolfdale R0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, dP, "Intel Pentium Dual-Core Processor E5000/E6000 (Wolfdale R0), 45nm");
   if(0);
   FMSQ(    0, 6,  1, 7, 10, dC, "Intel Celeron E3000 (Wolfdale R0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, se, "Intel Xeon Processor 3300 (Yorkfield E0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, sQ, "Intel Xeon Processor 3300 (Yorkfield R0), 45nm");
   FMSQ(    0, 6,  1, 7, 10, sX, "Intel Xeon Processor 3100 (Wolfdale E0) / Xeon Processor 3300 (Yorkfield R0) / Xeon Processor 5200 (Wolfdale E0) / Xeon Processor 5400 (Harpertown E0), 45nm");
   FMS (    0, 6,  1, 7, 10,     "Intel Core 2 Duo (Wolfdale E0/R0) / Core 2 Quad-Core Q9000 (Yorkfield E0/R0) / Mobile Core 2 (Penryn E0/R0) / Pentium Dual-Core Processor E5000/E600 (Wolfdale R0) / Celeron E3000 (Wolfdale R0) / Xeon Processor 3100 (Wolfdale E0) / Xeon Processor 3300 (Yorkfield E0/R0) / Xeon Processor 5200 (Wolfdale E0) / Xeon Processor 5400 (Harpertown E0), 45nm");
   FMQ (    0, 6,  1, 7,     se, "Intel Xeon (Wolfdale / Yorkfield / Harpertown), 45nm");
   FMQ (    0, 6,  1, 7,     sQ, "Intel Xeon (Wolfdale / Yorkfield / Harpertown), 45nm");
   FMQ (    0, 6,  1, 7,     sX, "Intel Xeon (Wolfdale / Yorkfield / Harpertown), 45nm");
   FMQ (    0, 6,  1, 7,     Mc, "Intel Mobile Core 2 (Penryn), 45nm");
   FMQ (    0, 6,  1, 7,     Xc, "Intel Core 2 Extreme (Yorkfield), 45nm");
   FMQ (    0, 6,  1, 7,     Qc, "Intel Core 2 Quad-Core (Yorkfield), 45nm");
   FMQ (    0, 6,  1, 7,     dc, "Intel Core 2 Duo (Wolfdale), 45nm");
   FMQ (    0, 6,  1, 7,     dC, "Intel Celeron (Wolfdale), 45nm");
   FMQ (    0, 6,  1, 7,     dP, "Intel Pentium Dual-Core (Wolfdale), 45nm");
   FM  (    0, 6,  1, 7,         "Intel Core 2 Duo (Wolfdale) / Mobile Core 2 (Penryn) / Core 2 Quad-Core (Yorkfield) / Core 2 Extreme (Yorkfield) / Celeron (Wolfdale) / Pentium Dual-Core (Wolfdale) / Xeon (Wolfdale / Yorkfield / Harpertown), 45nm");
   FMS (    0, 6,  1,10,  4,     "Intel Core i7-900 (Bloomfield C0), 45nm");
   FMSQ(    0, 6,  1,10,  5, dc, "Intel Core i7-900 (Bloomfield D0), 45nm");
   FMSQ(    0, 6,  1,10,  5, sX, "Intel Xeon Processor 3500 (Bloomfield D0) / Xeon Processor 5500 (Gainestown D0), 45nm");
   FMS (    0, 6,  1,10,  5,     "Intel Core i7-900 (Bloomfield D0) / Xeon Processor 3500 (Bloomfield D0) / Xeon Processor 5500 (Gainestown D0), 45nm");
   FMQ (    0, 6,  1,10,     dc, "Intel Core (Bloomfield), 45nm");
   FMQ (    0, 6,  1,10,     sX, "Intel Xeon (Bloomfield / Gainestown), 45nm");
   FM  (    0, 6,  1,10,         "Intel Core (Bloomfield) / Xeon (Bloomfield / Gainestown), 45nm");
   FMS (    0, 6,  1,12,  1,     "Intel Atom N270 (Diamondville B0), 45nm");
   FMS (    0, 6,  1,12,  2,     "Intel Atom 200/N200/300 (Diamondville C0) / Atom Z500 (Silverthorne C0), 45nm");
   FMS (    0, 6,  1,12, 10,     "Intel Atom D400/N400 (Pineview A0) / Atom D500/N500 (Pineview B0), 45nm");
   FM  (    0, 6,  1,12,         "Intel Atom (Diamondville / Silverthorne / Pineview), 45nm");
   FMS (    0, 6,  1,13,  1,     "Intel Xeon Processor 7400 (Dunnington A1), 45nm");
   FM  (    0, 6,  1,13,         "Intel Xeon (Dunnington), 45nm");
   FMSQ(    0, 6,  1,14,  4, sX, "Intel Xeon Processor C3500/C5500 (Jasper Forest B0), 45nm");
   FMSQ(    0, 6,  1,14,  4, dC, "Intel Celeron P1053 (Jasper Forest B0), 45nm");
   FMQ (    0, 6,  1,14,     sX, "Intel Xeon Processor C3500/C5500 (Jasper Forest B0) / Celeron P1053 (Jasper Forest B0), 45nm");
   FMSQ(    0, 6,  1,14,  5, sX, "Intel Xeon Processor 3400 (Lynnfield B1), 45nm");
   FMSQ(    0, 6,  1,14,  5, Mc, "Intel Core i7-700/800/900 Mobile (Clarksfield B1), 45nm");
   FMSQ(    0, 6,  1,14,  5, dc, "Intel Core i5-700 / i7-800 (Lynnfield B1), 45nm");
   FMS (    0, 6,  1,14,  5,     "Intel Intel Core i5-700 / i7-800 (Lynnfield B1) / Core i7-700/800/900 Mobile (Clarksfield B1) / Xeon Processor 3400 (Lynnfield B1), 45nm");
   FMQ (    0, 6,  1,14,     sX, "Intel Xeon (Lynnfield) / Xeon (Jasper Forest), 45nm");
   FMQ (    0, 6,  1,14,     dC, "Intel Celeron (Jasper Forest), 45nm");
   FMQ (    0, 6,  1,14,     Mc, "Intel Core Mobile (Clarksfield), 45nm");
   FMQ (    0, 6,  1,14,     dc, "Intel Core (Lynnfield), 45nm");
   FM  (    0, 6,  1,14,         "Intel Intel Core (Lynnfield) / Core Mobile (Clarksfield) / Xeon (Lynnfield) / Xeon (Jasper Forest), 45nm");
   FMSQ(    0, 6,  2, 5,  2, sX, "Intel Xeon Processor L3406 (Clarkdale C2), 32nm");
   FMSQ(    0, 6,  2, 5,  2, MC, "Intel Celeron Mobile P4500 (Arrandale C2), 32nm");
   FMSQ(    0, 6,  2, 5,  2, MP, "Intel Pentium P6000 Mobile (Arrandale C2), 32nm");
   FMSQ(    0, 6,  2, 5,  2, dP, "Intel Pentium G6900 / P4505 (Clarkdale C2), 32nm");
   FMSQ(    0, 6,  2, 5,  2, Mc, "Intel Core i3-300 Mobile / Core i5-400 Mobile / Core i5-500 Mobile / Core i7-600 Mobile (Arrandale C2), 32nm");
   FMSQ(    0, 6,  2, 5,  2, dc, "Intel Core i3-300 / i3-500 / i5-500 / i5-600 / i7-600 (Clarkdale C2), 32nm");
   FMS (    0, 6,  2, 5,  2,     "Intel Core i3 / i5 / i7 (Clarkdale C2) / Core i3 Mobile / Core i5 Mobile / Core i7 Mobile (Arrandale C2) / Pentium P6000 Mobile (Arrandale C2) / Celeron Mobile P4500 (Arrandale C2) / Xeon Processor L3406 (Clarkdale C2), 32nm");
   FMSQ(    0, 6,  2, 5,  5, MC, "Intel Celeron Celeron Mobile U3400 (Arrandale K0) / Celeron Mobile P4600 (Arrandale K0), 32nm");
   FMSQ(    0, 6,  2, 5,  5, MP, "Intel Pentium U5000 Mobile (Arrandale K0), 32nm");
   FMSQ(    0, 6,  2, 5,  2, dP, "Intel Pentium P4505 / U3405 (Clarkdale K0), 32nm");
   FMSQ(    0, 6,  2, 5,  5, dc, "Intel Core i3-300 / i3-500 / i5-400 / i5-500 / i5-600 / i7-600 (Clarkdale K0), 32nm");
   FMS (    0, 6,  2, 5,  5,     "Intel Core i3 / i5 / i7  (Clarkdale K0) / Pentium U5000 Mobile / Pentium P4505 / U3405 / Celeron Mobile P4000 / U3000 (Arrandale K0), 32nm");
   FMQ (    0, 6,  2, 5,     sX, "Intel Xeon Processor L3406 (Clarkdale), 32nm");
   FMQ (    0, 6,  2, 5,     MC, "Intel Celeron Mobile (Arrandale), 32nm");
   FMQ (    0, 6,  2, 5,     MP, "Intel Pentium Mobile (Arrandale), 32nm");
   FMQ (    0, 6,  2, 5,     dP, "Intel Pentium (Clarkdale), 32nm");
   FMQ (    0, 6,  2, 5,     Mc, "Intel Core Mobile (Arrandale), 32nm");
   FMQ (    0, 6,  2, 5,     dc, "Intel Core (Clarkdale), 32nm");
   FM  (    0, 6,  2, 5,         "Intel Core (Clarkdale) / Core (Arrandale) / Pentium (Clarkdale) / Pentium Mobile (Arrandale) / Celeron Mobile (Arrandale) / Xeon (Clarkdale), 32nm");
   FMSQ(    0, 6,  2,10,  7, Xc, "Intel Mobile Core i7 Extreme (Sandy Bridge D2), 32nm");
   FMSQ(    0, 6,  2,10,  7, Mc, "Intel Mobile Core i3-2000 / Mobile Core i5-2000 / Mobile Core i7-2000 (Sandy Bridge D2), 32nm");
   FMSQ(    0, 6,  2,10,  7, dc, "Intel Core i3-2000 / Core i5-2000 / Core i7-2000 (Sandy Bridge D2), 32nm");
   FMS (    0, 6,  2,10,  7,     "Intel Core i3-2000 / Core i5-2000 / Core i7-2000 / Mobile Core i7-2000 (Sandy Bridge D2), 32nm");
   FMQ (    0, 6,  2,10,     Xc, "Intel Mobile Core i7 Extreme (Sandy Bridge D2), 32nm");
   FMQ (    0, 6,  2,10,     Mc, "Intel Mobile Core i3-2000 / Mobile Core i5-2000 / Mobile Core i7-2000 (Sandy Bridge D2), 32nm");
   FMQ (    0, 6,  2,10,     dc, "Intel Core i5-2000 / Core i7-2000 (Sandy Bridge D2), 32nm");
   FM  (    0, 6,  2,10,         "Intel Core i5-2000 / Core i7-2000 / Mobile Core i3-2000 / Mobile Core i5-2000 / Mobile Core i7-2000 (Sandy Bridge D2), 32nm");
   FMSQ(    0, 6,  2,12,  2, dc, "Intel Core i7-900 / Core i7-980X (Gulftown B1), 32nm");
   FMSQ(    0, 6,  2,12,  2, sX, "Intel Xeon Processor 3600 (Westmere-EP B1) / Xeon Processor 5600 (Westmere-EP B1), 32nm");
   FMS (    0, 6,  2,12,  2,     "Intel Core i7-900 (Gulftown B1) / Core i7-980X (Gulftown B1) / Xeon Processor 3600 (Westmere-EP B1) / Xeon Processor 5600 (Westmere-EP B1), 32nm");
   FM  (    0, 6,  2,12,         "Intel Core (Gulftown) / Xeon (Westmere-EP), 32nm");
   FMS (    0, 6,  2,14,  6,     "Intel Xeon Processor 7500 (Beckton D0), 45nm");
   FM  (    0, 6,  2,14,         "Intel Xeon (Beckton), 45nm");
   FQ  (    0, 6,            sX, "Intel Xeon (unknown model)");
   FQ  (    0, 6,            se, "Intel Xeon (unknown model)");
   FQ  (    0, 6,            MC, "Intel Mobile Celeron (unknown model)");
   FQ  (    0, 6,            dC, "Intel Celeron (unknown model)");
   FQ  (    0, 6,            Xc, "Intel Core Extreme (unknown model)");
   FQ  (    0, 6,            Mc, "Intel Mobile Core (unknown model)");
   FQ  (    0, 6,            dc, "Intel Core (unknown model)");
   FQ  (    0, 6,            MP, "Intel Mobile Pentium (unknown model)");
   FQ  (    0, 6,            dP, "Intel Pentium (unknown model)");
   F   (    0, 6,                "Intel Pentium II / Pentium III / Pentium M / Celeron / Celeron M / Core / Core 2 / Core i / Xeon / Atom (unknown model)");
   FMS (    0, 7,  0, 6,  4,     "Intel Itanium (Merced C0)");
   FMS (    0, 7,  0, 7,  4,     "Intel Itanium (Merced C1)");
   FMS (    0, 7,  0, 8,  4,     "Intel Itanium (Merced C2)");
   F   (    0, 7,                "Intel Itanium (unknown model)");
   FMS (    0,15,  0, 0,  7,     "Intel Pentium 4 (Willamette B2), .18um");
   FMSQ(    0,15,  0, 0, 10, dP, "Intel Pentium 4 (Willamette C1), .18um");
   FMSQ(    0,15,  0, 0, 10, sX, "Intel Xeon (Foster C1), .18um");
   FMS (    0,15,  0, 0, 10,     "Intel Pentium 4 (Willamette C1) / Xeon (Foster C1), .18um");
   FMQ (    0,15,  0, 0,     dP, "Intel Pentium 4 (Willamette), .18um");
   FMQ (    0,15,  0, 0,     sX, "Intel Xeon (Foster), .18um");
   FM  (    0,15,  0, 0,         "Intel Pentium 4 (Willamette) / Xeon (Foster), .18um");
   FMS (    0,15,  0, 1,  1,     "Intel Xeon MP (Foster C0), .18um");
   FMSQ(    0,15,  0, 1,  2, dP, "Intel Pentium 4 (Willamette D0), .18um");
   FMSQ(    0,15,  0, 1,  2, sX, "Intel Xeon (Foster D0), .18um");
   FMS (    0,15,  0, 1,  2,     "Intel Pentium 4 (Willamette D0) / Xeon (Foster D0), .18um");
   FMSQ(    0,15,  0, 1,  3, dP, "Intel Pentium 4(Willamette E0), .18um");
   FMSQ(    0,15,  0, 1,  3, dC, "Intel Celeron 478-pin (Willamette E0), .18um");
   FMS (    0,15,  0, 1,  3,     "Intel Pentium 4 / Celeron (Willamette E0), .18um");
   FMQ (    0,15,  0, 1,     dP, "Intel Pentium 4 (Willamette), .18um");
   FMQ (    0,15,  0, 1,     sX, "Intel Xeon (Foster), .18um");
   FM  (    0,15,  0, 1,         "Intel Pentium 4 (Willamette) / Xeon (Foster), .18um");
   FMS (    0,15,  0, 2,  2,     "Intel Xeon MP (Gallatin A0), .13um");
   FMSQ(    0,15,  0, 2,  4, sX, "Intel Xeon (Prestonia B0), .13um");
   FMSQ(    0,15,  0, 2,  4, MM, "Intel Mobile Pentium 4 Processor-M (Northwood B0), .13um");
   FMSQ(    0,15,  0, 2,  4, MC, "Intel Mobile Celeron (Northwood B0), .13um");
   FMSQ(    0,15,  0, 2,  4, dP, "Intel Pentium 4 (Northwood B0), .13um");
   FMS (    0,15,  0, 2,  4,     "Intel Pentium 4 (Northwood B0) / Xeon (Prestonia B0) / Mobile Pentium 4 Processor-M (Northwood B0) / Mobile Celeron (Northwood B0), .13um");
   FMSQ(    0,15,  0, 2,  5, dP, "Intel Pentium 4 (Northwood B1/M0), .13um");
   FMSQ(    0,15,  0, 2,  5, sM, "Intel Xeon MP (Gallatin B1), .13um");
   FMSQ(    0,15,  0, 2,  5, sX, "Intel Xeon (Prestonia B1), .13um");
   FMS (    0,15,  0, 2,  5,     "Intel Pentium 4 (Northwood B1/M0) / Xeon (Prestonia B1) / Xeon MP (Gallatin B1), .13um");
   FMS (    0,15,  0, 2,  6,     "Intel Xeon MP (Gallatin C0), .13um");
   FMSQ(    0,15,  0, 2,  7, sX, "Intel Xeon (Prestonia C1), .13um");
   FMSQ(    0,15,  0, 2,  7, dC, "Intel Celeron 478-pin (Northwood C1), .13um");
   FMSQ(    0,15,  0, 2,  7, MC, "Intel Mobile Celeron (Northwood C1), .13um");
   FMSQ(    0,15,  0, 2,  7, MM, "Intel Mobile Pentium 4 Processor-M (Northwood C1), .13um");
   FMSQ(    0,15,  0, 2,  7, dP, "Intel Pentium 4 (Northwood C1), .13um");
   FMS (    0,15,  0, 2,  7,     "Intel Pentium 4 (Northwood C1) / Xeon (Prestonia C1) / Mobile Pentium 4 Processor-M (Northwood C1) / Celeron 478-Pin (Northwood C1) / Mobile Celeron (Northwood C1), .13um");
   if(0);
   FMSQ(    0,15,  0, 2,  9, sX, "Intel Xeon (Prestonia D1), .13um");
   FMSQ(    0,15,  0, 2,  9, dC, "Intel Celeron 478-pin (Northwood D1), .13um");
   FMSQ(    0,15,  0, 2,  9, MC, "Intel Mobile Celeron (Northwood D1), .13um");
   FMSQ(    0,15,  0, 2,  9, MM, "Intel Mobile Pentium 4 Processor-M (Northwood D1), .13um");
   FMSQ(    0,15,  0, 2,  9, MP, "Intel Mobile Pentium 4 (Northwood D1), .13um");
   FMSQ(    0,15,  0, 2,  9, dP, "Intel Pentium 4 (Northwood D1), .13um");
   FMS (    0,15,  0, 2,  9,     "Intel Pentium 4 (Northwood D1) / Xeon (Prestonia D1) / Mobile Pentium 4 (Northwood D1) / Mobile Pentium 4 Processor-M (Northwood D1) / Celeron 478-pin (Northwood D1), .13um");
   FMQ (    0,15,  0, 2,     dP, "Intel Pentium 4 (Northwood), .13um");
   FMQ (    0,15,  0, 2,     sM, "Intel Xeon MP (Gallatin), .13um");
   FMQ (    0,15,  0, 2,     sX, "Intel Xeon (Prestonia), .13um");
   FM  (    0,15,  0, 2,         "Intel Pentium 4 (Northwood) / Xeon (Prestonia) / Xeon MP (Gallatin) / Mobile Pentium 4 / Mobile Pentium 4 Processor-M (Northwood) / Celeron 478-pin (Northwood), .13um");
   FMSQ(    0,15,  0, 3,  3, dP, "Intel Pentium 4 (Prescott C0), 90nm");
   FMSQ(    0,15,  0, 3,  3, dC, "Intel Celeron D (Prescott C0), 90nm");
   FMS (    0,15,  0, 3,  3,     "Intel Pentium 4 (Prescott C0) / Celeron D (Prescott C0), 90nm");
   FMSQ(    0,15,  0, 3,  4, sX, "Intel Xeon (Nocona D0), 90nm");
   FMSQ(    0,15,  0, 3,  4, dC, "Intel Celeron D (Prescott D0), 90nm");
   FMSQ(    0,15,  0, 3,  4, MP, "Intel Mobile Pentium 4 (Prescott D0), 90nm");
   FMSQ(    0,15,  0, 3,  4, dP, "Intel Pentium 4 (Prescott D0), 90nm");
   FMS (    0,15,  0, 3,  4,     "Intel Pentium 4 (Prescott D0) / Xeon (Nocona D0) / Mobile Pentium 4 (Prescott D0), 90nm");
   FMQ (    0,15,  0, 3,     sX, "Intel Xeon (Nocona), 90nm");
   FMQ(     0,15,  0, 3,     dC, "Intel Celeron D (Prescott), 90nm");
   FMQ (    0,15,  0, 3,     MP, "Intel Mobile Pentium 4 (Prescott), 90nm");
   FMQ (    0,15,  0, 3,     dP, "Intel Pentium 4 (Prescott), 90nm");
   FM  (    0,15,  0, 3,         "Intel Pentium 4 (Prescott) / Xeon (Nocona) / Mobile Pentium 4 (Prescott), 90nm");
   FMSQ(    0,15,  0, 4,  1, sP, "Intel Xeon MP (Potomac C0), 90nm");
   FMSQ(    0,15,  0, 4,  1, sM, "Intel Xeon MP (Cranford A0), 90nm");
   FMSQ(    0,15,  0, 4,  1, sX, "Intel Xeon (Nocona E0), 90nm");
   FMSQ(    0,15,  0, 4,  1, dC, "Intel Celeron D (Prescott E0), 90nm");
   FMSQ(    0,15,  0, 4,  1, MP, "Intel Mobile Pentium 4 (Prescott E0), 90nm");
   FMSQ(    0,15,  0, 4,  1, dP, "Intel Pentium 4 (Prescott E0), 90nm");
   FMS (    0,15,  0, 4,  1,     "Intel Pentium 4 (Prescott E0) / Xeon (Nocona E0) / Xeon MP (Cranford A0 / Potomac C0) / Celeron D (Prescott E0 ) / Mobile Pentium 4 (Prescott E0), 90nm");
   FMSQ(    0,15,  0, 4,  3, sI, "Intel Xeon (Irwindale N0), 90nm");
   FMSQ(    0,15,  0, 4,  3, sX, "Intel Xeon (Nocona N0), 90nm");
   FMSQ(    0,15,  0, 4,  3, dP, "Intel Pentium 4 (Prescott N0), 90nm");
   FMS (    0,15,  0, 4,  3,     "Intel Pentium 4 (Prescott N0) / Xeon (Nocona N0 / Irwindale N0), 90nm");
   FMSQ(    0,15,  0, 4,  4, dc, "Intel Pentium Extreme Edition Processor 840 (Smithfield A0), 90nm");
   FMSQ(    0,15,  0, 4,  4, dd, "Intel Pentium D Processor 8x0 (Smithfield A0), 90nm");
   FMS (    0,15,  0, 4,  4,     "Intel Pentium D Processor 8x0 (Smithfield A0) / Pentium Extreme Edition Processor 840 (Smithfield A0), 90nm");
   FMSQ(    0,15,  0, 4,  7, dc, "Pentium Extreme Edition Processor 840 (Smithfield B0), 90nm");
   FMSQ(    0,15,  0, 4,  7, dd, "Intel Pentium D Processor 8x0 (Smithfield B0), 90nm");
   FMS (    0,15,  0, 4,  7,     "Intel Pentium D Processor 8x0 (Smithfield B0) / Pentium Extreme Edition Processor 840 (Smithfield B0), 90nm");
   FMSQ(    0,15,  0, 4,  8, s7, "Intel Dual-Core Xeon Processor 7000 (Paxville A0), 90nm");
   FMSQ(    0,15,  0, 4,  8, sX, "Intel Dual-Core Xeon (Paxville A0), 90nm");
   FMS (    0,15,  0, 4,  8,     "Intel Dual-Core Xeon (Paxville A0) / Dual-Core Xeon Processor 7000 (Paxville A0), 90nm");
   FMSQ(    0,15,  0, 4,  9, sM, "Intel Xeon MP (Cranford B0), 90nm");
   FMSQ(    0,15,  0, 4,  9, dC, "Intel Celeron D (Prescott G1), 90nm");
   FMSQ(    0,15,  0, 4,  9, dP, "Intel Pentium 4 (Prescott G1), 90nm");
   FMS (    0,15,  0, 4,  9,     "Intel Pentium 4 (Prescott G1) / Xeon MP (Cranford B0) / Celeron D (Prescott G1), 90nm");
   FMSQ(    0,15,  0, 4, 10, sI, "Intel Xeon (Irwindale R0), 90nm");
   FMSQ(    0,15,  0, 4, 10, sX, "Intel Xeon (Nocona R0), 90nm");
   FMSQ(    0,15,  0, 4, 10, dP, "Intel Pentium 4 (Prescott R0), 90nm");
   FMS (    0,15,  0, 4, 10,     "Intel Pentium 4 (Prescott R0) / Xeon (Nocona R0 / Irwindale R0), 90nm");
   FMQ (    0,15,  0, 4,     sM, "Intel Xeon MP (Nocona/Potomac), 90nm");
   FMQ (    0,15,  0, 4,     sX, "Intel Xeon (Nocona/Irwindale), 90nm");
   FMQ (    0,15,  0, 4,     dC, "Intel Celeron D (Prescott), 90nm");
   FMQ (    0,15,  0, 4,     MP, "Intel Mobile Pentium 4 (Prescott), 90nm");
   FMQ (    0,15,  0, 4,     dd, "Intel Pentium D (Smithfield A0), 90nm");
   FMQ (    0,15,  0, 4,     dP, "Intel Pentium 4 (Prescott) / Pentium Extreme Edition (Smithfield A0), 90nm");
   FM  (    0,15,  0, 4,         "Intel Pentium 4 (Prescott) / Xeon (Nocona / Irwindale) / Pentium D (Smithfield A0) / Pentium Extreme Edition (Smithfield A0) / Mobile Pentium 4 (Prescott) / Xeon MP (Nocona) / Xeon MP (Cranford / Potomac) / Celeron D (Prescott) / Dual-Core Xeon (Paxville A0) / Dual-Core Xeon Processor 7000 (Paxville A0), 90nm");
   FMSQ(    0,15,  0, 6,  2, dd, "Intel Pentium D Processor 9xx (Presler B1), 65nm");
   FMSQ(    0,15,  0, 6,  2, dP, "Intel Pentium 4 Processor 6x1 (Cedar Mill B1) / Pentium Extreme Edition Processor 955 (Presler B1)");
   FMS (    0,15,  0, 6,  2,     "Intel Pentium 4 Processor 6x1 (Cedar Mill B1) / Pentium Extreme Edition Processor 955 (Presler B1) / Pentium D Processor 900 (Presler B1), 65nm");
   FMSQ(    0,15,  0, 6,  4, dd, "Intel Pentium D Processor 9xx (Presler C1), 65nm");
   FMSQ(    0,15,  0, 6,  4, dP, "Intel Pentium 4 Processor 6x1 (Cedar Mill C1) / Pentium Extreme Edition Processor 955 (Presler C1)");
   FMSQ(    0,15,  0, 6,  4, dC, "Intel Celeron D Processor 34x/35x (Cedar Mill C1), 65nm");
   FMSQ(    0,15,  0, 6,  4, sX, "Intel Xeon Processor 5000 (Dempsey C1), 65nm");
   FMS (    0,15,  0, 6,  4,     "Intel Pentium 4 Processor 6x1 (Cedar Mill C1) / Pentium Extreme Edition Processor 955 (Presler C1) / Pentium D Processor 9xx (Presler C1) / Xeon Processor 5000 (Dempsey C1) / Celeron D Processor 3xx (Cedar Mill C1), 65nm");
   FMSQ(    0,15,  0, 6,  5, dC, "Intel Celeron D Processor 36x (Cedar Mill D0), 65nm");
   FMSQ(    0,15,  0, 6,  5, dP, "Intel Pentium 4 Processor 6x1 (Cedar Mill D0) / Pentium Extreme Edition Processor 955 (Presler D0), 65nm");
   FMS (    0,15,  0, 6,  5,     "Intel Pentium 4 Processor 6x1 (Cedar Mill D0) / Pentium Extreme Edition Processor 955 (Presler D0) / Celeron D Processor 36x (Cedar Mill D0), 65nm");
   FMS (    0,15,  0, 6,  8,     "Intel Xeon Processor 71x0 (Tulsa B0), 65nm");
   FMQ (    0,15,  0, 6,     dd, "Intel Pentium D (Presler), 65nm");
   FMQ (    0,15,  0, 6,     dP, "Intel Pentium 4 (Cedar Mill) / Pentium Extreme Edition (Presler)");
   FMQ (    0,15,  0, 6,     dC, "Intel Celeron D (Cedar Mill), 65nm");
   FMQ (    0,15,  0, 6,     sX, "Intel Xeon (Dempsey / Tulsa), 65nm");
   FM  (    0,15,  0, 6,         "Intel Pentium 4 (Cedar Mill) / Pentium Extreme Edition (Presler) / Pentium D (Presler) / Xeon (Dempsey) / Xeon (Tulsa) / Celeron D (Cedar Mill), 65nm");
   FQ  (    0,15,            sM, "Intel Xeon MP (unknown model)");
   FQ  (    0,15,            sX, "Intel Xeon (unknown model)");
   FQ  (    0,15,            MC, "Intel Mobile Celeron (unknown model)");
   FQ  (    0,15,            MC, "Intel Mobile Pentium 4 (unknown model)");
   FQ  (    0,15,            MM, "Intel Mobile Pentium 4 Processor-M (unknown model)");
   FQ  (    0,15,            dC, "Intel Celeron (unknown model)");
   FQ  (    0,15,            dd, "Intel Pentium D (unknown model)");
   FQ  (    0,15,            dP, "Intel Pentium 4 (unknown model)");
   FQ  (    0,15,            dc, "Intel Pentium (unknown model)");
   F   (    0,15,                "Intel Pentium 4 / Pentium D / Xeon / Xeon MP / Celeron / Celeron D (unknown model)");
   FMS (    1,15,  0, 0,  7,     "Intel Itanium2 (McKinley B3), .18um");
   FM  (    1,15,  0, 0,         "Intel Itanium2 (McKinley), .18um");
   FMS (    1,15,  0, 1,  5,     "Intel Itanium2 (Madison/Deerfield/Hondo B1), .13um");
   FM  (    1,15,  0, 1,         "Intel Itanium2 (Madison/Deerfield/Hondo), .13um");
   FMS (    1,15,  0, 2,  1,     "Intel Itanium2 (Madison 9M/Fanwood A1), .13um");
   FMS (    1,15,  0, 2,  2,     "Intel Itanium2 (Madison 9M/Fanwood A2), .13um");
   FM  (    1,15,  0, 2,         "Intel Itanium2 (Madison), .13um");
   F   (    1,15,                "Intel Itanium2 (unknown model)");
   FMS (    2,15,  0, 0,  5,     "Intel Itanium2 Dual-Core Processor 9000 (Montecito/Millington C1), 90nm");
   FMS (    2,15,  0, 0,  7,     "Intel Itanium2 Dual-Core Processor 9000 (Montecito/Millington C2), 90nm");
   FM  (    2,15,  0, 0,         "Intel Itanium2 Dual-Core Processor 9000 (Montecito/Millington), 90nm");
   FMS (    2,15,  0, 1,  1,     "Intel Itanium2 Dual-Core Processor 9100 (Montvale A1), 90nm");
   FM  (    2,15,  0, 1,         "Intel Itanium2 Dual-Core Processor 9100 (Montvale), 90nm");
   FMS (    2,15,  0, 2,  4,     "Intel Itanium2 Dual-Core Processor 9300 (Tukwila E0), 90nm");
   FM  (    2,15,  0, 2,         "Intel Itanium2 Dual-Core Processor 9300 (Tukwila), 90nm");
   F   (    2,15,                "Intel Itanium2 (unknown model)");
   DEFAULT                      ("unknown");
}


/*
** AMD special cases
*/
static boolean is_amd_egypt_athens_8xx(const code_stash_t*  stash)
{
   /*
   ** This makes its determination based on the Processor model
   ** logic from:
   **    Revision Guide for AMD Athlon 64 and AMD Opteron Processors 
   **    (25759 Rev 3.79), Constructing the Processor Name String.
   ** See also decode_amd_model().
   */

   if (stash->vendor == VENDOR_AMD && stash->br.opteron) {
      switch (__FM(stash->val_1_eax)) {
      case _FM(0,15, 2,1): /* Italy/Egypt */
      case _FM(0,15, 2,5): /* Troy/Athens */
         {
            unsigned int  bti;

            if (__B(stash->val_1_ebx) != 0) {
               bti = BIT_EXTRACT_LE(__B(stash->val_1_ebx), 5, 8) << 2;
            } else if (BIT_EXTRACT_LE(stash->val_80000001_ebx, 0, 12) != 0) {
               bti = BIT_EXTRACT_LE(stash->val_80000001_ebx, 6, 12);
            } else {
               return FALSE;
            }

            switch (bti) {
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x2a:
            case 0x30:
            case 0x31:
            case 0x39:
            case 0x3c:
            case 0x32:
            case 0x33:
               /* It's a 2xx */
               return FALSE;
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x2b:
            case 0x34:
            case 0x35:
            case 0x3a:
            case 0x3d:
            case 0x36:
            case 0x37:
               /* It's an 8xx */
               return TRUE;
            }
         }
      }
   }

   return FALSE;
}


/* Embedded Opteron, distinguished from Opteron (Barcelona & Shanghai) */
#define EO (dO && stash->br.embedded)
/* Opterons, distinguished by number of processors */
#define DO (dO && stash->br.cores == 2)
#define SO (dO && stash->br.cores == 6)
/* Athlons, distinguished by number of processors */
#define DA (dA && stash->br.cores == 2)
#define TA (dA && stash->br.cores == 3)
#define QA (dA && stash->br.cores == 4)
/* Phenoms distinguished by number of processors */
#define Dp (dp && stash->br.cores == 2)
#define Tp (dp && stash->br.cores == 3)
#define Qp (dp && stash->br.cores == 4)
#define Sp (dp && stash->br.cores == 6)
/* Semprons, distinguished by number of processors */
#define DS (dS  && stash->br.cores == 2)
/* Egypt, distinguished from Italy; and
   Athens, distingushed from Troy */
#define d8 (dO && is_amd_egypt_athens_8xx(stash))
/* Thorton A2, distinguished from Barton A2 */
#define dt (dX && stash->L2_256K)
/* Manchester E6, distinguished from from Toledo E6 */
#define dm (dA && stash->L2_512K)
/* Propus, distinguished from Regor */
#define dR (dA && stash->L2_512K)
/* Trinidad, distinguished from Taylor */
#define Mt (MT && stash->L2_512K)



/*
** AMD major queries:
**
** d? = think "desktop"
** s? = think "server" (MP)
** M? = think "mobile"
**
** ?A = think Athlon
** ?X = think Athlon XP
** ?L = think Athlon XP (LV)
** ?F = think Athlon FX
** ?D = think Duron
** ?S = think Sempron
** ?O = think Opteron
** ?T = think Turion
** ?p = think Phenom
** ?V = think V-Series
** ?n = think Turion Neo
** ?N = think Neo
*/
#define dA (is_amd && !is_mobile && stash->br.athlon)
#define dX (is_amd && !is_mobile && stash->br.athlon_xp)
#define dF (is_amd && !is_mobile && stash->br.athlon_fx)
#define dD (is_amd && !is_mobile && stash->br.duron)
#define dS (is_amd && !is_mobile && stash->br.sempron)
#define dO (is_amd && !is_mobile && stash->br.opteron)
#define dp (is_amd && !is_mobile && stash->br.phenom)
#define sA (is_amd && !is_mobile && stash->br.athlon_mp)
#define sD (is_amd && !is_mobile && stash->br.duron_mp)
#define MA (is_amd && is_mobile && stash->br.athlon)
#define MX (is_amd && is_mobile && stash->br.athlon_xp)
#define ML (is_amd && is_mobile && stash->br.athlon_lv)
#define MD (is_amd && is_mobile && stash->br.duron)
#define MS (is_amd && is_mobile && stash->br.sempron)
#define Mp (is_amd && is_mobile && stash->br.phenom)
#define MV (is_amd && is_mobile && stash->br.v_series)
#define MG (is_amd && stash->br.geode)
#define MT (is_amd && stash->br.turion)
#define Mn (is_amd && stash->br.turion && stash->br.neo)
#define MN (is_amd && stash->br.neo)


char* SysDataCPU::get_amd_name(const char*          name,
                unsigned int         val,
                const code_stash_t*  stash)
{
    START;
   FM  (0, 4,  0, 3,         "AMD 80486DX2");
   FM  (0, 4,  0, 7,         "AMD 80486DX2WB");
   FM  (0, 4,  0, 8,         "AMD 80486DX4");
   FM  (0, 4,  0, 9,         "AMD 80486DX4WB");
   FM  (0, 4,  0,14,         "AMD 5x86");
   FM  (0, 4,  0,15,         "AMD 5xWB");
   F   (0, 4,                "AMD 80486 / 5x (unknown model)");
   FM  (0, 5,  0, 0,         "AMD SSA5 (PR75, PR90, PR100)");
   FM  (0, 5,  0, 1,         "AMD 5k86 (PR120, PR133)");
   FM  (0, 5,  0, 2,         "AMD 5k86 (PR166)");
   FM  (0, 5,  0, 3,         "AMD 5k86 (PR200)");
   FM  (0, 5,  0, 5,         "AMD Geode GX");
   FM  (0, 5,  0, 6,         "AMD K6, .30um");
   FM  (0, 5,  0, 7,         "AMD K6 (Little Foot), .25um");
   FMS (0, 5,  0, 8,  0,     "AMD K6-2 (Chomper A)");
   FMS (0, 5,  0, 8, 12,     "AMD K6-2 (Chomper A)");
   FM  (0, 5,  0, 8,         "AMD K6-2 (Chomper)");
   FMS (0, 5,  0, 9,  1,     "AMD K6-III (Sharptooth B)");
   FM  (0, 5,  0, 9,         "AMD K6-III (Sharptooth)");
   FM  (0, 5,  0,10,         "AMD Geode LX");
   FM  (0, 5,  0,13,         "AMD K6-2+, K6-III+");
   F   (0, 5,                "AMD 5k86 / K6 / Geode (unknown model)");
   FM  (0, 6,  0, 1,         "AMD Athlon, .25um");
   FM  (0, 6,  0, 2,         "AMD Athlon (K75 / Pluto / Orion), .18um");
   FMS (0, 6,  0, 3,  0,     "AMD Duron / mobile Duron (Spitfire A0)");
   FMS (0, 6,  0, 3,  1,     "AMD Duron / mobile Duron (Spitfire A2)");
   FM  (0, 6,  0, 3,         "AMD Duron / mobile Duron (Spitfire)");
   FMS (0, 6,  0, 4,  2,     "AMD Athlon (Thunderbird A4-A7)");
   FMS (0, 6,  0, 4,  4,     "AMD Athlon (Thunderbird A9)");
   FM  (0, 6,  0, 4,         "AMD Athlon (Thunderbird)");
   FMSQ(0, 6,  0, 6,  0, sA, "AMD Athlon MP (Palomino A0)");
   FMSQ(0, 6,  0, 6,  0, dA, "AMD Athlon (Palomino A0)");
   FMSQ(0, 6,  0, 6,  0, MA, "AMD mobile Athlon 4 (Palomino A0)");
   FMSQ(0, 6,  0, 6,  0, sD, "AMD Duron MP (Palomino A0)");
   FMSQ(0, 6,  0, 6,  0, MD, "AMD mobile Duron (Palomino A0)");
   FMS (0, 6,  0, 6,  0,     "AMD Athlon / Athlon MP mobile Athlon 4 / mobile Duron (Palomino A0)");
   FMSQ(0, 6,  0, 6,  1, sA, "AMD Athlon MP (Palomino A2)");
   FMSQ(0, 6,  0, 6,  1, dA, "AMD Athlon (Palomino A2)");
   FMSQ(0, 6,  0, 6,  1, MA, "AMD mobile Athlon 4 (Palomino A2)");
   FMSQ(0, 6,  0, 6,  1, sD, "AMD Duron MP (Palomino A2)");
   FMSQ(0, 6,  0, 6,  1, MD, "AMD mobile Duron (Palomino A2)");
   FMSQ(0, 6,  0, 6,  1, dD, "AMD Duron (Palomino A2)");
   FMS (0, 6,  0, 6,  1,     "AMD Athlon / Athlon MP / Duron / mobile Athlon / mobile Duron (Palomino A2)");
   FMSQ(0, 6,  0, 6,  2, sA, "AMD Athlon MP (Palomino A5)");
   FMSQ(0, 6,  0, 6,  2, dX, "AMD Athlon XP (Palomino A5)");
   FMSQ(0, 6,  0, 6,  2, MA, "AMD mobile Athlon 4 (Palomino A5)");
   FMSQ(0, 6,  0, 6,  2, sD, "AMD Duron MP (Palomino A5)");
   FMSQ(0, 6,  0, 6,  2, MD, "AMD mobile Duron (Palomino A5)");
   FMSQ(0, 6,  0, 6,  2, dD, "AMD Duron (Palomino A5)");
   FMS (0, 6,  0, 6,  2,     "AMD Athlon MP / Athlon XP / Duron / Duron MP / mobile Athlon / mobile Duron (Palomino A5)");
   FMQ (0, 6,  0, 6,     MD, "AMD mobile Duron (Palomino)");
   FMQ (0, 6,  0, 6,     dD, "AMD Duron (Palomino)");
   FMQ (0, 6,  0, 6,     MA, "AMD mobile Athlon (Palomino)");
   FMQ (0, 6,  0, 6,     dX, "AMD Athlon XP (Palomino)");
   FMQ (0, 6,  0, 6,     dA, "AMD Athlon (Palomino)");
   FM  (0, 6,  0, 6,         "AMD Athlon / Athlon MP / Athlon XP / Duron / Duron MP / mobile Athlon / mobile Duron (Palomino)");
   FMSQ(0, 6,  0, 7,  0, sD, "AMD Duron MP (Morgan A0)");
   FMSQ(0, 6,  0, 7,  0, MD, "AMD mobile Duron (Morgan A0)");
   FMSQ(0, 6,  0, 7,  0, dD, "AMD Duron (Morgan A0)");
   FMS (0, 6,  0, 7,  0,     "AMD Duron / Duron MP / mobile Duron (Morgan A0)");
   FMSQ(0, 6,  0, 7,  1, sD, "AMD Duron MP (Morgan A1)");
   FMSQ(0, 6,  0, 7,  1, MD, "AMD mobile Duron (Morgan A1)");
   FMSQ(0, 6,  0, 7,  1, dD, "AMD Duron (Morgan A1)");
   FMS (0, 6,  0, 7,  1,     "AMD Duron / Duron MP / mobile Duron (Morgan A1)");
   FMQ (0, 6,  0, 7,     sD, "AMD Duron MP (Morgan)");
   FMQ (0, 6,  0, 7,     MD, "AMD mobile Duron (Morgan)");
   FMQ (0, 6,  0, 7,     dD, "AMD Duron (Morgan)");
   FM  (0, 6,  0, 7,         "AMD Duron / Duron MP / mobile Duron (Morgan)");
   FMSQ(0, 6,  0, 8,  0, dS, "AMD Sempron (Thoroughbred A0)");
   FMSQ(0, 6,  0, 8,  0, sD, "AMD Duron MP (Applebred A0)");
   FMSQ(0, 6,  0, 8,  0, dD, "AMD Duron (Applebred A0)");
   FMSQ(0, 6,  0, 8,  0, MX, "AMD mobile Athlon XP (Thoroughbred A0)");
   FMSQ(0, 6,  0, 8,  0, sA, "AMD Athlon MP (Thoroughbred A0)");
   FMSQ(0, 6,  0, 8,  0, dX, "AMD Athlon XP (Thoroughbred A0)");
   FMSQ(0, 6,  0, 8,  0, dA, "AMD Athlon (Thoroughbred A0)");
   FMS (0, 6,  0, 8,  0,     "AMD Athlon / Athlon XP / Athlon MP / Sempron / Duron / Duron MP (Thoroughbred A0)");
   FMSQ(0, 6,  0, 8,  1, MG, "AMD Geode NX (Thoroughbred B0)");
   FMSQ(0, 6,  0, 8,  1, dS, "AMD Sempron (Thoroughbred B0)");
   FMSQ(0, 6,  0, 8,  1, sD, "AMD Duron MP (Thoroughbred B0)");
   FMSQ(0, 6,  0, 8,  1, dD, "AMD Duron (Thoroughbred B0)");
   FMSQ(0, 6,  0, 8,  1, sA, "AMD Athlon MP (Thoroughbred B0)");
   FMSQ(0, 6,  0, 8,  1, dX, "AMD Athlon XP (Thoroughbred B0)");
   FMSQ(0, 6,  0, 8,  1, dA, "AMD Athlon (Thoroughbred B0)");
   FMS (0, 6,  0, 8,  1,     "AMD Athlon / Athlon XP / Athlon MP / Sempron / Duron / Duron MP / Geode (Thoroughbred B0)");
   FMQ (0, 6,  0, 8,     MG, "AMD Geode NX (Thoroughbred)");
   FMQ (0, 6,  0, 8,     dS, "AMD Sempron (Thoroughbred)");
   FMQ (0, 6,  0, 8,     sD, "AMD Duron MP (Thoroughbred)");
   FMQ (0, 6,  0, 8,     dD, "AMD Duron (Thoroughbred)");
   FMQ (0, 6,  0, 8,     MX, "AMD mobile Athlon XP (Thoroughbred)");
   FMQ (0, 6,  0, 8,     sA, "AMD Athlon MP (Thoroughbred)");
   FMQ (0, 6,  0, 8,     dX, "AMD Athlon XP (Thoroughbred)");
   FMQ (0, 6,  0, 8,     dA, "AMD Athlon XP (Thoroughbred)");
   FM  (0, 6,  0, 8,         "AMD Athlon / Athlon XP / Athlon MP / Sempron / Duron / Duron MP / Geode (Thoroughbred)");
   FMSQ(0, 6,  0,10,  0, dS, "AMD Sempron (Barton A2)");
   FMSQ(0, 6,  0,10,  0, ML, "AMD mobile Athlon XP-M (LV) (Barton A2)");
   FMSQ(0, 6,  0,10,  0, MX, "AMD mobile Athlon XP-M (Barton A2)");
   FMSQ(0, 6,  0,10,  0, dt, "AMD Athlon XP (Thorton A2)");
   FMSQ(0, 6,  0,10,  0, sA, "AMD Athlon MP (Barton A2)");
   FMSQ(0, 6,  0,10,  0, dX, "AMD Athlon XP (Barton A2)");
   FMS (0, 6,  0,10,  0,     "AMD Athlon XP / Athlon MP / Sempron / mobile Athlon XP-M / mobile Athlon XP-M (LV) (Barton A2)");
   FMQ (0, 6,  0,10,     dS, "AMD Sempron (Barton)");
   FMQ (0, 6,  0,10,     ML, "AMD mobile Athlon XP-M (LV) (Barton)");
   FMQ (0, 6,  0,10,     MX, "AMD mobile Athlon XP-M (Barton)");
   FMQ (0, 6,  0,10,     sA, "AMD Athlon MP (Barton)");
   FMQ (0, 6,  0,10,     dX, "AMD Athlon XP (Barton)");
   FM  (0, 6,  0,10,         "AMD Athlon XP / Athlon MP / Sempron / mobile Athlon XP-M / mobile Athlon XP-M (LV) (Barton)");
   F   (0, 6,                "AMD Athlon / Athlon XP / Athlon MP / Duron / Duron MP / Sempron / mobile Athlon / mobile Athlon XP-M / mobile Athlon XP-M (LV) / mobile Duron (unknown model)");
   F   (0, 7,                "AMD Opteron (unknown model)");
   FMS (0,15,  0, 4,  0,     "AMD Athlon 64 (SledgeHammer SH7-B0), .13um");
   FMSQ(0,15,  0, 4,  8, MX, "AMD mobile Athlon XP-M (SledgeHammer SH7-C0), 754-pin, .13um");
   FMSQ(0,15,  0, 4,  8, MA, "AMD mobile Athlon 64 (SledgeHammer SH7-C0), 754-pin, .13um");
   FMSQ(0,15,  0, 4,  8, dA, "AMD Athlon 64 (SledgeHammer SH7-C0), 754-pin, .13um");
   FMS (0,15,  0, 4,  8,     "AMD Athlon 64 (SledgeHammer SH7-C0) / mobile Athlon 64 (SledgeHammer SH7-C0) / mobile Athlon XP-M (SledgeHammer SH7-C0), 754-pin, .13um");
   FMSQ(0,15,  0, 4, 10, MX, "AMD mobile Athlon XP-M (SledgeHammer SH7-CG), 940-pin, .13um");
   FMSQ(0,15,  0, 4, 10, MA, "AMD mobile Athlon 64 (SledgeHammer SH7-CG), 940-pin, .13um");
   FMSQ(0,15,  0, 4, 10, dA, "AMD Athlon 64 (SledgeHammer SH7-CG), 940-pin, .13um");
   FMS (0,15,  0, 4, 10,     "AMD Athlon 64 (SledgeHammer SH7-CG) / mobile Athlon 64 (SledgeHammer SH7-CG) / mobile Athlon XP-M (SledgeHammer SH7-CG), 940-pin, .13um");
   FMQ (0,15,  0, 4,     MX, "AMD mobile Athlon XP-M (SledgeHammer SH7), .13um");
   FMQ (0,15,  0, 4,     MA, "AMD mobile Athlon 64 (SledgeHammer SH7), .13um");
   FMQ (0,15,  0, 4,     dA, "AMD Athlon 64 (SledgeHammer SH7), .13um");
   FM  (0,15,  0, 4,         "AMD Athlon 64 (SledgeHammer SH7) / mobile Athlon 64 (SledgeHammer SH7) / mobile Athlon XP-M (SledgeHammer SH7), .13um");

   if(0);
   FMS (0,15,  0, 5,  0,     "AMD Opteron (DP SledgeHammer SH7-B0), 940-pin, .13um");
   FMS (0,15,  0, 5,  1,     "AMD Opteron (DP SledgeHammer SH7-B3), 940-pin, .13um");
   FMSQ(0,15,  0, 5,  8, dO, "AMD Opteron (DP SledgeHammer SH7-C0), 940-pin, .13um");
   FMSQ(0,15,  0, 5,  8, dF, "AMD Athlon 64 FX (DP SledgeHammer SH7-C0), 940-pin, .13um");
   FMS (0,15,  0, 5,  8,     "AMD Opteron (DP SledgeHammer SH7-C0) / Athlon 64 FX (DP SledgeHammer SH7-C0), 940-pin, .13um");
   FMSQ(0,15,  0, 5, 10, dO, "AMD Opteron (DP SledgeHammer SH7-CG), 940-pin, .13um");
   FMSQ(0,15,  0, 5, 10, dF, "AMD Athlon 64 FX (DP SledgeHammer SH7-CG), 940-pin, .13um");
   FMS (0,15,  0, 5, 10,     "AMD Opteron (DP SledgeHammer SH7-CG) / Athlon 64 FX (DP SledgeHammer SH7-CG), 940-pin, .13um");
   FMQ (0,15,  0, 5,     dO, "AMD Opteron (SledgeHammer SH7), 940-pin, .13um");
   FMQ (0,15,  0, 5,     dF, "AMD Athlon 64 FX (SledgeHammer SH7), 940-pin, .13um");
   FM  (0,15,  0, 5,         "AMD Opteron (SledgeHammer SH7) / Athlon 64 (SledgeHammer SH7) FX, 940-pin, .13um");
   FMSQ(0,15,  0, 7, 10, dF, "AMD Athlon 64 FX (DP SledgeHammer SH7-CG), 939-pin, .13um");
   FMSQ(0,15,  0, 7, 10, dA, "AMD Athlon 64 (DP SledgeHammer SH7-CG), 939-pin, .13um");
   FMS (0,15,  0, 7, 10,     "AMD Athlon 64 (DP SledgeHammer SH7-CG) / Athlon 64 FX (DP SledgeHammer SH7-CG), 939-pin, .13um");
   FMQ (0,15,  0, 7,     dF, "AMD Athlon 64 FX (DP SledgeHammer SH7), 939-pin, .13um");
   FMQ (0,15,  0, 7,     dA, "AMD Athlon 64 (DP SledgeHammer SH7), 939-pin, .13um");
   FM  (0,15,  0, 7,         "AMD Athlon 64 (DP SledgeHammer SH7) / Athlon 64 FX (DP SledgeHammer SH7), 939-pin, .13um");
   FMSQ(0,15,  0, 8,  2, MS, "AMD mobile Sempron (ClawHammer CH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0, 8,  2, MX, "AMD mobile Athlon XP-M (ClawHammer CH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0, 8,  2, MA, "AMD mobile Athlon 64 (Odessa CH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0, 8,  2, dA, "AMD Athlon 64 (ClawHammer CH7-CG), 754-pin, .13um");
   FMS (0,15,  0, 8,  2,     "AMD Athlon 64 (ClawHammer CH7-CG) / mobile Athlon 64 (Odessa CH7-CG) / mobile Sempron (ClawHammer CH7-CG) / mobile Athlon XP-M (ClawHammer CH7-CG), 754-pin, .13um");
   FMQ (0,15,  0, 8,     MS, "AMD mobile Sempron (Odessa CH7), 754-pin, .13um");
   FMQ (0,15,  0, 8,     MX, "AMD mobile Athlon XP-M (Odessa CH7), 754-pin, .13um");
   FMQ (0,15,  0, 8,     MA, "AMD mobile Athlon 64 (Odessa CH7), 754-pin, .13um");
   FMQ (0,15,  0, 8,     dA, "AMD Athlon 64 (ClawHammer CH7), 754-pin, .13um");
   FM  (0,15,  0, 8,         "AMD Athlon 64 (ClawHammer CH7) / mobile Athlon 64 (Odessa CH7) / mobile Sempron (Odessa CH7) / mobile Athlon XP-M (Odessa CH7), 754-pin, .13um");
   FMS (0,15,  0,11,  2,     "AMD Athlon 64 (ClawHammer CH7-CG), 939-pin, .13um");
   FM  (0,15,  0,11,         "AMD Athlon 64 (ClawHammer CH7), 939-pin, .13um");
   FMSQ(0,15,  0,12,  0, MS, "AMD mobile Sempron (Dublin DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,12,  0, dS, "AMD Sempron (Paris DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,12,  0, MX, "AMD mobile Athlon XP-M (ClawHammer/Odessa DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,12,  0, MA, "AMD mobile Athlon 64 (ClawHammer/Odessa DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,12,  0, dA, "AMD Athlon 64 (NewCastle DH7-CG), 754-pin, .13um");
   FMS (0,15,  0,12,  0,     "AMD Athlon 64 (NewCastle DH7-CG) / Sempron (Paris DH7-CG) / mobile Athlon 64 (ClawHammer/Odessa DH7-CG) / mobile Sempron (Dublin DH7-CG) / mobile Athlon XP-M (ClawHammer/Odessa DH7-CG), 754-pin, .13um");
   FMQ (0,15,  0,12,     MS, "AMD mobile Sempron (Dublin DH7), 754-pin, .13um");
   FMQ (0,15,  0,12,     dS, "AMD Sempron (Paris DH7), 754-pin, .13um");
   FMQ (0,15,  0,12,     MX, "AMD mobile Athlon XP-M (NewCastle DH7), 754-pin, .13um");
   FMQ (0,15,  0,12,     MA, "AMD mobile Athlon 64 (ClawHammer/Odessa DH7), 754-pin, .13um");
   FMQ (0,15,  0,12,     dA, "AMD Athlon 64 (NewCastle DH7), 754-pin, .13um");
   FM  (0,15,  0,12,         "AMD Athlon 64 (NewCastle DH7) / Sempron (Paris DH7) / mobile Athlon 64 (ClawHammer/Odessa DH7) / mobile Sempron (Dublin DH7) / mobile Athlon XP-M (ClawHammer/Odessa DH7), 754-pin, .13um");
   FMSQ(0,15,  0,14,  0, MS, "AMD mobile Sempron (Dublin DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,14,  0, dS, "AMD Sempron (Paris DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,14,  0, MX, "AMD mobile Athlon XP-M (ClawHammer/Odessa DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,14,  0, MA, "AMD mobile Athlon 64 (ClawHammer/Odessa DH7-CG), 754-pin, .13um");
   FMSQ(0,15,  0,14,  0, dA, "AMD Athlon 64 (NewCastle DH7-CG), 754-pin, .13um");
   FMS (0,15,  0,14,  0,     "AMD Athlon 64 (NewCastle DH7-CG) / Sempron (Paris DH7-CG) / mobile Athlon 64 (ClawHammer/Odessa DH7-CG) / mobile Sempron (Dublin DH7-CG) / mobile Athlon XP-M (ClawHammer/Odessa DH7-CG), 754-pin, .13um");
   FMQ (0,15,  0,14,     dS, "AMD Sempron (Paris DH7), 754-pin, .13um");
   FMQ (0,15,  0,14,     MS, "AMD mobile Sempron (Dublin DH7), 754-pin, .13um");
   FMQ (0,15,  0,14,     MX, "AMD mobile Athlon XP-M (ClawHammer/Odessa DH7), 754-pin, .13um");
   FMQ (0,15,  0,14,     MA, "AMD mobile Athlon 64 (ClawHammer/Odessa DH7), 754-pin, .13um");
   FMQ (0,15,  0,14,     dA, "AMD Athlon 64 (NewCastle DH7), 754-pin, .13um");
   FM  (0,15,  0,14,         "AMD Athlon 64 (NewCastle DH7) / Sempron (Paris DH7) / mobile Athlon 64 (ClawHammer/Odessa DH7) / mobile Sempron (Dublin DH7) / mobile Athlon XP-M (ClawHammer/Odessa DH7), 754-pin, .13um");
   FMSQ(0,15,  0,15,  0, dS, "AMD Sempron (Paris DH7-CG), 939-pin, .13um");
   FMSQ(0,15,  0,15,  0, MA, "AMD mobile Athlon 64 (ClawHammer/Odessa DH7-CG), 939-pin, .13um");
   FMSQ(0,15,  0,15,  0, dA, "AMD Athlon 64 (NewCastle DH7-CG), 939-pin, .13um");
   FMS (0,15,  0,15,  0,     "AMD Athlon 64 (NewCastle DH7-CG) / Sempron (Paris DH7-CG) / mobile Athlon 64 (ClawHammer/Odessa DH7-CG), 939-pin, .13um");
   FMQ (0,15,  0,15,     dS, "AMD Sempron (Paris DH7), 939-pin, .13um");
   FMQ (0,15,  0,15,     MA, "AMD mobile Athlon 64 (ClawHammer/Odessa DH7), 939-pin, .13um");
   FMQ (0,15,  0,15,     dA, "AMD Athlon 64 (NewCastle DH7), 939-pin, .13um");
   FM  (0,15,  0,15,         "AMD Athlon 64 (NewCastle DH7) / Sempron (Paris DH7) / mobile Athlon 64 (ClawHammer/Odessa DH7), 939-pin, .13um");
   FMSQ(0,15,  1, 4,  0, MX, "AMD mobile Athlon XP-M (Oakville SH7-D0), 754-pin, 90nm");
   FMSQ(0,15,  1, 4,  0, MA, "AMD mobile Athlon 64 (Oakville SH7-D0), 754-pin, 90nm");
   FMSQ(0,15,  1, 4,  0, dA, "AMD Athlon 64 (Winchester SH7-D0), 754-pin, 90nm");
   FMS (0,15,  1, 4,  0,     "AMD Athlon 64 (Winchester SH7-D0) / mobile Athlon 64 (Oakville SH7-D0) / mobile Athlon XP-M (Oakville SH7-D0), 754-pin, 90nm");
   FMQ (0,15,  1, 4,     MX, "AMD mobile Athlon XP-M (Winchester SH7), 754-pin, 90nm");
   FMQ (0,15,  1, 4,     MA, "AMD mobile Athlon 64 (Winchester SH7), 754-pin, 90nm");
   FMQ (0,15,  1, 4,     dA, "AMD Athlon 64 (Winchester SH7), 754-pin, 90nm");
   FM  (0,15,  1, 4,         "AMD Athlon 64 (Winchester SH7) / mobile Athlon 64 (Winchester SH7) / mobile Athlon XP-M (Winchester SH7), 754-pin, 90nm");
   FMSQ(0,15,  1, 5,  0, dO, "AMD Opteron (Winchester SH7-D0), 940-pin, 90nm");
   FMSQ(0,15,  1, 5,  0, dF, "AMD Athlon 64 FX (Winchester SH7-D0), 940-pin, 90nm");
   FMS (0,15,  1, 5,  0,     "AMD Opteron (Winchester SH7-D0) / Athlon 64 FX (Winchester SH7-D0), 940-pin, 90nm");
   FMQ (0,15,  1, 5,     dO, "AMD Opteron (Winchester SH7), 940-pin, 90nm");
   FMQ (0,15,  1, 5,     dF, "AMD Athlon 64 FX (Winchester SH7), 940-pin, 90nm");
   FM  (0,15,  1, 5,         "AMD Opteron (Winchester SH7) / Athlon 64 FX (Winchester SH7), 940-pin, 90nm");
   FMSQ(0,15,  1, 7,  0, dF, "AMD Athlon 64 FX (Winchester SH7-D0), 939-pin, 90nm");
   FMSQ(0,15,  1, 7,  0, dA, "AMD Athlon 64 (Winchester SH7-D0), 939-pin, 90nm");
   FMS (0,15,  1, 7,  0,     "AMD Athlon 64 (Winchester SH7-D0) / Athlon 64 FX (Winchester SH7-D0), 939-pin, 90nm");
   FMQ (0,15,  1, 7,     dF, "AMD Athlon 64 FX (Winchester SH7), 939-pin, 90nm");
   FMQ (0,15,  1, 7,     dA, "AMD Athlon 64 (Winchester SH7), 939-pin, 90nm");
   FM  (0,15,  1, 7,         "AMD Athlon 64 (Winchester SH7) / Athlon 64 FX (Winchester SH7), 939-pin, 90nm");
   FMSQ(0,15,  1, 8,  0, MS, "AMD mobile Sempron (Georgetown/Sonora CH-D0), 754-pin, 90nm");
   FMSQ(0,15,  1, 8,  0, MX, "AMD mobile Athlon XP-M (Oakville CH-D0), 754-pin, 90nm");
   FMSQ(0,15,  1, 8,  0, MA, "AMD mobile Athlon 64 (Oakville CH-D0), 754-pin, 90nm");
   FMSQ(0,15,  1, 8,  0, dA, "AMD Athlon 64 (Winchester CH-D0), 754-pin, 90nm");
   FMS (0,15,  1, 8,  0,     "AMD Athlon 64 (Winchester CH-D0) / mobile Athlon 64 (Oakville CH-D0) / mobile Sempron (Georgetown/Sonora CH-D0) / mobile Athlon XP-M (Oakville CH-D0), 754-pin, 90nm");
   FMQ (0,15,  1, 8,     MS, "AMD mobile Sempron (Georgetown/Sonora CH), 754-pin, 90nm");
   FMQ (0,15,  1, 8,     MX, "AMD mobile Athlon XP-M (Winchester CH), 754-pin, 90nm");
   FMQ (0,15,  1, 8,     MA, "AMD mobile Athlon 64 (Winchester CH), 754-pin, 90nm");
   FMQ (0,15,  1, 8,     dA, "AMD Athlon 64 (Winchester CH), 754-pin, 90nm");
   FM  (0,15,  1, 8,         "AMD Athlon 64 (Winchester CH) / mobile Athlon 64 (Winchester CH) / mobile Sempron (Georgetown/Sonora CH) / mobile Athlon XP-M (Winchester CH), 754-pin, 90nm");
   FMS (0,15,  1,11,  0,     "AMD Athlon 64 (Winchester CH-D0), 939-pin, 90nm");
   FM  (0,15,  1,11,         "AMD Athlon 64 (Winchester CH), 939-pin, 90nm");
   FMSQ(0,15,  1,12,  0, MS, "AMD mobile Sempron (Georgetown/Sonora DH8-D0), 754-pin, 90nm");
   FMSQ(0,15,  1,12,  0, dS, "AMD Sempron (Palermo DH8-D0), 754-pin, 90nm");
   FMSQ(0,15,  1,12,  0, MX, "AMD Athlon XP-M (Winchester DH8-D0), 754-pin, 90nm");
   FMSQ(0,15,  1,12,  0, MA, "AMD mobile Athlon 64 (Oakville DH8-D0), 754-pin, 90nm");
   FMSQ(0,15,  1,12,  0, dA, "AMD Athlon 64 (Winchester DH8-D0), 754-pin, 90nm");
   FMS (0,15,  1,12,  0,     "AMD Athlon 64 (Winchester DH8-D0) / Sempron (Palermo DH8-D0) / mobile Athlon 64 (Oakville DH8-D0) / mobile Sempron (Georgetown/Sonora DH8-D0) / mobile Athlon XP-M (Winchester DH8-D0), 754-pin, 90nm");
   FMQ (0,15,  1,12,     MS, "AMD mobile Sempron (Georgetown/Sonora DH8), 754-pin, 90nm");
   FMQ (0,15,  1,12,     dS, "AMD Sempron (Palermo DH8), 754-pin, 90nm");
   FMQ (0,15,  1,12,     MX, "AMD Athlon XP-M (Winchester DH8), 754-pin, 90nm");
   FMQ (0,15,  1,12,     MA, "AMD mobile Athlon 64 (Winchester DH8), 754-pin, 90nm");
   FMQ (0,15,  1,12,     dA, "AMD Athlon 64 (Winchester DH8), 754-pin, 90nm");
   FM  (0,15,  1,12,         "AMD Athlon 64 (Winchester DH8) / Sempron (Palermo DH8) / mobile Athlon 64 (Winchester DH8) / mobile Sempron (Georgetown/Sonora DH8) / mobile Athlon XP-M (Winchester DH8), 754-pin, 90nm");
   FMSQ(0,15,  1,15,  0, dS, "AMD Sempron (Palermo DH8-D0), 939-pin, 90nm");
   FMSQ(0,15,  1,15,  0, dA, "AMD Athlon 64 (Winchester DH8-D0), 939-pin, 90nm");
   FMS (0,15,  1,15,  0,     "AMD Athlon 64 (Winchester DH8-D0) / Sempron (Palermo DH8-D0), 939-pin, 90nm");
   FMQ (0,15,  1,15,     dS, "AMD Sempron (Palermo DH8), 939-pin, 90nm");
   FMQ (0,15,  1,15,     dA, "AMD Athlon 64 (Winchester DH8), 939-pin, 90nm");
   FM  (0,15,  1,15,         "AMD Athlon 64 (Winchester DH8) / Sempron (Palermo DH8), 939-pin, 90nm");
   FMSQ(0,15,  2, 1,  0, d8, "AMD Dual Core Opteron (Egypt JH-E1), 940-pin, 90nm");
   FMSQ(0,15,  2, 1,  0, dO, "AMD Dual Core Opteron (Italy JH-E1), 940-pin, 90nm");
   FMS (0,15,  2, 1,  0,     "AMD Dual Core Opteron (Italy/Egypt JH-E1), 940-pin, 90nm");
   FMSQ(0,15,  2, 1,  2, d8, "AMD Dual Core Opteron (Egypt JH-E6), 940-pin, 90nm");
   FMSQ(0,15,  2, 1,  2, dO, "AMD Dual Core Opteron (Italy JH-E6), 940-pin, 90nm");
   FMS (0,15,  2, 1,  2,     "AMD Dual Core Opteron (Italy/Egypt JH-E6), 940-pin, 90nm");
   FMQ (0,15,  2, 1,     d8, "AMD Dual Core Opteron (Egypt JH), 940-pin, 90nm");
   FMQ (0,15,  2, 1,     dO, "AMD Dual Core Opteron (Italy JH), 940-pin, 90nm");
   FM  (0,15,  2, 1,         "AMD Dual Core Opteron (Italy/Egypt JH), 940-pin, 90nm");
   FMSQ(0,15,  2, 3,  2, DO, "AMD Dual Core Opteron (Denmark JH-E6), 939-pin, 90nm");
   FMSQ(0,15,  2, 3,  2, dF, "AMD Athlon 64 FX (Toledo JH-E6), 939-pin, 90nm");
   if(0);
   FMSQ(0,15,  2, 3,  2, dm, "AMD Athlon 64 X2 (Manchester JH-E6), 939-pin, 90nm");
   FMSQ(0,15,  2, 3,  2, dA, "AMD Athlon 64 X2 (Toledo JH-E6), 939-pin, 90nm");
   FMS (0,15,  2, 3,  2,     "AMD Dual Core Opteron (Denmark JH-E6) / Athlon 64 X2 (Toledo JH-E6) / Athlon 64 FX (Toledo JH-E6), 939-pin, 90nm");
   FMQ (0,15,  2, 3,     dO, "AMD Dual Core Opteron (Denmark JH), 939-pin, 90nm");
   FMQ (0,15,  2, 3,     dF, "AMD Athlon 64 FX (Toledo JH), 939-pin, 90nm");
   FMQ (0,15,  2, 3,     dm, "AMD Athlon 64 X2 (Manchester JH), 939-pin, 90nm");
   FMQ (0,15,  2, 3,     dA, "AMD Athlon 64 X2 (Toledo JH), 939-pin, 90nm");
   FM  (0,15,  2, 3,         "AMD Dual Core Opteron (Denmark JH) / Athlon 64 X2 (Toledo JH / Manchester JH) / Athlon 64 FX (Toledo JH), 939-pin, 90nm");
   FMSQ(0,15,  2, 4,  2, MA, "AMD mobile Athlon 64 (Newark SH-E5), 754-pin, 90nm");
   FMSQ(0,15,  2, 4,  2, MT, "AMD mobile Turion (Lancaster/Richmond SH-E5), 754-pin, 90nm");
   FMS (0,15,  2, 4,  2,     "AMD mobile Athlon 64 (Newark SH-E5) / mobile Turion (Lancaster/Richmond SH-E5), 754-pin, 90nm");
   FMQ (0,15,  2, 4,     MA, "AMD mobile Athlon 64 (Newark SH), 754-pin, 90nm");
   FMQ (0,15,  2, 4,     MT, "AMD mobile Turion (Lancaster/Richmond SH), 754-pin, 90nm");
   FM  (0,15,  2, 4,         "AMD mobile Athlon 64 (Newark SH) / mobile Turion (Lancaster/Richmond SH), 754-pin, 90nm");
   FMQ (0,15,  2, 5,     d8, "AMD Opteron (Athens SH-E4), 940-pin, 90nm");
   FMQ (0,15,  2, 5,     dO, "AMD Opteron (Troy SH-E4), 940-pin, 90nm");
   FM  (0,15,  2, 5,         "AMD Opteron (Troy/Athens SH-E4), 940-pin, 90nm");
   FMSQ(0,15,  2, 7,  1, dO, "AMD Opteron (Venus SH-E4), 939-pin, 90nm");
   FMSQ(0,15,  2, 7,  1, dF, "AMD Athlon 64 FX (San Diego SH-E4), 939-pin, 90nm");
   FMSQ(0,15,  2, 7,  1, dA, "AMD Athlon 64 (San Diego SH-E4), 939-pin, 90nm");
   FMS (0,15,  2, 7,  1,     "AMD Opteron (Venus SH-E4) / Athlon 64 (San Diego SH-E4) / Athlon 64 FX (San Diego SH-E4), 939-pin, 90nm");
   FMQ (0,15,  2, 7,     dO, "AMD Opteron (San Diego SH), 939-pin, 90nm");
   FMQ (0,15,  2, 7,     dF, "AMD Athlon 64 FX (San Diego SH), 939-pin, 90nm");
   FMQ (0,15,  2, 7,     dA, "AMD Athlon 64 (San Diego SH), 939-pin, 90nm");
   FM  (0,15,  2, 7,         "AMD Opteron (San Diego SH) / Athlon 64 (San Diego SH) / Athlon 64 FX (San Diego SH), 939-pin, 90nm");
   FM  (0,15,  2,11,         "AMD Athlon 64 X2 (Manchester BH-E4), 939-pin, 90nm");
   FMS (0,15,  2,12,  0,     "AMD Sempron (Palermo DH-E3), 754-pin, 90nm");
   FMSQ(0,15,  2,12,  2, MS, "AMD mobile Sempron (Albany/Roma DH-E6), 754-pin, 90nm");
   FMSQ(0,15,  2,12,  2, dS, "AMD Sempron (Palermo DH-E6), 754-pin, 90nm");
   FMS (0,15,  2,12,  2,     "AMD Sempron (Palermo DH-E6) / mobile Sempron (Albany/Roma DH-E6), 754-pin, 90nm");
   FMQ (0,15,  2,12,     MS, "AMD mobile Sempron (Albany/Roma DH), 754-pin, 90nm");
   FMQ (0,15,  2,12,     dS, "AMD Sempron (Palermo DH), 754-pin, 90nm");
   FM  (0,15,  2,12,         "AMD Sempron (Palermo DH) / mobile Sempron (Albany/Roma DH), 754-pin, 90nm");
   FMSQ(0,15,  2,15,  0, dS, "AMD Sempron (Palermo DH-E3), 939-pin, 90nm");
   FMSQ(0,15,  2,15,  0, dA, "AMD Athlon 64 (Venice DH-E3), 939-pin, 90nm");
   FMS (0,15,  2,15,  0,     "AMD Athlon 64 (Venice DH-E3) / Sempron (Palermo DH-E3), 939-pin, 90nm");
   FMSQ(0,15,  2,15,  2, dS, "AMD Sempron (Palermo DH-E6), 939-pin, 90nm");
   FMSQ(0,15,  2,15,  2, dA, "AMD Athlon 64 (Venice DH-E6), 939-pin, 90nm");
   FMS (0,15,  2,15,  2,     "AMD Athlon 64 (Venice DH-E6) / Sempron (Palermo DH-E6), 939-pin, 90nm");
   FMQ (0,15,  2,15,     dS, "AMD Sempron (Palermo DH), 939-pin, 90nm");
   FMQ (0,15,  2,15,     dA, "AMD Athlon 64 (Venice DH), 939-pin, 90nm");
   FM  (0,15,  2,15,         "AMD Athlon 64 (Venice DH) / Sempron (Palermo DH), 939-pin, 90nm");
   FMS (0,15,  4, 1,  2,     "AMD Dual-Core Opteron (Santa Rosa JH-F2), 90nm");
   FMS (0,15,  4, 1,  3,     "AMD Dual-Core Opteron (Santa Rosa JH-F3), 90nm");
   FM  (0,15,  4, 1,         "AMD Dual-Core Opteron (Santa Rosa), 90nm");
   FMSQ(0,15,  4, 3,  2, DO, "AMD Dual-Core Opteron (Santa Rosa JH-F2), 90nm");
   FMSQ(0,15,  4, 3,  2, dO, "AMD Opteron (Santa Rosa JH-F2), 90nm");
   FMSQ(0,15,  4, 3,  2, dF, "AMD Athlon 64 FX Dual-Core (Windsor JH-F2), 90nm");
   FMSQ(0,15,  4, 3,  2, dA, "AMD Athlon 64 X2 Dual-Core (Windsor JH-F2), 90nm");
   FMS (0,15,  4, 3,  2,     "AMD Opteron (Santa Rosa JH-F2) / Athlon 64 X2 Dual-Core (Windsor JH-F2) / Athlon 64 FX Dual-Core (Windsor JH-F2), 90nm");
   FMSQ(0,15,  4, 3,  3, DO, "AMD Dual-Core Opteron (Santa Rosa JH-F3), 90nm");
   FMSQ(0,15,  4, 3,  3, dO, "AMD Opteron (Santa Rosa JH-F3), 90nm");
   FMSQ(0,15,  4, 3,  3, dF, "AMD Athlon 64 FX Dual-Core (Windsor JH-F3), 90nm");
   FMSQ(0,15,  4, 3,  3, dA, "AMD Athlon 64 X2 Dual-Core (Windsor JH-F3), 90nm");
   FMS (0,15,  4, 3,  3,     "AMD Opteron (Santa Rosa JH-F3) / Athlon 64 X2 Dual-Core (Windsor JH-F3) / Athlon 64 FX Dual-Core (Windsor JH-F3), 90nm");
   FMQ (0,15,  4, 3,     DO, "AMD Dual-Core Opteron (Santa Rosa), 90nm");
   FMQ (0,15,  4, 3,     dO, "AMD Opteron (Santa Rosa), 90nm");
   FMQ (0,15,  4, 3,     dF, "AMD Athlon 64 FX Dual-Core (Windsor), 90nm");
   FMQ (0,15,  4, 3,     dA, "AMD Athlon 64 X2 Dual-Core (Windsor), 90nm");
   FM  (0,15,  4, 3,         "AMD Opteron (Santa Rosa) / Athlon 64 X2 Dual-Core (Windsor) / Athlon 64 FX Dual-Core (Windsor), 90nm");
   FMSQ(0,15,  4, 8,  2, dA, "AMD Athlon 64 X2 Dual-Core (Windsor BH-F2), 90nm");
   FMSQ(0,15,  4, 8,  2, Mt, "AMD Turion 64 X2 (Trinidad BH-F2), 90nm");
   FMSQ(0,15,  4, 8,  2, MT, "AMD Turion 64 X2 (Taylor BH-F2), 90nm");
   FMS (0,15,  4, 8,  2,     "AMD Athlon 64 X2 Dual-Core (Windsor BH-F2) / Turion 64 X2 (Taylor / Trinidad BH-F2), 90nm");
   FMQ (0,15,  4, 8,     dA, "AMD Athlon 64 X2 Dual-Core (Windsor), 90nm");
   FMQ (0,15,  4, 8,     Mt, "AMD Turion 64 X2 (Trinidad), 90nm");
   FMQ (0,15,  4, 8,     MT, "AMD Turion 64 X2 (Taylor), 90nm");
   FM  (0,15,  4, 8,         "AMD Athlon 64 X2 Dual-Core (Windsor) / Turion 64 X2 (Taylor / Trinidad), 90nm");
   FMS (0,15,  4,11,  2,     "AMD Athlon 64 X2 Dual-Core (Windsor BH-F2), 90nm");
   FM  (0,15,  4,11,         "AMD Athlon 64 X2 Dual-Core (Windsor), 90nm");
   FMSQ(0,15,  4,12,  2, MS, "AMD mobile Sempron (Keene BH-F2), 90nm");
   FMSQ(0,15,  4,12,  2, dS, "AMD Sempron (Manila BH-F2), 90nm");
   FMSQ(0,15,  4,12,  2, Mt, "AMD Turion (Trinidad BH-F2), 90nm");
   FMSQ(0,15,  4,12,  2, MT, "AMD Turion (Taylor BH-F2), 90nm");
   FMSQ(0,15,  4,12,  2, dA, "AMD Athlon 64 (Orleans BH-F2), 90nm"); 
   FMS (0,15,  4,12,  2,     "AMD Athlon 64 (Orleans BH-F2) / Sempron (Manila BH-F2) / mobile Sempron (Keene BH-F2) / Turion (Taylor/Trinidad BH-F2), 90nm");
   FMQ (0,15,  4,12,     MS, "AMD mobile Sempron (Keene), 90nm");
   FMQ (0,15,  4,12,     dS, "AMD Sempron (Manila), 90nm");
   FMQ (0,15,  4,12,     Mt, "AMD Turion (Trinidad), 90nm");
   FMQ (0,15,  4,12,     MT, "AMD Turion (Taylor), 90nm");
   FMQ (0,15,  4,12,     dA, "AMD Athlon 64 (Orleans), 90nm"); 
   FM  (0,15,  4,12,         "AMD Athlon 64 (Orleans) / Sempron (Manila) / mobile Sempron (Keene) / Turion (Taylor/Trinidad), 90nm");
   FMSQ(0,15,  4,15,  2, MS, "AMD mobile Sempron (Keene DH-F2), 90nm");
   FMSQ(0,15,  4,15,  2, dS, "AMD Sempron (Manila DH-F2), 90nm");
   FMSQ(0,15,  4,15,  2, dA, "AMD Athlon 64 (Orleans DH-F2), 90nm");
   FMS (0,15,  4,15,  2,     "AMD Athlon 64 (Orleans DH-F2) / Sempron (Manila DH-F2) / mobile Sempron (Keene DH-F2), 90nm");
   FMQ (0,15,  4,15,     MS, "AMD mobile Sempron (Keene), 90nm");
   FMQ (0,15,  4,15,     dS, "AMD Sempron (Manila), 90nm");
   FMQ (0,15,  4,15,     dA, "AMD Athlon 64 (Orleans), 90nm");
   FM  (0,15,  4,15,         "AMD Athlon 64 (Orleans) / Sempron (Manila) / mobile Sempron (Keene), 90nm");
   FMS (0,15,  5,13,  3,     "AMD Opteron (Santa Rosa JH-F3), 90nm");
   FM  (0,15,  5,13,         "AMD Opteron (Santa Rosa), 90nm");
   FMSQ(0,15,  5,15,  2, MS, "AMD mobile Sempron (Keene DH-F2), 90nm");
   FMSQ(0,15,  5,15,  2, dS, "AMD Sempron (Manila DH-F2), 90nm");
   FMSQ(0,15,  5,15,  2, dA, "AMD Athlon 64 (Orleans DH-F2), 90nm");
   FMS (0,15,  5,15,  2,     "AMD Athlon 64 (Orleans DH-F2) / Sempron (Manila DH-F2) / mobile Sempron (Keene DH-F2), 90nm");
   FMS (0,15,  5,15,  3,     "AMD Athlon 64 (Orleans DH-F3), 90nm");
   FMQ (0,15,  5,15,     MS, "AMD mobile Sempron (Keene), 90nm");
   FMQ (0,15,  5,15,     dS, "AMD Sempron (Manila), 90nm");
   FMQ (0,15,  5,15,     dA, "AMD Athlon 64 (Orleans), 90nm");
   FM  (0,15,  5,15,         "AMD Athlon 64 (Orleans) / Sempron (Manila) / mobile Sempron (Keene), 90nm");
   FM  (0,15,  5,15,         "AMD Athlon 64 (Orleans), 90nm");
   FMS (0,15,  6, 8,  1,     "AMD Turion 64 X2 (Tyler BH-G1), 65nm");
   FMSQ(0,15,  6, 8,  2, MT, "AMD Turion 64 X2 (Tyler BH-G2), 65nm");
   FMSQ(0,15,  6, 8,  2, dS, "AMD Sempron Dual-Core (Tyler BH-G2), 65nm");
   FMS (0,15,  6, 8,  2,     "AMD AMD Turion 64 X2 (Tyler BH-G2) / Sempron Dual-Core (Tyler BH-G2), 65nm");
   FMQ (0,15,  6, 8,     MT, "AMD Turion 64 X2 (Tyler), 65nm");
   FMQ (0,15,  6, 8,     dS, "AMD Sempron Dual-Core (Tyler), 65nm");
   FM  (0,15,  6, 8,         "AMD AMD Turion 64 X2 (Tyler) / Sempron Dual-Core (Tyler), 65nm");
   FMSQ(0,15,  6,11,  1, dS, "AMD Sempron Dual-Core (Sparta BH-G1), 65nm");
   FMSQ(0,15,  6,11,  1, dA, "AMD Athlon 64 X2 Dual-Core (Brisbane BH-G1), 65nm");
   FMS (0,15,  6,11,  1,     "AMD Athlon 64 X2 Dual-Core (Brisbane BH-G1) / Sempron Dual-Core (Sparta BH-G1), 65nm");
   FMSQ(0,15,  6,11,  2, dA, "AMD Athlon 64 X2 Dual-Core (Brisbane BH-G2), 65nm");
   FMSQ(0,15,  6,11,  2, Mn, "AMD Turion Neo X2 Dual-Core (Huron BH-G2), 65nm");
   FMSQ(0,15,  6,11,  2, MN, "AMD Athlon Neo X2 (Huron BH-G2), 65nm");
   FMS (0,15,  6,11,  2,     "AMD Athlon 64 X2 Dual-Core (Brisbane BH-G2) / Athlon Neo X2 (Huron BH-G2), 65nm");
   FMQ (0,15,  6,11,     dS, "AMD Sempron Dual-Core (Sparta), 65nm");
   FMQ (0,15,  6,11,     Mn, "AMD Turion Neo X2 Dual-Core (Huron), 65nm");
   FMQ (0,15,  6,11,     MN, "AMD Athlon Neo X2 (Huron), 65nm");
   FMQ (0,15,  6,11,     dA, "AMD Athlon 64 X2 Dual-Core (Brisbane), 65nm");
   if(0);
   FM  (0,15,  6,11,         "AMD Athlon 64 X2 Dual-Core (Brisbane) / Sempron Dual-Core (Sparta) / Athlon Neo X2 (Huron), 65nm");
   FMSQ(0,15,  6,12,  2, MS, "AMD mobile Sempron (Sherman DH-G2), 65nm");
   FMSQ(0,15,  6,12,  2, dS, "AMD Sempron (Sparta DH-G2), 65nm");
   FMSQ(0,15,  6,12,  2, dA, "AMD Athlon 64 (Lima DH-G2), 65nm");
   FMS (0,15,  6,12,  2,     "AMD Athlon 64 (Lima DH-G2) / Sempron (Sparta DH-G2) / mobile Sempron (Sherman DH-G2), 65nm");
   FMQ (0,15,  6,12,     MS, "AMD mobile Sempron (Sherman), 65nm");
   FMQ (0,15,  6,12,     dS, "AMD Sempron (Sparta), 65nm");
   FMQ (0,15,  6,12,     dA, "AMD Athlon 64 (Lima), 65nm");
   FM  (0,15,  6,12,         "AMD Athlon 64 (Lima) / Sempron (Sparta) / mobile Sempron (Sherman), 65nm");
   FMSQ(0,15,  6,15,  2, MS, "AMD mobile Sempron (Sherman DH-G2), 65nm");
   FMSQ(0,15,  6,15,  2, dS, "AMD Sempron (Sparta DH-G2), 65nm");
   FMSQ(0,15,  6,15,  2, MN, "AMD Athlon Neo (Huron DH-G2), 65nm");
   FMS (0,15,  6,15,  2,     "AMD Athlon Neo (Huron DH-G2) / Sempron (Sparta DH-G2) / mobile Sempron (Sherman DH-G2), 65nm");
   FMQ (0,15,  6,15,     MS, "AMD mobile Sempron (Sherman), 65nm");
   FMQ (0,15,  6,15,     dS, "AMD Sempron (Sparta), 65nm");
   FMQ (0,15,  6,15,     MN, "AMD Athlon Neo (Huron), 65nm");
   FM  (0,15,  6,15,         "AMD Athlon Neo (Huron) / Sempron (Sparta) / mobile Sempron (Sherman), 65nm");
   FMSQ(0,15,  7,12,  2, MS, "AMD mobile Sempron (Sherman DH-G2), 65nm");
   FMSQ(0,15,  7,12,  2, dS, "AMD Sempron (Sparta DH-G2), 65nm");
   FMSQ(0,15,  7,12,  2, dA, "AMD Athlon (Lima DH-G2), 65nm");
   FMS (0,15,  7,12,  2,     "AMD Athlon (Lima DH-G2) / Sempron (Sparta DH-G2) / mobile Sempron (Sherman DH-G2), 65nm");
   FMQ (0,15,  7,12,     MS, "AMD mobile Sempron (Sherman), 65nm");
   FMQ (0,15,  7,12,     dS, "AMD Sempron (Sparta), 65nm");
   FMQ (0,15,  7,12,     dA, "AMD Athlon (Lima), 65nm");
   FM  (0,15,  7,12,         "AMD Athlon (Lima) / Sempron (Sparta) / mobile Sempron (Sherman), 65nm");
   FMSQ(0,15,  7,15,  1, MS, "AMD mobile Sempron (Sherman DH-G1), 65nm");
   FMSQ(0,15,  7,15,  1, dS, "AMD Sempron (Sparta DH-G1), 65nm");
   FMSQ(0,15,  7,15,  1, dA, "AMD Athlon 64 (Lima DH-G1), 65nm");
   FMS (0,15,  7,15,  1,     "AMD Athlon 64 (Lima DH-G1) / Sempron (Sparta DH-G1) / mobile Sempron (Sherman DH-G1), 65nm");
   FMSQ(0,15,  7,15,  2, MS, "AMD mobile Sempron (Sherman DH-G2), 65nm");
   FMSQ(0,15,  7,15,  2, dS, "AMD Sempron (Sparta DH-G2), 65nm");
   FMSQ(0,15,  7,15,  2, MN, "AMD Athlon Neo (Huron DH-G2), 65nm");
   FMSQ(0,15,  7,15,  2, dA, "AMD Athlon 64 (Lima DH-G2), 65nm");
   FMS (0,15,  7,15,  2,     "AMD Athlon 64 (Lima DH-G2) / Sempron (Sparta DH-G2) / mobile Sempron (Sherman DH-G2) / Athlon Neo (Huron DH-G2), 65nm");
   FMQ (0,15,  7,15,     MS, "AMD mobile Sempron (Sherman), 65nm");
   FMQ (0,15,  7,15,     dS, "AMD Sempron (Sparta), 65nm");
   FMQ (0,15,  7,15,     MN, "AMD Athlon Neo (Huron), 65nm");
   FMQ (0,15,  7,15,     dA, "AMD Athlon 64 (Lima), 65nm");
   FM  (0,15,  7,15,         "AMD Athlon 64 (Lima) / Sempron (Sparta) / mobile Sempron (Sherman) / Athlon Neo (Huron), 65nm");
   FMS (0,15, 12, 1,  3,     "AMD Athlon 64 FX Dual-Core (Windsor JH-F3), 90nm");
   FM  (0,15, 12, 1,         "AMD Athlon 64 FX Dual-Core (Windsor), 90nm");
   F   (0,15,                "AMD Opteron / Athlon 64 / Athlon 64 FX / Sempron / Turion / Athlon Neo / Dual Core Opteron / Athlon 64 X2 / Athlon 64 FX / mobile Athlon 64 / mobile Sempron / mobile Athlon XP-M (DP) (unknown model)");
   FMSQ(1,15,  0, 2,  1, dO, "AMD Quad-Core Opteron (Barcelona DR-B1), 65nm");
   FMS (1,15,  0, 2,  1,     "AMD Quad-Core Opteron (Barcelona DR-B1), 65nm");
   FMSQ(1,15,  0, 2,  2, EO, "AMD Embedded Opteron (Barcelona DR-B2), 65nm");
   FMSQ(1,15,  0, 2,  2, dO, "AMD Quad-Core Opteron (Barcelona DR-B2), 65nm");
   FMSQ(1,15,  0, 2,  2, Tp, "AMD Phenom Triple-Core (Toliman DR-B2), 65nm");
   FMSQ(1,15,  0, 2,  2, Qp, "AMD Phenom Quad-Core (Agena DR-B2), 65nm");
   FMS (1,15,  0, 2,  2,     "AMD Quad-Core Opteron (Barcelona DR-B2) / Embedded Opteron (Barcelona DR-B2) / Phenom Triple-Core (Toliman DR-B2) / Phenom Quad-Core (Agena DR-B2), 65nm");
   FMQ (1,15,  0, 2,     EO, "AMD Embedded Opteron (Barcelona), 65nm");
   FMQ (1,15,  0, 2,     dO, "AMD Quad-Core Opteron (Barcelona), 65nm");
   FMQ (1,15,  0, 2,     Tp, "AMD Phenom Triple-Core (Toliman), 65nm");
   FMQ (1,15,  0, 2,     Qp, "AMD Phenom Quad-Core (Agena), 65nm");
   FM  (1,15,  0, 2,         "AMD Opteron (Barcelona) / Phenom Triple-Core (Toliman) / Phenom Quad-Core (Agena), 65nm");
   FMSQ(1,15,  0, 2,  3, EO, "AMD Embedded Opteron (Barcelona DR-B3), 65nm");
   FMSQ(1,15,  0, 2,  3, dO, "AMD Quad-Core Opteron (Barcelona DR-B3), 65nm");
   FMSQ(1,15,  0, 2,  3, Tp, "AMD Phenom Triple-Core (Toliman DR-B3), 65nm");
   FMSQ(1,15,  0, 2,  3, Qp, "AMD Phenom Quad-Core (Agena DR-B3), 65nm");
   FMSQ(1,15,  0, 2,  3, dA, "AMD Athlon Dual-Core (Kuma DR-B3), 65nm");
   FMS (1,15,  0, 2,  3,     "AMD Quad-Core Opteron (Barcelona DR-B3) / Embedded Opteron (Barcelona DR-B2) / Phenom Triple-Core (Toliman DR-B3) / Phenom Quad-Core (Agena DR-B3) / Athlon Dual-Core (Kuma DR-B3), 65nm");
   FMS (1,15,  0, 2, 10,     "AMD Quad-Core Opteron (Barcelona DR-BA), 65nm");
   FMQ (1,15,  0, 2,     EO, "AMD Embedded Opteron (Barcelona), 65nm");
   FMQ (1,15,  0, 2,     dO, "AMD Quad-Core Opteron (Barcelona), 65nm");
   FMQ (1,15,  0, 2,     Tp, "AMD Phenom Triple-Core (Toliman), 65nm");
   FMQ (1,15,  0, 2,     Qp, "AMD Phenom Quad-Core (Agena), 65nm");
   FMQ (1,15,  0, 2,     dA, "AMD Athlon Dual-Core (Kuma), 65nm");
   FM  (1,15,  0, 2,         "AMD Quad-Core Opteron (Barcelona) / Phenom Triple-Core (Toliman) / Phenom Quad-Core (Agena) / Athlon Dual-Core (Kuma), 65nm");
   FMSQ(1,15,  0, 4,  2, EO, "AMD Embedded Opteron (Shanghai RB-C2), 45nm");
   FMSQ(1,15,  0, 4,  2, dO, "AMD Quad-Core Opteron (Shanghai RB-C2), 45nm");
   FMSQ(1,15,  0, 4,  2, dR, "AMD Athlon Dual-Core (Propus RB-C2), 45nm");
   FMSQ(1,15,  0, 4,  2, dA, "AMD Athlon Dual-Core (Regor RB-C2), 45nm");
   FMSQ(1,15,  0, 4,  2, Dp, "AMD Phenom II X2 (Callisto RB-C2), 45nm");
   FMSQ(1,15,  0, 4,  2, Tp, "AMD Phenom II X3 (Heka RB-C2), 45nm");
   FMSQ(1,15,  0, 4,  2, Qp, "AMD Phenom II X4 (Deneb RB-C2), 45nm");
   FMS (1,15,  0, 4,  2,     "AMD Quad-Core Opteron (Shanghai RB-C2) / Embedded Opteron (Shanghai RB-C2) / Athlon Dual-Core (Regor / Propus RB-C2) / Phenom II (Callisto / Heka / Deneb RB-C2), 45nm");
   FMSQ(1,15,  0, 4,  3, Dp, "AMD Phenom II X2 (Callisto RB-C3), 45nm");
   FMSQ(1,15,  0, 4,  3, Tp, "AMD Phenom II X3 (Heka RB-C3), 45nm");
   FMSQ(1,15,  0, 4,  3, Qp, "AMD Phenom II X4 (Deneb RB-C3), 45nm");
   FMS (1,15,  0, 4,  3,     "AMD Phenom II (Callisto / Heka / Deneb RB-C3), 45nm");
   FMQ (1,15,  0, 4,     EO, "AMD Embedded Opteron (Shanghai), 45nm");
   FMQ (1,15,  0, 4,     dO, "AMD Quad-Core Opteron (Shanghai), 45nm");
   FMQ (1,15,  0, 4,     dR, "AMD Athlon Dual-Core (Propus), 45nm");
   FMQ (1,15,  0, 4,     dA, "AMD Athlon Dual-Core (Regor), 45nm");
   FMQ (1,15,  0, 4,     Dp, "AMD Phenom II X2 (Callisto), 45nm");
   FMQ (1,15,  0, 4,     Tp, "AMD Phenom II X3 (Heka), 45nm");
   FMQ (1,15,  0, 4,     Qp, "AMD Phenom II X4 (Deneb), 45nm");
   FM  (1,15,  0, 4,         "AMD Quad-Core Opteron (Shanghai) / Athlon Dual-Core (Regor / Propus) / Phenom II (Callisto / Heka / Deneb), 45nm");
   FMSQ(1,15,  0, 5,  2, DA, "AMD Athlon II X2 (Regor BL-C2), 45nm");
   FMSQ(1,15,  0, 5,  2, TA, "AMD Athlon II X3 (Rana BL-C2), 45nm");
   FMSQ(1,15,  0, 5,  2, QA, "AMD Athlon II X4 (Propus BL-C2), 45nm");
   FMS (1,15,  0, 5,  2,     "AMD Athlon II X2 / X3 / X4 (Regor / Rana / Propus BL-C2), 45nm");
   FMSQ(1,15,  0, 5,  3, TA, "AMD Athlon II X3 (Rana BL-C3), 45nm");
   FMSQ(1,15,  0, 5,  3, QA, "AMD Athlon II X4 (Propus BL-C3), 45nm");
   FMSQ(1,15,  0, 5,  3, Tp, "AMD Phenom II Triple-Core (Heka BL-C3), 45nm");
   FMSQ(1,15,  0, 5,  3, Qp, "AMD Phenom II Quad-Core (Deneb BL-C3), 45nm");
   FMS (1,15,  0, 5,  3,     "AMD Athlon II X3 / X4 (Rana / Propus BL-C3) / Phenom II Triple-Core (Heka BL-C3) / Quad-Core (Deneb BL-C3), 45nm");
   FMQ (1,15,  0, 5,     DA, "AMD Athlon II X2 (Regor), 45nm");
   FMQ (1,15,  0, 5,     TA, "AMD Athlon II X3 (Rana), 45nm");
   FMQ (1,15,  0, 5,     QA, "AMD Athlon II X4 (Propus), 45nm");
   FMQ (1,15,  0, 5,     Tp, "AMD Phenom II Triple-Core (Heka), 45nm");
   FMQ (1,15,  0, 5,     Qp, "AMD Phenom II Quad-Core (Deneb), 45nm");
   FM  (1,15,  0, 5,         "AMD Athlon II X2 / X3 / X4 (Regor / Rana / Propus) / Phenom II Triple-Core (Heka) / Quad-Core (Deneb), 45nm");
   FMSQ(1,15,  0, 6,  2, MS, "AMD Sempron Mobile (Sargas DA-C2), 45nm");
   FMSQ(1,15,  0, 6,  2, dS, "AMD Sempron II (Sargas DA-C2), 45nm");
   FMSQ(1,15,  0, 6,  2, MT, "AMD Turion II Dual-Core Mobile (Caspian DA-C2), 45nm");
   FMSQ(1,15,  0, 6,  2, MA, "AMD Athlon II Dual-Core Mobile (Regor DA-C2), 45nm");
   FMSQ(1,15,  0, 6,  2, DA, "AMD Athlon II X2 (Regor DA-C2), 45nm");
   FMSQ(1,15,  0, 6,  2, dA, "AMD Athlon II (Sargas DA-C2), 45nm");
   FMS (1,15,  0, 6,  2,     "AMD Athlon II (Sargas DA-C2) / Athlon II X2 (Regor DA-C2) / Sempron II (Sargas DA-C2) / Athlon II Dual-Core Mobile (Regor DA-C2) / Sempron Mobile (Sargas DA-C2) / Turion II Dual-Core Mobile (Caspian DA-C2), 45nm");
   FMSQ(1,15,  0, 6,  3, MV, "AMD V-Series Mobile (Champlain DA-C3), 45nm");
   FMSQ(1,15,  0, 6,  3, DS, "AMD Sempron II X2 (Regor DA-C3), 45nm");
   FMSQ(1,15,  0, 6,  3, dS, "AMD Sempron II (Sargas DA-C3), 45nm");
   FMSQ(1,15,  0, 6,  3, MT, "AMD Turion II Dual-Core Mobile (Champlain DA-C3), 45nm");
   FMSQ(1,15,  0, 6,  3, Mp, "AMD Phenom II Dual-Core Mobile (Champlain DA-C3), 45nm");
   FMSQ(1,15,  0, 6,  3, MA, "AMD Athlon II Dual-Core Mobile (Champlain DA-C3), 45nm");
   FMSQ(1,15,  0, 6,  3, DA, "AMD Athlon II X2 (Regor DA-C3), 45nm");
   FMSQ(1,15,  0, 6,  3, dA, "AMD Athlon II (Sargas DA-C3), 45nm");
   FMS (1,15,  0, 6,  3,     "AMD Athlon II (Sargas DA-C3) / Athlon II X2 (Regor DA-C2) / Sempron II (Sargas DA-C2) / Sempron II X2 (Regor DA-C3) / V-Series Mobile (Champlain DA-C3) / Athlon II Dual-Core Mobile (Champlain DA-C3) / Turion II Dual-Core Mobile (Champlain DA-C3) / Phenom II Dual-Core Mobile (Champlain DA-C3), 45nm");
   FMQ (1,15,  0, 6,     MV, "AMD V-Series Mobile (Champlain), 45nm");
   FMQ (1,15,  0, 6,     MS, "AMD Sempron Mobile (Sargas), 45nm");
   FMQ (1,15,  0, 6,     DS, "AMD Sempron II X2 (Regor), 45nm");
   FMQ (1,15,  0, 6,     dS, "AMD Sempron II (Sargas), 45nm");
   if(0);
   FMQ (1,15,  0, 6,     MT, "AMD Turion II Dual-Core Mobile (Caspian / Champlain), 45nm");
   FMQ (1,15,  0, 6,     Mp, "AMD Phenom II Dual-Core Mobile (Champlain), 45nm");
   FMQ (1,15,  0, 6,     MA, "AMD Athlon II Dual-Core Mobile (Regor / Champlain), 45nm");
   FMQ (1,15,  0, 6,     DA, "AMD Athlon II X2 (Regor), 45nm");
   FMQ (1,15,  0, 6,     dA, "AMD Athlon II (Sargas), 45nm");
   FM  (1,15,  0, 6,         "AMD Athlon II (Sargas) / Athlon II X2 (Regor) / Sempron II (Sargas) / Sempron II X2 (Regor) / Sempron Mobile (Sargas) / V-Series Mobile (Champlain) / Athlon II Dual-Core Mobile (Regor / Champlain) / Turion II Dual-Core Mobile (Caspian / Champlain) / Phenom II Dual-Core Mobile (Champlain), 45nm");
   FMSQ(1,15,  0, 8,  0, SO, "AMD Six-Core Opteron (Istanbul HY-D0), 45nm");
   FMSQ(1,15,  0, 8,  0, dO, "AMD Opteron 4100 (Lisbon HY-D0), 45nm");
   FMS (1,15,  0, 8,  0,     "AMD Opteron 4100 (Lisbon HY-D0) / Six-Core Opteron (Istanbul HY-D0), 45nm");
   FMS (1,15,  0, 8,  1,     "AMD Opteron 4100 (Lisbon HY-D1), 45nm");
   FMQ (1,15,  0, 8,     SO, "AMD Six-Core Opteron (Istanbul), 45nm");
   FMQ (1,15,  0, 8,     dO, "AMD Opteron 4100 (Lisbon), 45nm");
   FM  (1,15,  0, 8,         "AMD Opteron 4100 (Lisbon) / Six-Core Opteron (Istanbul), 45nm");
   FMS (1,15,  0, 9,  1,     "AMD Opteron 6100 (Magny-Cours HY-D1), 45nm");
   FM  (1,15,  0, 9,         "AMD Opteron 6100 (Magny-Cours), 45nm");
   FMSQ(1,15,  0,10,  0, Qp, "AMD Phenom II X4 (Zosma PH-E0), 45nm");
   FMSQ(1,15,  0,10,  0, Sp, "AMD Phenom II X6 (Thuban PH-E0), 45nm");
   FMS (1,15,  0,10,  0,     "AMD Phenom II X4 / X6 (Zosma / Thuban PH-E0), 45nm");
   FMQ (1,15,  0,10,     Qp, "AMD Phenom II X4 (Zosma), 45nm");
   FMQ (1,15,  0,10,     Sp, "AMD Phenom II X6 (Thuban), 45nm");
   FM  (1,15,  0,10,         "AMD Phenom II X4 / X6 (Zosma / Thuban), 45nm");
   F   (1,15,                "AMD Athlon / Athlon II / Athlon II Xn / Opteron / Opteron 4100 / Opteron 6100 / Embedded Opteron / Phenom / Phenom II / Phenom II Xn / Sempron II / Sempron II Xn / Sempron Mobile / Turion II / V-Series Mobile");
   FMSQ(2,15,  0, 3,  1, MT, "AMD Turion X2 Dual-Core Mobile (Lion LG-B1), 65nm");
   FMSQ(2,15,  0, 3,  1, DS, "AMD Sempron X2 Dual-Core (Sable LG-B1), 65nm");
   FMSQ(2,15,  0, 3,  1, dS, "AMD Sempron (Sable LG-B1), 65nm");
   FMSQ(2,15,  0, 3,  1, DA, "AMD Athlon X2 Dual-Core (Lion LG-B1), 65nm");
   FMSQ(2,15,  0, 3,  1, dA, "AMD Athlon (Lion LG-B1), 65nm");
   FMS (2,15,  0, 3,  1,     "AMD Turion X2 Dual-Core Mobile (Lion LG-B1) / Athlon (Lion LG-B1) / Athlon X2 Dual-Core (Lion LG-B1) / Sempron (Sable LG-B1) / Sempron X2 Dual-Core (Sable LG-B1), 65nm");
   FMQ (2,15,  0, 3,     MT, "AMD Turion X2 (Lion), 65nm");
   FMQ (2,15,  0, 3,     DS, "AMD Sempron X2 Dual-Core (Sable), 65nm");
   FMQ (2,15,  0, 3,     dS, "AMD Sempron (Sable), 65nm");
   FMQ (2,15,  0, 3,     DA, "AMD Athlon X2 Dual-Core (Lion), 65nm");
   FMQ (2,15,  0, 3,     dA, "AMD Athlon (Lion), 65nm");
   FM  (2,15,  0, 3,         "AMD Turion X2 (Lion) / Athlon (Lion) / Sempron (Sable), 65nm");
   F   (2,15,                "AMD Turion X2 Mobile / Athlon / Athlon X2 / Sempron / Sempron X2, 65nm");
   FMS (5,15,  0, 1,  0,     "AMD C-Series (Ontario B0) / E-Series (Zacate B0) / G-Series (Ontario/Zacat B0), 40nm");
   FM  (5,15,  0, 1,         "AMD C-Series (Ontario) / E-Series (Zacate) / G-Series (Ontario/Zacat), 40nm");
   F   (5,15,                "AMD C-Series / E-Series / G-Series, 40nm");
   DEFAULT                  ("unknown");

  /* const char*  brand_pre;
   const char*  brand_post;
   char         proc[96];
   decode_amd_model(stash, &brand_pre, &brand_post, proc);
   if (proc[0] != '\0') {
      printf(" %s", proc);
   }
   */
}
