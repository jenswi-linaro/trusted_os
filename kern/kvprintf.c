/*-
 * Copyright (c) 1986, 1988, 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)subr_prf.c  8.3 (Berkeley) 1/21/94
 */

/*
 * File is copied from FreeBSD sys/kern/subr_prf.c rev 226435
 * and optimized to fit in this environment.
 */

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <kvprintf.h>

#define NBBY    8
/* Max number conversion buffer length: a uint64_t in base 2, plus NUL byte. */
#define MAXNBUF (sizeof(intmax_t) * NBBY + 1)

#define toupper(c)      ((c) - 0x20 * (((c) >= 'a') && ((c) <= 'z')))

static char const hex2ascii[] = "0123456789abcdefghijklmnopqrstuvwxyz";

static int imax(int a, int b)
{
    return (a > b ? a : b);
}

/*
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
static char *ksprintn(char *nbuf, uintmax_t num, int base, int *lenp,
                bool upper)
{
    char *p, c;

    p = nbuf;
    *p = '\0';
    do {
        c = hex2ascii[num % base];
        *++p = upper ? toupper(c) : c;
    } while (num /= base);
    if (lenp)
        *lenp = p - nbuf;
    return (p);
}

enum flag {
    FLAG_LFLAG = 0,
    FLAG_QFLAG,
    FLAG_SHARPFLAG,
    FLAG_NEG,
    FLAG_SIGN,
    FLAG_DOT,
    FLAG_UPPER,
    FLAG_CFLAG,
    FLAG_HFLAG,
    FLAG_JFLAG,
    FLAG_TFLAG,
    FLAG_ZFLAG,
    FLAG_STOP,
    FLAG_LADJUST,
};

#define FLAG_IS_SET(flag)   (flags & (1 << (flag)))
#define FLAG_SET(flag)      do { flags |=  (1 << (flag)); } while (0)
#define FLAG_CLR(flag)      do { flags &= ~(1 << (flag)); } while (0)
#define FLAG_INV(flag)      do { flags ^=  (1 << (flags)); } while (0)

/*
 * Scaled down version of printf(3).
 */
