#pragma once

// ISO-8859-9 Turkish Characters (extended ASCII)
#define CH_C_CEDILLA 0xC7
#define CH_G_BREVE   0xD0
#define CH_I_DOT     0xDD
#define CH_O_UMLAUT  0xD6
#define CH_S_CEDILLA 0xDE
#define CH_U_UMLAUT  0xDC

#define CH_c_cedilla 0xE7
#define CH_g_breve   0xF0
#define CH_i_nodot   0xFD
#define CH_o_umlaut  0xF6
#define CH_s_cedilla 0xFE
#define CH_u_umlaut  0xFC

// US QWERTY Layout (Lower)
static const char us_map_lower[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', 0x09,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0x1C, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// US QWERTY Layout (Upper - Shift)
static const char us_map_upper[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', 0x09,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x1C, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

// Turkish Q Layout (Lower)
static const char tr_q_map_lower[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '*', '-', '\b', 0x09,
    'q', 'w', 'e', 'r', 't', 'y', 'u', CH_i_nodot, 'o', 'p', CH_g_breve, CH_u_umlaut, 0x1C, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', CH_s_cedilla, 'i', ',', 0,
    '<', 'z', 'x', 'c', 'v', 'b', 'n', 'm', CH_o_umlaut, CH_c_cedilla, '.', 0, '*', 0, ' '
};

// Turkish Q Layout (Upper)
static const char tr_q_map_upper[128] = {
    0, 27, '!', '\'', '^', '+', '%', '&', '/', '(', ')', '=', '?', '_', '\b', 0x09,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', CH_G_BREVE, CH_U_UMLAUT, 0x1C, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', CH_S_CEDILLA, CH_I_DOT, ';', 0,
    '>', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', CH_O_UMLAUT, CH_C_CEDILLA, ':', 0, '*', 0, ' '
};
