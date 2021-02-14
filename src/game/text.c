#include "game/data.h"
#include "game/text.h"
#include "types.h"
#include "util.h"
#include "mod.h"
#include <string.h>

static int8_t TextSpacing[110] = {
    14 /*A*/,  11 /*B*/, 11 /*C*/, 11 /*D*/, 11 /*E*/, 11 /*F*/, 11 /*G*/,
    13 /*H*/,  8 /*I*/,  11 /*J*/, 12 /*K*/, 11 /*L*/, 13 /*M*/, 13 /*N*/,
    12 /*O*/,  11 /*P*/, 12 /*Q*/, 12 /*R*/, 11 /*S*/, 12 /*T*/, 13 /*U*/,
    13 /*V*/,  13 /*W*/, 12 /*X*/, 12 /*Y*/, 11 /*Z*/, 9 /*a*/,  9 /*b*/,
    9 /*c*/,   9 /*d*/,  9 /*e*/,  9 /*f*/,  9 /*g*/,  9 /*h*/,  5 /*i*/,
    9 /*j*/,   9 /*k*/,  5 /*l*/,  12 /*m*/, 10 /*n*/, 9 /*o*/,  9 /*p*/,
    9 /*q*/,   8 /*r*/,  9 /*s*/,  8 /*t*/,  9 /*u*/,  9 /*v*/,  11 /*w*/,
    9 /*x*/,   9 /*y*/,  9 /*z*/,  12 /*0*/, 8 /*1*/,  10 /*2*/, 10 /*3*/,
    10 /*4*/,  10 /*5*/, 10 /*6*/, 9 /*7*/,  10 /*8*/, 10 /*9*/, 5 /*.*/,
    5 /*,*/,   5 /*!*/,  11 /*?*/, 9 /*"*/,  10 /*"*/, 8 /**/,   6 /*(*/,
    6 /*)*/,   7 /*-*/,  7 /*=*/,  3 /*:*/,  11 /*%*/, 8 /*+*/,  13 /*(c)*/,
    16 /*tm*/, 9 /*&*/,  4 /*'*/,  12,       12,       7,        5,
    7,         7,        7,        7,        7,        7,        7,
    7,         16,       14,       14,       14,       16,       16,
    16,        16,       16,       12,       14,       8,        8,
    8,         8,        8,        8,        8
};

static int8_t TextRemapASCII[95] = {
    0 /* */,   64 /*!*/,  66 /*"*/,  78 /*#*/, 77 /*$*/, 74 /*%*/, 78 /*&*/,
    79 /*'*/,  69 /*(*/,  70 /*)*/,  92 /***/, 72 /*+*/, 63 /*,*/, 71 /*-*/,
    62 /*.*/,  68 /**/,   52 /*0*/,  53 /*1*/, 54 /*2*/, 55 /*3*/, 56 /*4*/,
    57 /*5*/,  58 /*6*/,  59 /*7*/,  60 /*8*/, 61 /*9*/, 73 /*:*/, 73 /*;*/,
    66 /*<*/,  74 /*=*/,  75 /*>*/,  65 /*?*/, 0 /**/,   0 /*A*/,  1 /*B*/,
    2 /*C*/,   3 /*D*/,   4 /*E*/,   5 /*F*/,  6 /*G*/,  7 /*H*/,  8 /*I*/,
    9 /*J*/,   10 /*K*/,  11 /*L*/,  12 /*M*/, 13 /*N*/, 14 /*O*/, 15 /*P*/,
    16 /*Q*/,  17 /*R*/,  18 /*S*/,  19 /*T*/, 20 /*U*/, 21 /*V*/, 22 /*W*/,
    23 /*X*/,  24 /*Y*/,  25 /*Z*/,  80 /*[*/, 76 /*\*/, 81 /*]*/, 97 /*^*/,
    98 /*_*/,  77 /*`*/,  26 /*a*/,  27 /*b*/, 28 /*c*/, 29 /*d*/, 30 /*e*/,
    31 /*f*/,  32 /*g*/,  33 /*h*/,  34 /*i*/, 35 /*j*/, 36 /*k*/, 37 /*l*/,
    38 /*m*/,  39 /*n*/,  40 /*o*/,  41 /*p*/, 42 /*q*/, 43 /*r*/, 44 /*s*/,
    45 /*t*/,  46 /*u*/,  47 /*v*/,  48 /*w*/, 49 /*x*/, 50 /*y*/, 51 /*z*/,
    100 /*{*/, 101 /*|*/, 102 /*}*/, 67 /*~*/
};

