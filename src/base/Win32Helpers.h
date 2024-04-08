#pragma once

#include <string>

/* The alphabet, the code corresponds to the capitalized letter in the ASCII code */
#define VK_KEY_A	0x41 /* 'A' key */
#define VK_KEY_B	0x42 /* 'B' key */
#define VK_KEY_C	0x43 /* 'C' key */
#define VK_KEY_D	0x44 /* 'D' key */
#define VK_KEY_E	0x45 /* 'E' key */
#define VK_KEY_F	0x46 /* 'F' key */
#define VK_KEY_G	0x47 /* 'G' key */
#define VK_KEY_H	0x48 /* 'H' key */
#define VK_KEY_I	0x49 /* 'I' key */
#define VK_KEY_J	0x4A /* 'J' key */
#define VK_KEY_K	0x4B /* 'K' key */
#define VK_KEY_L	0x4C /* 'L' key */
#define VK_KEY_M	0x4D /* 'M' key */
#define VK_KEY_N	0x4E /* 'N' key */
#define VK_KEY_O	0x4F /* 'O' key */
#define VK_KEY_P	0x50 /* 'P' key */
#define VK_KEY_Q	0x51 /* 'Q' key */
#define VK_KEY_R	0x52 /* 'R' key */
#define VK_KEY_S	0x53 /* 'S' key */
#define VK_KEY_T	0x54 /* 'T' key */
#define VK_KEY_U	0x55 /* 'U' key */
#define VK_KEY_V	0x56 /* 'V' key */
#define VK_KEY_W	0x57 /* 'W' key */
#define VK_KEY_X	0x58 /* 'X' key */
#define VK_KEY_Y	0x59 /* 'Y' key */
#define VK_KEY_Z	0x5A /* 'Z' key */

namespace Win32Helpers
{
    void SetupConsole(const std::string& title);
}