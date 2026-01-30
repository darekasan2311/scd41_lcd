/*******************************************************************************
 * Size: 24 px
 * Bpp: 1
 * Opts: --bpp 1 --size 24 --no-compress --stride 1 --align 1 --font JetBrainsMono-Regular.ttf --symbols CO2%°pm --format lvgl -o jb_mono_reg_24.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef JB_MONO_REG_24
#define JB_MONO_REG_24 1
#endif

#if JB_MONO_REG_24

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0025 "%" */
    0x7c, 0xf, 0xf8, 0x6c, 0x63, 0x31, 0x8c, 0xc6,
    0x63, 0x1b, 0xf, 0xec, 0x1f, 0x60, 0x3, 0x0,
    0xc, 0x0, 0x6f, 0x83, 0x7f, 0xd, 0x8c, 0x66,
    0x33, 0x18, 0xcc, 0x63, 0x61, 0xff, 0x3, 0xe0,

    /* U+0032 "2" */
    0x1f, 0xf, 0xf9, 0xc3, 0x60, 0x3c, 0x6, 0x0,
    0xc0, 0x18, 0x6, 0x0, 0xc0, 0x30, 0xc, 0x3,
    0x0, 0xc0, 0x30, 0xc, 0x3, 0x0, 0x7f, 0xef,
    0xfc,

    /* U+0043 "C" */
    0x1e, 0x1f, 0xe6, 0x1b, 0x3, 0xc0, 0xf0, 0xc,
    0x3, 0x0, 0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0,
    0x30, 0x3c, 0xd, 0x86, 0x7f, 0x87, 0x80,

    /* U+004F "O" */
    0x1e, 0x1f, 0xe6, 0x1b, 0x3, 0xc0, 0xf0, 0x3c,
    0xf, 0x3, 0xc0, 0xf0, 0x3c, 0xf, 0x3, 0xc0,
    0xf0, 0x3c, 0xd, 0x86, 0x7f, 0x8f, 0xc0,

    /* U+006D "m" */
    0xdd, 0xdf, 0xff, 0x19, 0xe3, 0x3c, 0x67, 0x8c,
    0xf1, 0x9e, 0x33, 0xc6, 0x78, 0xcf, 0x19, 0xe3,
    0x3c, 0x67, 0x8c, 0xc0,

    /* U+0070 "p" */
    0xcf, 0x37, 0xee, 0x1f, 0x3, 0xc0, 0xf0, 0x3c,
    0xf, 0x3, 0xc0, 0xf0, 0x3c, 0xf, 0x87, 0xdf,
    0xb3, 0xcc, 0x3, 0x0, 0xc0, 0x30, 0x0,

    /* U+00B0 "°" */
    0x38, 0x8b, 0x1e, 0x3c, 0x6f, 0x8e, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 230, .box_w = 14, .box_h = 18, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 230, .box_w = 11, .box_h = 18, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 230, .box_w = 10, .box_h = 18, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 230, .box_w = 10, .box_h = 18, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 230, .box_w = 11, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 230, .box_w = 10, .box_h = 18, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 146, .adv_w = 230, .box_w = 7, .box_h = 7, .ofs_x = 4, .ofs_y = 11}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xd, 0x1e, 0x2a, 0x48, 0x4b, 0x8b
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 37, .range_length = 140, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 7, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};

extern const lv_font_t lv_font_montserrat_14;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t jb_mono_reg_24 = {
#else
lv_font_t jb_mono_reg_24 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 22,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -4,
    .underline_thickness = 1,
#endif
    .static_bitmap = 0,
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &lv_font_montserrat_14,
#endif
    .user_data = NULL,
};



#endif /*#if JB_MONO_REG_24*/