int __cdecl T_GetStringLen(const char* string)
{
    int len = 1;
    while (*string++) {
        len++;
    }
    return len;
}

TEXTSTRING* __cdecl T_Print(
    int16_t xpos, int16_t ypos, int16_t zpos, const char* string)
{
    if (TextStringCount == MAX_TEXT_STRINGS) {
        return NULL;
    }

    TEXTSTRING* result = &TextInfoTable[0];
    int n;
    for (n = 0; n < MAX_TEXT_STRINGS; n++) {
        if (!(result->flags & TF_ACTIVE)) {
            break;
        }
        result++;
    }
    if (n >= MAX_TEXT_STRINGS) {
        return NULL;
    }

    if (!string) {
        return NULL;
    }

    int length = T_GetStringLen(string);
    if (length >= MAX_STRING_SIZE) {
        length = MAX_STRING_SIZE - 1;
    }

    result->xpos = xpos;
    result->ypos = ypos;
    result->zpos = zpos;
    result->letter_spacing = 1;
    result->word_spacing = 6;
    result->scale_h = 0x10000;
    result->scale_v = 0x10000;

    result->string = TextStrings[n];
    memcpy(result->string, string, length + 1);

    result->flags = TF_ACTIVE;
    result->text_flags = 0;
    result->outl_flags = 0;
    result->bgnd_flags = 0;

    result->bgnd_size_x = 0;
    result->bgnd_size_y = 0;
    result->bgnd_off_x = 0;
    result->bgnd_off_y = 0;
    result->bgnd_off_z = 0;

    TextStringCount++;

    return result;
}

void __cdecl T_ChangeText(TEXTSTRING* textstring, const char* string)
{
    if (!(textstring->flags & TF_ACTIVE)) {
        return;
    }
    strncpy(textstring->string, string, MAX_STRING_SIZE);
    if (T_GetStringLen(string) > MAX_STRING_SIZE) {
        textstring->string[MAX_STRING_SIZE - 1] = '\0';
    }
}

void __cdecl T_SetScale(
    TEXTSTRING* textstring, int32_t scale_h, int32_t scale_v)
{
    textstring->scale_h = scale_h;
    textstring->scale_v = scale_v;
}

void __cdecl T_FlashText(TEXTSTRING* textstring, int16_t b, int16_t rate)
{
    if (b) {
        textstring->flags |= TF_FLASH;
        textstring->flash_rate = rate;
        textstring->flash_count = rate;
    } else {
        textstring->flags &= ~TF_FLASH;
    }
}

void __cdecl T_AddBackground(
    TEXTSTRING* textstring, int16_t xsize, int16_t ysize, int16_t xoff,
    int16_t yoff, int16_t zoff, int16_t colour, SG_COL* gourptr, int16_t flags)
{
    textstring->flags |= TF_BGND;
    textstring->bgnd_size_x = xsize;
    textstring->bgnd_size_y = ysize;
    textstring->bgnd_off_x = xoff;
    textstring->bgnd_off_y = yoff;
    textstring->bgnd_off_z = zoff;
    textstring->bgnd_colour = colour;
    textstring->bgnd_gour = gourptr;
    textstring->bgnd_flags = flags;
}

void __cdecl T_RemoveBackground(TEXTSTRING* textstring)
{
    textstring->flags &= ~TF_BGND;
}

void __cdecl T_AddOutline(
    TEXTSTRING* textstring, int b, int16_t colour, SG_COL* gourptr,
    int16_t flags)
{
    textstring->flags |= TF_OUTLINE;
    textstring->outl_gour = gourptr;
    textstring->outl_colour = colour;
    textstring->outl_flags = flags;
}

void __cdecl T_RemoveOutline(TEXTSTRING* textstring)
{
    textstring->flags &= ~TF_OUTLINE;
}

void __cdecl T_CentreH(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_CENTRE_H;
    } else {
        textstring->flags &= ~TF_CENTRE_H;
    }
}

void __cdecl T_CentreV(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_CENTRE_V;
    } else {
        textstring->flags &= ~TF_CENTRE_V;
    }
}

void __cdecl T_RightAlign(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_RIGHT;
    } else {
        textstring->flags &= ~TF_RIGHT;
    }
}