int kvprintf(kvprintf_putc func, void *arg, int radix,
                const char *fmt, va_list ap)
{
#define PCHAR(c) {int cc=(c); if (func) (*func)(cc,arg); else *d++ = cc; retval++; }
    char nbuf[MAXNBUF];
    char *d;
    const char *p, *percent;
    int ch, n;
    uintmax_t num;
    int base, tmp, width;
    int dwidth;
    char padc;
    int retval = 0;
    uint32_t flags = 0;

    num = 0;
    if (!func)
        d = (char *) arg;
    else
        d = NULL;

    if (fmt == NULL)
        fmt = "(fmt null)\n";

    if (radix < 2 || radix > 36)
        radix = 10;

    for (;;) {
        padc = ' ';
        width = 0;
        while ((ch = (unsigned char)*fmt++) != '%' || FLAG_IS_SET(FLAG_STOP)) {
            if (ch == '\0')
                return (retval);
            PCHAR(ch);
        }
        percent = fmt - 1;
        flags = 0;
        dwidth = 0;
reswitch:   switch (ch = (unsigned char)*fmt++) {
        case '.':
            FLAG_SET(FLAG_DOT);
            goto reswitch;
        case '#':
            FLAG_SET(FLAG_SHARPFLAG);
            goto reswitch;
        case '+':
            FLAG_SET(FLAG_SIGN);
            goto reswitch;
        case '-':
            FLAG_SET(FLAG_LADJUST);
            goto reswitch;
        case '%':
            PCHAR(ch);
            break;
        case '*':
            if (!FLAG_IS_SET(FLAG_DOT)) {
                width = va_arg(ap, int);
                if (width < 0) {
                    FLAG_INV(FLAG_LADJUST);
                    width = -width;
                }
            } else {
                dwidth = va_arg(ap, int);
            }
            goto reswitch;
        case '0':
            if (!FLAG_IS_SET(FLAG_DOT)) {
                padc = '0';
                goto reswitch;
            }
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
                for (n = 0;; ++fmt) {
                    n = n * 10 + ch - '0';
                    ch = *fmt;
                    if (ch < '0' || ch > '9')
                        break;
                }
            if (FLAG_IS_SET(FLAG_DOT))
                dwidth = n;
            else
                width = n;
            goto reswitch;
        case 'c':
            PCHAR(va_arg(ap, int));
            break;
        case 'd':
        case 'i':
            base = 10;
            FLAG_SET(FLAG_SIGN);
            goto handle_sign;
        case 'h':
            if (FLAG_IS_SET(FLAG_HFLAG)) {
                FLAG_CLR(FLAG_HFLAG);
                FLAG_SET(FLAG_CFLAG);
            } else
                FLAG_SET(FLAG_HFLAG);
            goto reswitch;
        case 'j':
            FLAG_SET(FLAG_JFLAG);
            goto reswitch;
        case 'l':
            if (FLAG_IS_SET(FLAG_LFLAG)) {
                FLAG_SET(FLAG_QFLAG);
            }

            FLAG_INV(FLAG_LFLAG);
            goto reswitch;
        case 'o':
            base = 8;
            goto handle_nosign;
        case 'p':
            base = 16;
            if (width == 0)
                FLAG_SET(FLAG_SHARPFLAG);
            else
                FLAG_CLR(FLAG_SHARPFLAG);
            FLAG_CLR(FLAG_SIGN);
            num = (uintptr_t)va_arg(ap, void *);
            goto number;
        case 'r':
            base = radix;
            if (FLAG_IS_SET(FLAG_SIGN))
                goto handle_sign;
            goto handle_nosign;
        case 's':
            p = va_arg(ap, char *);
            if (p == NULL)
                p = "(null)";
            if (!FLAG_IS_SET(FLAG_DOT))
                n = strlen (p);
            else
                for (n = 0; n < dwidth && p[n]; n++)
                    continue;

            width -= n;

            if (!FLAG_IS_SET(FLAG_LADJUST) && width > 0)
                while (width--)
                    PCHAR(padc);
            while (n--)
                PCHAR(*p++);
            if (FLAG_IS_SET(FLAG_LADJUST) && width > 0)
                while (width--)
                    PCHAR(padc);
            break;
        case 't':
            FLAG_SET(FLAG_TFLAG);
            goto reswitch;
        case 'u':
            base = 10;
            goto handle_nosign;
        case 'X':
            FLAG_SET(FLAG_UPPER);
        case 'x':
            base = 16;
            goto handle_nosign;
        case 'y':
            base = 16;
            FLAG_SET(FLAG_SIGN);
            goto handle_sign;
        case 'z':
            FLAG_SET(FLAG_ZFLAG);
            goto reswitch;
handle_nosign:
            FLAG_CLR(FLAG_SIGN);
            if (FLAG_IS_SET(FLAG_JFLAG))
                num = va_arg(ap, uintmax_t);
            else if (FLAG_IS_SET(FLAG_QFLAG))
                num = va_arg(ap, uint64_t);
            else if (FLAG_IS_SET(FLAG_TFLAG))
                num = va_arg(ap, ptrdiff_t);
            else if (FLAG_IS_SET(FLAG_LFLAG))
                num = va_arg(ap, unsigned long);
            else if (FLAG_IS_SET(FLAG_ZFLAG))
                num = va_arg(ap, size_t);
            else if (FLAG_IS_SET(FLAG_HFLAG))
                num = (unsigned short)va_arg(ap, int);
            else if (FLAG_IS_SET(FLAG_CFLAG))
                num = (unsigned char)va_arg(ap, int);
            else
                num = va_arg(ap, unsigned int);
            goto number;
handle_sign:
            if (FLAG_IS_SET(FLAG_JFLAG))
                num = va_arg(ap, intmax_t);
            else if (FLAG_IS_SET(FLAG_QFLAG))
                num = va_arg(ap, uint64_t);
            else if (FLAG_IS_SET(FLAG_TFLAG))
                num = va_arg(ap, ptrdiff_t);
            else if (FLAG_IS_SET(FLAG_LFLAG))
                num = va_arg(ap, long);
            else if (FLAG_IS_SET(FLAG_ZFLAG))
                num = va_arg(ap, ssize_t);
            else if (FLAG_IS_SET(FLAG_HFLAG))
                num = (short)va_arg(ap, int);
            else if (FLAG_IS_SET(FLAG_CFLAG))
                num = (char)va_arg(ap, int);
            else
                num = va_arg(ap, int);
number:
            if (FLAG_IS_SET(FLAG_SIGN) && (intmax_t)num < 0) {
                FLAG_SET(FLAG_NEG);
                num = -(intmax_t)num;
            }
            p = ksprintn(nbuf, num, base, &n, FLAG_IS_SET(FLAG_UPPER));
            tmp = 0;
            if (FLAG_IS_SET(FLAG_SHARPFLAG) && num != 0) {
                if (base == 8)
                    tmp++;
                else if (base == 16)
                    tmp += 2;
            }
            if (FLAG_IS_SET(FLAG_NEG))
                tmp++;

            if (!FLAG_IS_SET(FLAG_LADJUST) && padc == '0')
                dwidth = width - tmp;
            width -= tmp + imax(dwidth, n);
            dwidth -= n;
            if (!FLAG_IS_SET(FLAG_LADJUST))
                while (width-- > 0)
                    PCHAR(' ');
            if (FLAG_IS_SET(FLAG_NEG))
                PCHAR('-');
            if (FLAG_IS_SET(FLAG_SHARPFLAG) && num != 0) {
                if (base == 8) {
                    PCHAR('0');
                } else if (base == 16) {
                    PCHAR('0');
                    PCHAR('x');
                }
            }
            while (dwidth-- > 0)
                PCHAR('0');

            while (*p)
                PCHAR(*p--);

            if (FLAG_IS_SET(FLAG_LADJUST))
                while (width-- > 0)
                    PCHAR(' ');

            break;
        default:
            while (percent < fmt)
                PCHAR(*percent++);
            /*
             * Since we ignore an formatting argument it is no
             * longer safe to obey the remaining formatting
             * arguments as the arguments will no longer match
             * the format specs.
             */
            FLAG_SET(FLAG_STOP);
            break;
        }
    }
#undef PCHAR
}
