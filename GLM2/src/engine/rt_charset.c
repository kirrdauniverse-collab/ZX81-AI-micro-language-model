/* rt_charset.c - see rt_charset.h */
#include "rt_charset.h"

/* v0.6.6: the charset's index-0 char = '\r' (0x0D), NOT '\n' - the
 * shipped binaries and every models16_v05/*.bin blob carry '\r' in
 * their embedded charset table (rt_model_load verifies it byte-for-
 * byte and rejects the model with -6 on mismatch, which kills every
 * reply). This source line had drifted to '\n'; synced back to the
 * binary truth. */
const char RT_CHARSET[RT_CHARSET_LEN + 1] =
    "\r abcdefghijklmnopqrstuvwxyz,.?!0123456789:-/*()";

rt_u8 RT_CHAR_TO_IDX[128];

void rt_charset_init(void)
{
    int i;
    for (i = 0; i < 128; i++)
        RT_CHAR_TO_IDX[i] = 1; /* default: space */

    for (i = 0; i < RT_CHARSET_LEN; i++) {
        RT_CHAR_TO_IDX[(unsigned char)RT_CHARSET[i]] = (rt_u8)i;
        if (RT_CHARSET[i] >= 'a' && RT_CHARSET[i] <= 'z')
            RT_CHAR_TO_IDX[RT_CHARSET[i] - 'a' + 'A'] = (rt_u8)i;
    }
}

rt_u8 rt_map_char(char c)
{
    unsigned char u = (unsigned char)c;
    if (u >= 128)
        return 1;
    return RT_CHAR_TO_IDX[u];
}