void __cdecl T_BottomAlign(TEXTSTRING* textstring, int16_t b)
{
    if (b) {
        textstring->flags |= TF_BOTTOM;
    } else {
        textstring->flags &= ~TF_BOTTOM;
    }
}

int32_t __cdecl T_GetTextWidth(TEXTSTRING* textstring)
{
    int width = 0;
    char* ptr = textstring->string;
    for (char letter = *ptr; *ptr; letter = *ptr++) {
        if (letter == '\x7F' || letter > 10 && letter < 32) {
            continue;
        }

        if (letter == ' ') {
            if (textstring->scale_h == 0x10000) {
                width += textstring->word_spacing;
            } else {
                width += (textstring->word_spacing * textstring->scale_h) >> 16;
            }
            continue;
        }

        if (letter >= 11) {
            if (letter >= 16)
                letter = TextRemapASCII[letter - 32];
            else
                letter = letter + 91;
        } else {
            letter = letter + 81;
        }

        if (textstring->scale_h == 0x10000) {
            width += textstring->letter_spacing + TextSpacing[letter];
        } else {
            width +=
                (((int32_t)textstring->letter_spacing + TextSpacing[letter])
                 * textstring->scale_h)
                >> 16;
        }
    }
    width -= textstring->letter_spacing;
    width &= 0xFFFE;
    return width;
}

void __cdecl T_RemovePrint(TEXTSTRING* textstring)
{
    if (!textstring) {
        return;
    }

    if (textstring->flags & TF_ACTIVE) {
        textstring->flags &= ~TF_ACTIVE;
        --TextStringCount;
    }
}

void __cdecl T_DrawText()
{
    // TombATI FPS counter, pretty pointless IMO as it always shows 30 for me.
    // Additionally, it's not present in TR2+.
    static TEXTSTRING* fps_text = NULL;
    static char fps_buf[20];
    static int fps_counter1 = 0;
    static int fps_counter2 = 0;

    if (AppSettings & ASF_FPS) {
        int fps = ++fps_counter1;
        int tmp = Camera.number_frames + fps_counter2;
        fps_counter2 += Camera.number_frames;
        if (tmp >= 60) {
            sprintf(fps_buf, "%d FPS", fps);
            if (fps_text) {
                T_ChangeText(fps_text, fps_buf);
                fps_counter1 = 0;
                fps_counter2 = 0;
            } else {
                int fps_x;
                int fps_y;
                if (TR1MGetOverlayScale(1) > 1) {
                    fps_x = TR1MData.fps_x;
                    fps_y = TR1MData.fps_y;
                } else {
                    fps_x = 10;
                    fps_y = 30;
                }
                fps_text = T_Print(fps_x, fps_y, 0, fps_buf);
                fps_counter1 = 0;
                fps_counter2 = 0;
            }
        }
        if (Camera.number_frames > 30 && fps_text) {
            if (fps_text->flags & TF_ACTIVE) {
                fps_text->flags &= ~TF_ACTIVE;
                --TextStringCount;
            }
            fps_text = NULL;
        }
    } else if (fps_text) {
        if (fps_text->flags & TF_ACTIVE) {
            fps_text->flags &= ~TF_ACTIVE;
            --TextStringCount;
        }
        fps_text = NULL;
    }

    for (int i = 0; i < MAX_TEXT_STRINGS; i++) {
        TEXTSTRING* textstring = &TextInfoTable[i];
        if (textstring->flags & TF_ACTIVE) {
            T_DrawThisText(textstring);
        }
    }
}

void TR1MInjectText()
{
    INJECT(0x00439780, T_Print);
    INJECT(0x00439860, T_ChangeText);
    INJECT(0x004398A0, T_SetScale);
    INJECT(0x004398C0, T_FlashText);
    INJECT(0x004398F0, T_AddBackground);
    INJECT(0x00439950, T_RemoveBackground);
    INJECT(0x00439960, T_AddOutline);
    INJECT(0x00439990, T_RemoveOutline);
    INJECT(0x004399A0, T_CentreH);
    INJECT(0x004399C0, T_CentreV);
    INJECT(0x004399E0, T_RightAlign);
    INJECT(0x00439A00, T_BottomAlign);
    INJECT(0x00439A20, T_GetTextWidth);
    INJECT(0x00439AD0, T_RemovePrint);
    INJECT(0x00439B00, T_DrawText);
}
