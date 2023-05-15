#include "game/output.h"

#include "config.h"
#include "game/clock.h"
#include "game/picture.h"
#include "game/random.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/math_misc.h"
#include "math/matrix.h"
#include "specific/s_output.h"
#include "specific/s_shell.h"
#include "util.h"

#include <stddef.h>

static bool m_IsWibbleEffect = false;
static bool m_IsWaterEffect = false;
static bool m_IsShadeEffect = false;
static int m_OverlayCurAlpha = 0;
static int m_OverlayDstAlpha = 0;
static int m_BackdropCurAlpha = 0;
static int m_BackdropDstAlpha = 0;
static double m_FadeSpeed = 1.0;

static int32_t m_WibbleOffset = 0;
static int32_t m_WibbleTable[WIBBLE_SIZE] = { 0 };
static int32_t m_ShadeTable[WIBBLE_SIZE] = { 0 };
static int32_t m_RandTable[WIBBLE_SIZE] = { 0 };

static PHD_VBUF m_VBuf[1500] = { 0 };
static int32_t m_DrawDistFade = 0;
static int32_t m_DrawDistMax = 0;
static RGBF m_WaterColor = { 0 };
static PHD_VECTOR m_LsVectorView = { 0 };

static void Output_DrawBlackOverlay(uint8_t alpha);
static void Output_FadeAnimate(int ticks);

static const int16_t *Output_DrawObjectG3(
    const int16_t *obj_ptr, int32_t number);
static const int16_t *Output_DrawObjectG4(
    const int16_t *obj_ptr, int32_t number);
static const int16_t *Output_DrawObjectGT3(
    const int16_t *obj_ptr, int32_t number);
static const int16_t *Output_DrawObjectGT4(
    const int16_t *obj_ptr, int32_t number);
static const int16_t *Output_DrawRoomSprites(
    const int16_t *obj_ptr, int32_t vertex_count);
static const int16_t *Output_CalcObjectVertices(const int16_t *obj_ptr);
static const int16_t *Output_CalcVerticeLight(const int16_t *obj_ptr);
static const int16_t *Output_CalcRoomVertices(const int16_t *obj_ptr);
static int32_t Output_CalcFogShade(int32_t depth);
static void Output_CalcWibbleTable(void);

static const int16_t *Output_DrawObjectG3(
    const int16_t *obj_ptr, int32_t number)
{
    S_Output_DisableTextureMode();

    for (int i = 0; i < number; i++) {
        PHD_VBUF *vns[3] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        uint8_t color_idx = *obj_ptr++;
        RGB888 color = Output_GetPaletteColor(color_idx);
        S_Output_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
    }

    return obj_ptr;
}

static const int16_t *Output_DrawObjectG4(
    const int16_t *obj_ptr, int32_t number)
{
    S_Output_DisableTextureMode();

    for (int i = 0; i < number; i++) {
        PHD_VBUF *vns[4] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        uint8_t color_idx = *obj_ptr++;
        RGB888 color = Output_GetPaletteColor(color_idx);
        S_Output_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
        S_Output_DrawFlatTriangle(vns[2], vns[3], vns[0], color);
    }

    return obj_ptr;
}

static const int16_t *Output_DrawObjectGT3(
    const int16_t *obj_ptr, int32_t number)
{
    S_Output_EnableTextureMode();

    for (int i = 0; i < number; i++) {
        PHD_VBUF *vns[3] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        PHD_TEXTURE *tex = &g_PhdTextureInfo[*obj_ptr++];

        S_Output_DrawTexturedTriangle(
            vns[0], vns[1], vns[2], tex->tpage, &tex->uv[0], &tex->uv[1],
            &tex->uv[2], tex->drawtype);
    }

    return obj_ptr;
}

static const int16_t *Output_DrawObjectGT4(
    const int16_t *obj_ptr, int32_t number)
{
    S_Output_EnableTextureMode();

    for (int i = 0; i < number; i++) {
        PHD_VBUF *vns[4] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        PHD_TEXTURE *tex = &g_PhdTextureInfo[*obj_ptr++];

        S_Output_DrawTexturedQuad(
            vns[0], vns[1], vns[2], vns[3], tex->tpage, &tex->uv[0],
            &tex->uv[1], &tex->uv[2], &tex->uv[3], tex->drawtype);
    }

    return obj_ptr;
}

static const int16_t *Output_DrawRoomSprites(
    const int16_t *obj_ptr, int32_t vertex_count)
{
    for (int i = 0; i < vertex_count; i++) {
        int16_t vbuf_num = obj_ptr[0];
        int16_t sprnum = obj_ptr[1];
        obj_ptr += 2;

        PHD_VBUF *vbuf = &m_VBuf[vbuf_num];
        if (vbuf->clip < 0) {
            continue;
        }

        int32_t zv = vbuf->zv;
        PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
        int32_t zp = (zv / g_PhdPersp);
        int32_t x1 =
            Viewport_GetCenterX() + (vbuf->xv + (sprite->x1 << W2V_SHIFT)) / zp;
        int32_t y1 =
            Viewport_GetCenterY() + (vbuf->yv + (sprite->y1 << W2V_SHIFT)) / zp;
        int32_t x2 =
            Viewport_GetCenterX() + (vbuf->xv + (sprite->x2 << W2V_SHIFT)) / zp;
        int32_t y2 =
            Viewport_GetCenterY() + (vbuf->yv + (sprite->y2 << W2V_SHIFT)) / zp;
        if (x2 >= g_PhdLeft && y2 >= g_PhdTop && x1 < g_PhdRight
            && y1 < g_PhdBottom) {
            S_Output_DrawSprite(x1, y1, x2, y2, zv, sprnum, vbuf->g);
        }
    }

    return obj_ptr;
}

static const int16_t *Output_CalcObjectVertices(const int16_t *obj_ptr)
{
    int16_t total_clip = -1;

    obj_ptr++;
    int vertex_count = *obj_ptr++;
    for (int i = 0; i < vertex_count; i++) {
        double xv = g_MatrixPtr->_00 * obj_ptr[0]
            + g_MatrixPtr->_01 * obj_ptr[1] + g_MatrixPtr->_02 * obj_ptr[2]
            + g_MatrixPtr->_03;
        double yv = g_MatrixPtr->_10 * obj_ptr[0]
            + g_MatrixPtr->_11 * obj_ptr[1] + g_MatrixPtr->_12 * obj_ptr[2]
            + g_MatrixPtr->_13;
        int32_t zv_int = g_MatrixPtr->_20 * obj_ptr[0]
            + g_MatrixPtr->_21 * obj_ptr[1] + g_MatrixPtr->_22 * obj_ptr[2]
            + g_MatrixPtr->_23;
        double zv = zv_int;
        m_VBuf[i].xv = xv;
        m_VBuf[i].yv = yv;
        m_VBuf[i].zv = zv;

        int32_t clip_flags;
        if (zv < Output_GetNearZ()) {
            clip_flags = -32768;
        } else {
            clip_flags = 0;

            double persp = g_PhdPersp / zv;
            double xs = Viewport_GetCenterX() + xv * persp;
            double ys = Viewport_GetCenterY() + yv * persp;

            if (xs < g_PhdLeft) {
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
                clip_flags |= 8;
            }

            m_VBuf[i].xs = xs;
            m_VBuf[i].ys = ys;
        }

        m_VBuf[i].clip = clip_flags;
        total_clip &= clip_flags;
        obj_ptr += 3;
    }

    return total_clip == 0 ? obj_ptr : NULL;
}

static const int16_t *Output_CalcVerticeLight(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;
    if (vertex_count > 0) {
        if (g_LsDivider) {
            int32_t xv = (g_MatrixPtr->_00 * m_LsVectorView.x
                          + g_MatrixPtr->_10 * m_LsVectorView.y
                          + g_MatrixPtr->_20 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t yv = (g_MatrixPtr->_01 * m_LsVectorView.x
                          + g_MatrixPtr->_11 * m_LsVectorView.y
                          + g_MatrixPtr->_21 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t zv = (g_MatrixPtr->_02 * m_LsVectorView.x
                          + g_MatrixPtr->_12 * m_LsVectorView.y
                          + g_MatrixPtr->_22 * m_LsVectorView.z)
                / g_LsDivider;
            for (int i = 0; i < vertex_count; i++) {
                int16_t shade = g_LsAdder
                    + ((obj_ptr[0] * xv + obj_ptr[1] * yv + obj_ptr[2] * zv)
                       >> 16);
                CLAMP(shade, 0, 0x1FFF);
                m_VBuf[i].g = shade;
                obj_ptr += 3;
            }
            return obj_ptr;
        } else {
            int16_t shade = g_LsAdder;
            CLAMP(shade, 0, 0x1FFF);
            for (int i = 0; i < vertex_count; i++) {
                m_VBuf[i].g = shade;
            }
            obj_ptr += 3 * vertex_count;
        }
    } else {
        for (int i = 0; i < -vertex_count; i++) {
            int16_t shade = g_LsAdder + *obj_ptr++;
            CLAMP(shade, 0, 0x1FFF);
            m_VBuf[i].g = shade;
        }
    }
    return obj_ptr;
}

static const int16_t *Output_CalcRoomVertices(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;

    for (int i = 0; i < vertex_count; i++) {
        double xv = g_MatrixPtr->_00 * obj_ptr[0]
            + g_MatrixPtr->_01 * obj_ptr[1] + g_MatrixPtr->_02 * obj_ptr[2]
            + g_MatrixPtr->_03;
        double yv = g_MatrixPtr->_10 * obj_ptr[0]
            + g_MatrixPtr->_11 * obj_ptr[1] + g_MatrixPtr->_12 * obj_ptr[2]
            + g_MatrixPtr->_13;
        int32_t zv_int = g_MatrixPtr->_20 * obj_ptr[0]
            + g_MatrixPtr->_21 * obj_ptr[1] + g_MatrixPtr->_22 * obj_ptr[2]
            + g_MatrixPtr->_23;
        double zv = zv_int;
        m_VBuf[i].xv = xv;
        m_VBuf[i].yv = yv;
        m_VBuf[i].zv = zv;
        m_VBuf[i].g = obj_ptr[3];

        if (zv < Output_GetNearZ()) {
            m_VBuf[i].clip = 0x8000;
        } else {
            int16_t clip_flags = 0;
            int32_t depth = zv_int >> W2V_SHIFT;
            if (depth > Output_GetDrawDistMax()) {
                m_VBuf[i].g = 0x1FFF;
                clip_flags |= 16;
            } else if (depth) {
                m_VBuf[i].g += Output_CalcFogShade(depth);
                if (!m_IsWaterEffect) {
                    CLAMPG(m_VBuf[i].g, 0x1FFF);
                }
            }

            double persp = g_PhdPersp / zv;
            double xs = Viewport_GetCenterX() + xv * persp;
            double ys = Viewport_GetCenterY() + yv * persp;
            if (m_IsWibbleEffect) {
                xs += m_WibbleTable[(m_WibbleOffset + (int)ys) & 0x1F];
                ys += m_WibbleTable[(m_WibbleOffset + (int)xs) & 0x1F];
            }

            if (xs < g_PhdLeft) {
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
                clip_flags |= 8;
            }

            if (m_IsWaterEffect) {
                m_VBuf[i].g += m_ShadeTable[(
                    ((uint8_t)m_WibbleOffset
                     + (uint8_t)m_RandTable[(vertex_count - i) % WIBBLE_SIZE])
                    % WIBBLE_SIZE)];
                CLAMP(m_VBuf[i].g, 0, 0x1FFF);
            }

            m_VBuf[i].xs = xs;
            m_VBuf[i].ys = ys;
            m_VBuf[i].clip = clip_flags;
        }
        obj_ptr += 4;
    }

    return obj_ptr;
}

static int32_t Output_CalcFogShade(int32_t depth)
{
    int32_t fog_begin = Output_GetDrawDistFade();
    int32_t fog_end = Output_GetDrawDistMax();

    if (depth < fog_begin) {
        return 0;
    }
    if (depth >= fog_end) {
        return 0x1FFF;
    }

    return (depth - fog_begin) * 0x1FFF / (fog_end - fog_begin);
}

static void Output_CalcWibbleTable(void)
{
    for (int i = 0; i < WIBBLE_SIZE; i++) {
        PHD_ANGLE angle = (i * PHD_360) / WIBBLE_SIZE;
        m_WibbleTable[i] = Math_Sin(angle) * MAX_WIBBLE >> W2V_SHIFT;
        m_ShadeTable[i] = Math_Sin(angle) * MAX_SHADE >> W2V_SHIFT;
        m_RandTable[i] = (Random_GetDraw() >> 5) - 0x01FF;
    }
}

bool Output_Init(void)
{
    Output_CalcWibbleTable();
    return S_Output_Init();
}

void Output_Shutdown(void)
{
    S_Output_Shutdown();
}

void Output_SetViewport(int width, int height)
{
    S_Output_SetViewport(width, height);
}

void Output_SetFullscreen(bool fullscreen)
{
    S_Output_SetFullscreen(fullscreen);
}

void Output_ApplyResolution(void)
{
    S_Output_ApplyResolution();
}

void Output_DownloadTextures(int page_count)
{
    S_Output_DownloadTextures(page_count);
}

RGBA8888 Output_RGB2RGBA(const RGB888 color)
{
    RGBA8888 ret = { .r = color.r, .g = color.g, .b = color.b, .a = 255 };
    return ret;
}

void Output_SetPalette(RGB888 palette[256])
{
    S_Output_SetPalette(palette);
}

RGB888 Output_GetPaletteColor(uint8_t idx)
{
    return S_Output_GetPaletteColor(idx);
}

void Output_DrawBlack(void)
{
    Output_DrawBlackOverlay(255);
}

void Output_DrawEmpty(void)
{
    S_Output_DrawEmpty();
}

int32_t Output_DumpScreen(void)
{
    Output_DrawOverlayScreen();
    S_Output_DumpScreen();
    S_Shell_SpinMessageLoop();
    g_FPSCounter++;
    int ticks = Clock_SyncTicks(TICKS_PER_FRAME);
    Output_FadeAnimate(ticks);
    return ticks;
}

void Output_CalculateLight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    ROOM_INFO *r = &g_RoomInfo[room_num];

    if (r->num_lights == 0) {
        g_LsAdder = r->ambient;
        g_LsDivider = 0;
    } else {
        int32_t ambient = 0x1FFF - r->ambient;
        int32_t brightest = 0;

        PHD_VECTOR ls = { 0, 0, 0 };
        for (int i = 0; i < r->num_lights; i++) {
            LIGHT_INFO *light = &r->light[i];
            PHD_VECTOR lc;
            lc.x = x - light->x;
            lc.y = y - light->y;
            lc.z = z - light->z;

            int32_t distance =
                (SQUARE(lc.x) + SQUARE(lc.y) + SQUARE(lc.z)) >> 12;
            int32_t falloff = SQUARE(light->falloff) >> 12;
            int32_t shade =
                ambient + (light->intensity * falloff) / (distance + falloff);

            if (shade > brightest) {
                brightest = shade;
                ls = lc;
            }
        }

        g_LsAdder = (ambient + brightest) / 2;
        if (brightest == ambient) {
            g_LsDivider = 0;
        } else {
            g_LsDivider = (1 << (W2V_SHIFT + 12)) / (brightest - g_LsAdder);
        }
        g_LsAdder = 0x1FFF - g_LsAdder;

        PHD_ANGLE angles[2];
        Math_GetVectorAngles(ls.x, ls.y, ls.z, angles);
        Output_RotateLight(angles[1], angles[0]);
    }

    int32_t distance = g_MatrixPtr->_23 >> W2V_SHIFT;
    g_LsAdder += Output_CalcFogShade(distance);
    CLAMPG(g_LsAdder, 0x1FFF);
}

void Output_CalculateStaticLight(int16_t adder)
{
    g_LsAdder = adder - 16 * 256;
    int32_t distance = g_MatrixPtr->_23 >> W2V_SHIFT;
    g_LsAdder += Output_CalcFogShade(distance);
    CLAMPG(g_LsAdder, 0x1FFF);
}

void Output_CalculateObjectLighting(ITEM_INFO *item, int16_t *frame)
{
    if (item->shade >= 0) {
        Output_CalculateStaticLight(item->shade);
        return;
    }

    Matrix_PushUnit();
    g_MatrixPtr->_23 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_03 = 0;

    Matrix_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    Matrix_TranslateRel(
        (frame[FRAME_BOUND_MIN_X] + frame[FRAME_BOUND_MAX_X]) / 2,
        (frame[FRAME_BOUND_MIN_Y] + frame[FRAME_BOUND_MAX_Y]) / 2,
        (frame[FRAME_BOUND_MIN_Z] + frame[FRAME_BOUND_MAX_Z]) / 2);

    int32_t x = (g_MatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
    int32_t y = (g_MatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
    int32_t z = (g_MatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;

    Matrix_Pop();

    Output_CalculateLight(x, y, z, item->room_number);
}

void Output_DrawPolygons(const int16_t *obj_ptr, int clip)
{
    obj_ptr += 4;
    obj_ptr = Output_CalcObjectVertices(obj_ptr);
    if (obj_ptr) {
        obj_ptr = Output_CalcVerticeLight(obj_ptr);
        obj_ptr = Output_DrawObjectGT4(obj_ptr + 1, *obj_ptr);
        obj_ptr = Output_DrawObjectGT3(obj_ptr + 1, *obj_ptr);
        obj_ptr = Output_DrawObjectG4(obj_ptr + 1, *obj_ptr);
        obj_ptr = Output_DrawObjectG3(obj_ptr + 1, *obj_ptr);
    }
}

void Output_DrawPolygons_I(const int16_t *obj_ptr, int32_t clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawPolygons(obj_ptr, clip);
    Matrix_Pop();
}

void Output_DrawRoom(const int16_t *obj_ptr)
{
    obj_ptr = Output_CalcRoomVertices(obj_ptr);
    obj_ptr = Output_DrawObjectGT4(obj_ptr + 1, *obj_ptr);
    obj_ptr = Output_DrawObjectGT3(obj_ptr + 1, *obj_ptr);
    obj_ptr = Output_DrawRoomSprites(obj_ptr + 1, *obj_ptr);
}

void Output_DrawShadow(int16_t size, int16_t *bptr, ITEM_INFO *item)
{
    int i;

    g_ShadowInfo.vertex_count = g_Config.enable_round_shadow ? 32 : 8;

    int32_t x0 = bptr[FRAME_BOUND_MIN_X];
    int32_t x1 = bptr[FRAME_BOUND_MAX_X];
    int32_t z0 = bptr[FRAME_BOUND_MIN_Z];
    int32_t z1 = bptr[FRAME_BOUND_MAX_Z];

    int32_t x_mid = (x0 + x1) / 2;
    int32_t z_mid = (z0 + z1) / 2;

    int32_t x_add = (x1 - x0) * size / 1024;
    int32_t z_add = (z1 - z0) * size / 1024;

    for (i = 0; i < g_ShadowInfo.vertex_count; i++) {
        int32_t angle = (PHD_180 + i * PHD_360) / g_ShadowInfo.vertex_count;
        g_ShadowInfo.vertex[i].x =
            x_mid + (x_add * 2) * Math_Sin(angle) / PHD_90;
        g_ShadowInfo.vertex[i].z =
            z_mid + (z_add * 2) * Math_Cos(angle) / PHD_90;
        g_ShadowInfo.vertex[i].y = 0;
    }

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, item->floor, item->pos.z);
    Matrix_RotY(item->pos.y_rot);

    if (Output_CalcObjectVertices(&g_ShadowInfo.poly_count)) {
        int16_t clip_and = 1;
        int16_t clip_positive = 1;
        int16_t clip_or = 0;
        for (i = 0; i < g_ShadowInfo.vertex_count; i++) {
            clip_and &= m_VBuf[i].clip;
            clip_positive &= m_VBuf[i].clip >= 0;
            clip_or |= m_VBuf[i].clip;
        }
        PHD_VBUF *vn1 = &m_VBuf[0];
        PHD_VBUF *vn2 = &m_VBuf[g_Config.enable_round_shadow ? 4 : 1];
        PHD_VBUF *vn3 = &m_VBuf[g_Config.enable_round_shadow ? 8 : 2];

        bool visible =
            ((int32_t)(((vn3->xs - vn2->xs) * (vn1->ys - vn2->ys)) - ((vn1->xs - vn2->xs) * (vn3->ys - vn2->ys)))
             >= 0);

        if (!clip_and && clip_positive && visible) {
            S_Output_DrawShadow(
                &m_VBuf[0], clip_or ? 1 : 0, g_ShadowInfo.vertex_count);
        }
    }

    Matrix_Pop();
}

int32_t Output_GetDrawDistMin(void)
{
    return 127;
}

int32_t Output_GetDrawDistFade(void)
{
    return m_DrawDistFade;
}

int32_t Output_GetDrawDistMax(void)
{
    return m_DrawDistMax;
}

void Output_SetDrawDistFade(int32_t dist)
{
    m_DrawDistFade = dist;
}

void Output_SetDrawDistMax(int32_t dist)
{
    m_DrawDistMax = dist;
}

void Output_SetWaterColor(const RGBF *color)
{
    m_WaterColor.r = color->r;
    m_WaterColor.g = color->g;
    m_WaterColor.b = color->b;
}

int32_t Output_GetNearZ(void)
{
    return Output_GetDrawDistMin() << W2V_SHIFT;
}

int32_t Output_GetFarZ(void)
{
    return Output_GetDrawDistMax() << W2V_SHIFT;
}

void Output_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    x -= g_W2VMatrix._03;
    y -= g_W2VMatrix._13;
    z -= g_W2VMatrix._23;

    if (x < -Output_GetDrawDistMax() || x > Output_GetDrawDistMax()) {
        return;
    }

    if (y < -Output_GetDrawDistMax() || y > Output_GetDrawDistMax()) {
        return;
    }

    if (z < -Output_GetDrawDistMax() || z > Output_GetDrawDistMax()) {
        return;
    }

    int32_t zv =
        g_W2VMatrix._20 * x + g_W2VMatrix._21 * y + g_W2VMatrix._22 * z;
    if (zv < Output_GetNearZ() || zv > Output_GetFarZ()) {
        return;
    }

    int32_t xv =
        g_W2VMatrix._00 * x + g_W2VMatrix._01 * y + g_W2VMatrix._02 * z;
    int32_t yv =
        g_W2VMatrix._10 * x + g_W2VMatrix._11 * y + g_W2VMatrix._12 * z;
    int32_t zp = zv / g_PhdPersp;

    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = Viewport_GetCenterX() + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = Viewport_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = Viewport_GetCenterX() + (xv + (sprite->x2 << W2V_SHIFT)) / zp;
    int32_t y2 = Viewport_GetCenterY() + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= Viewport_GetMinX() && y2 >= Viewport_GetMinY()
        && x1 <= Viewport_GetMaxX() && y1 <= Viewport_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += Output_CalcFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        S_Output_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void Output_DrawBackdropImage(void)
{
    S_Output_DrawBackdropSurface();
}

void Output_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 color)
{
    S_Output_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void Output_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 tl, RGBA8888 tr,
    RGBA8888 bl, RGBA8888 br)
{
    S_Output_Draw2DQuad(sx, sy, sx + w, sy + h, tl, tr, bl, br);
}

void Output_DrawScreenLine(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 col)
{
    S_Output_Draw2DLine(sx, sy, sx + w, sy + h, col, col);
}

void Output_DrawScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 colDark,
    RGBA8888 colLight, int32_t thickness)
{
    float scale = Viewport_GetHeight() / 480.0;
    S_Output_ScreenBox(
        sx - scale, sy - scale, w, h, colDark, colLight,
        thickness * scale / 2.0f);
}

void Output_DrawGradientScreenLine(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 col1, RGBA8888 col2)
{
    S_Output_Draw2DLine(sx, sy, sx + w, sy + h, col1, col2);
}

void Output_DrawGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 tl, RGBA8888 tr,
    RGBA8888 bl, RGBA8888 br, int32_t thickness)
{
    float scale = Viewport_GetHeight() / 480.0;
    S_Output_4ColourTextBox(
        sx - scale, sy - scale, w + scale, h + scale, tl, tr, bl, br,
        thickness * scale / 2.0f);
}

void Output_DrawCentreGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 edge,
    RGBA8888 center, int32_t thickness)
{
    float scale = Viewport_GetHeight() / 480.0;
    S_Output_2ToneColourTextBox(
        sx - scale, sy - scale, w + scale, h + scale, edge, center,
        thickness * scale / 2.0f);
}

void Output_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h)
{
    RGBA8888 color = { 0, 0, 0, 128 };
    S_Output_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int16_t sprnum, int16_t shade, uint16_t flags)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = sx + (scale_h * (sprite->x1 >> 3) / PHD_ONE);
    int32_t x2 = sx + (scale_h * (sprite->x2 >> 3) / PHD_ONE);
    int32_t y1 = sy + (scale_v * (sprite->y1 >> 3) / PHD_ONE);
    int32_t y2 = sy + (scale_v * (sprite->y2 >> 3) / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < Viewport_GetWidth()
        && y1 < Viewport_GetHeight()) {
        S_Output_DrawSprite(x1, y1, x2, y2, 8 * z, sprnum, shade);
    }
}

void Output_DrawScreenSprite2D(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int32_t page)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = sx + (scale_h * sprite->x1 / PHD_ONE);
    int32_t x2 = sx + (scale_h * sprite->x2 / PHD_ONE);
    int32_t y1 = sy + (scale_v * sprite->y1 / PHD_ONE);
    int32_t y2 = sy + (scale_v * sprite->y2 / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < Viewport_GetWidth()
        && y1 < Viewport_GetHeight()) {
        S_Output_DrawSprite(x1, y1, x2, y2, 200, sprnum, 0);
    }
}

void Output_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    int32_t zv = g_MatrixPtr->_20 * x + g_MatrixPtr->_21 * y
        + g_MatrixPtr->_22 * z + g_MatrixPtr->_23;
    if (zv < Output_GetNearZ() || zv > Output_GetFarZ()) {
        return;
    }

    int32_t xv = g_MatrixPtr->_00 * x + g_MatrixPtr->_01 * y
        + g_MatrixPtr->_02 * z + g_MatrixPtr->_03;
    int32_t yv = g_MatrixPtr->_10 * x + g_MatrixPtr->_11 * y
        + g_MatrixPtr->_12 * z + g_MatrixPtr->_13;
    int32_t zp = zv / g_PhdPersp;

    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = Viewport_GetCenterX() + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = Viewport_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = Viewport_GetCenterX() + (xv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t y2 = Viewport_GetCenterY() + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= Viewport_GetMinX() && y2 >= Viewport_GetMinY()
        && x1 <= Viewport_GetMaxX() && y1 <= Viewport_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += Output_CalcFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        S_Output_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void Output_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = x + (scale * sprite->x1 >> 16);
    int32_t x2 = x + (scale * sprite->x2 >> 16);
    int32_t y1 = y + (scale * sprite->y1 >> 16);
    int32_t y2 = y + (scale * sprite->y2 >> 16);
    if (x2 >= Viewport_GetMinX() && y2 >= Viewport_GetMinY()
        && x1 <= Viewport_GetMaxX() && y1 <= Viewport_GetMaxY()) {
        S_Output_DrawSprite(x1, y1, x2, y2, 200, sprnum, shade);
    }
}

void Output_LoadBackdropImage(const char *filename)
{
    PICTURE *orig_pic = Picture_CreateFromFile(filename);
    if (orig_pic) {
        PICTURE *scaled_pic = Picture_ScaleSmart(
            orig_pic, Viewport_GetWidth(), Viewport_GetHeight());
        if (scaled_pic) {
            S_Output_DownloadBackdropSurface(scaled_pic);
            Picture_Free(scaled_pic);
        }
        Picture_Free(orig_pic);
    }
}

void Output_DrawLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2,
    int32_t width)
{
    if (z1 >= Output_GetNearZ() && z1 <= Output_GetFarZ()
        && z2 >= Output_GetNearZ() && z2 <= Output_GetFarZ()) {
        x1 = Viewport_GetCenterX() + x1 / (z1 / g_PhdPersp);
        y1 = Viewport_GetCenterY() + y1 / (z1 / g_PhdPersp);
        x2 = Viewport_GetCenterX() + x2 / (z2 / g_PhdPersp);
        y2 = Viewport_GetCenterY() + y2 / (z2 / g_PhdPersp);
        int32_t thickness1 = (width << W2V_SHIFT) / (z1 / g_PhdPersp);
        int32_t thickness2 = (width << W2V_SHIFT) / (z2 / g_PhdPersp);
        S_Output_DrawLightningSegment(
            x1, y1, z1, thickness1, x2, y2, z2, thickness2);
    }
}

void Output_SetupBelowWater(bool underwater)
{
    m_IsWaterEffect = true;
    m_IsWibbleEffect = !underwater;
    m_IsShadeEffect = true;
}

void Output_SetupAboveWater(bool underwater)
{
    m_IsWaterEffect = false;
    m_IsWibbleEffect = underwater;
    m_IsShadeEffect = underwater;
}

void Output_AnimateTextures(int32_t ticks)
{
    m_WibbleOffset = (m_WibbleOffset + ticks / TICKS_PER_FRAME) % WIBBLE_SIZE;

    static int32_t tick_comp = 0;
    tick_comp += ticks;

    while (tick_comp > TICKS_PER_FRAME * 5) {
        int16_t *ptr = g_AnimTextureRanges;
        int16_t i = *ptr++;
        while (i > 0) {
            int16_t j = *ptr++;
            PHD_TEXTURE temp = g_PhdTextureInfo[*ptr];
            while (j > 0) {
                g_PhdTextureInfo[ptr[0]] = g_PhdTextureInfo[ptr[1]];
                j--;
                ptr++;
            }
            g_PhdTextureInfo[*ptr] = temp;
            i--;
            ptr++;
        }
        tick_comp -= TICKS_PER_FRAME * 5;
    }
}

void Output_RotateLight(int16_t pitch, int16_t yaw)
{
    int32_t cp = Math_Cos(pitch);
    int32_t sp = Math_Sin(pitch);
    int32_t cy = Math_Cos(yaw);
    int32_t sy = Math_Sin(yaw);
    int32_t ls_x = TRIGMULT2(cp, sy);
    int32_t ls_y = -sp;
    int32_t ls_z = TRIGMULT2(cp, cy);
    m_LsVectorView.x = (g_W2VMatrix._00 * ls_x + g_W2VMatrix._01 * ls_y
                        + g_W2VMatrix._02 * ls_z)
        >> W2V_SHIFT;
    m_LsVectorView.y = (g_W2VMatrix._10 * ls_x + g_W2VMatrix._11 * ls_y
                        + g_W2VMatrix._12 * ls_z)
        >> W2V_SHIFT;
    m_LsVectorView.z = (g_W2VMatrix._20 * ls_x + g_W2VMatrix._21 * ls_y
                        + g_W2VMatrix._22 * ls_z)
        >> W2V_SHIFT;
}

static void Output_DrawBlackOverlay(uint8_t alpha)
{
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sw = Viewport_GetWidth();
    int32_t sh = Viewport_GetHeight();

    RGBA8888 background = { 0, 0, 0, alpha };
    S_Output_DisableDepthTest();
    S_Output_ClearDepthBuffer();
    Output_DrawScreenFlatQuad(sx, sy, sw, sh, background);
    S_Output_EnableDepthTest();
}

static void Output_FadeAnimate(int ticks)
{
    if (!g_Config.enable_fade_effects) {
        return;
    }

    const int delta = 5 * m_FadeSpeed * ticks;
    if (m_OverlayCurAlpha + delta <= m_OverlayDstAlpha) {
        m_OverlayCurAlpha += delta;
    } else if (m_OverlayCurAlpha - delta >= m_OverlayDstAlpha) {
        m_OverlayCurAlpha -= delta;
    } else {
        m_OverlayCurAlpha = m_OverlayDstAlpha;
    }
    if (m_BackdropCurAlpha + delta <= m_BackdropDstAlpha) {
        m_BackdropCurAlpha += delta;
    } else if (m_BackdropCurAlpha - delta >= m_BackdropDstAlpha) {
        m_BackdropCurAlpha -= delta;
    } else {
        m_BackdropCurAlpha = m_BackdropDstAlpha;
    }
}

void Output_DrawBackdropScreen(void)
{
    Output_DrawBlackOverlay(m_BackdropCurAlpha);
}

void Output_DrawOverlayScreen(void)
{
    Output_DrawBlackOverlay(m_OverlayCurAlpha);
}

void Output_FadeReset(void)
{
    m_BackdropCurAlpha = 0;
    m_OverlayCurAlpha = 0;
    m_BackdropDstAlpha = 0;
    m_OverlayDstAlpha = 0;
}

void Output_FadeSetSpeed(double speed)
{
    m_FadeSpeed = speed;
}

void Output_FadeResetToBlack(void)
{
    m_OverlayCurAlpha = 255;
    m_OverlayDstAlpha = 255;
}

void Output_FadeToBlack(bool allow_immediate)
{
    if (g_Config.enable_fade_effects) {
        m_OverlayDstAlpha = 255;
    } else if (allow_immediate) {
        m_OverlayCurAlpha = 255;
    }
}

void Output_FadeToSemiBlack(bool allow_immediate)
{
    if (g_Config.enable_fade_effects) {
        m_BackdropDstAlpha = 128;
        m_OverlayDstAlpha = 0;
    } else if (allow_immediate) {
        m_BackdropCurAlpha = 128;
        m_OverlayCurAlpha = 0;
    }
}

void Output_FadeToTransparent(bool allow_immediate)
{
    if (g_Config.enable_fade_effects) {
        m_BackdropDstAlpha = 0;
        m_OverlayDstAlpha = 0;
    } else if (allow_immediate) {
        m_BackdropCurAlpha = 0;
        m_OverlayCurAlpha = 0;
    }
}

bool Output_FadeIsAnimating(void)
{
    if (!g_Config.enable_fade_effects) {
        return false;
    }
    return m_OverlayCurAlpha != m_OverlayDstAlpha
        || m_BackdropCurAlpha != m_BackdropDstAlpha;
}

void Output_ApplyWaterEffect(float *r, float *g, float *b)
{
    if (m_IsShadeEffect) {
        *r *= m_WaterColor.r;
        *g *= m_WaterColor.g;
        *b *= m_WaterColor.b;
    }
}

bool Output_MakeScreenshot(const char *path)
{
    return S_Output_MakeScreenshot(path);
}

int Output_GetObjectBounds(int16_t *bptr)
{
    if (g_MatrixPtr->_23 >= Output_GetFarZ()) {
        return 0;
    }

    int32_t x_min = bptr[0];
    int32_t x_max = bptr[1];
    int32_t y_min = bptr[2];
    int32_t y_max = bptr[3];
    int32_t z_min = bptr[4];
    int32_t z_max = bptr[5];

    PHD_VECTOR vtx[8];
    vtx[0].x = x_min;
    vtx[0].y = y_min;
    vtx[0].z = z_min;
    vtx[1].x = x_max;
    vtx[1].y = y_min;
    vtx[1].z = z_min;
    vtx[2].x = x_max;
    vtx[2].y = y_max;
    vtx[2].z = z_min;
    vtx[3].x = x_min;
    vtx[3].y = y_max;
    vtx[3].z = z_min;
    vtx[4].x = x_min;
    vtx[4].y = y_min;
    vtx[4].z = z_max;
    vtx[5].x = x_max;
    vtx[5].y = y_min;
    vtx[5].z = z_max;
    vtx[6].x = x_max;
    vtx[6].y = y_max;
    vtx[6].z = z_max;
    vtx[7].x = x_min;
    vtx[7].y = y_max;
    vtx[7].z = z_max;

    int num_z = 0;
    x_min = 0x3FFFFFFF;
    y_min = 0x3FFFFFFF;
    x_max = -0x3FFFFFFF;
    y_max = -0x3FFFFFFF;

    for (int i = 0; i < 8; i++) {
        int32_t zv = g_MatrixPtr->_20 * vtx[i].x + g_MatrixPtr->_21 * vtx[i].y
            + g_MatrixPtr->_22 * vtx[i].z + g_MatrixPtr->_23;

        if (zv > Output_GetNearZ() && zv < Output_GetFarZ()) {
            ++num_z;
            int32_t zp = zv / g_PhdPersp;
            int32_t xv =
                (g_MatrixPtr->_00 * vtx[i].x + g_MatrixPtr->_01 * vtx[i].y
                 + g_MatrixPtr->_02 * vtx[i].z + g_MatrixPtr->_03)
                / zp;
            int32_t yv =
                (g_MatrixPtr->_10 * vtx[i].x + g_MatrixPtr->_11 * vtx[i].y
                 + g_MatrixPtr->_12 * vtx[i].z + g_MatrixPtr->_13)
                / zp;

            if (x_min > xv) {
                x_min = xv;
            } else if (x_max < xv) {
                x_max = xv;
            }

            if (y_min > yv) {
                y_min = yv;
            } else if (y_max < yv) {
                y_max = yv;
            }
        }
    }

    x_min += Viewport_GetCenterX();
    x_max += Viewport_GetCenterX();
    y_min += Viewport_GetCenterY();
    y_max += Viewport_GetCenterY();

    if (!num_z || x_min > g_PhdRight || y_min > g_PhdBottom || x_max < g_PhdLeft
        || y_max < g_PhdTop) {
        return 0; // out of screen
    }

    if (num_z < 8 || x_min < 0 || y_min < 0 || x_max > Viewport_GetMaxX()
        || y_max > Viewport_GetMaxY()) {
        return -1; // clipped
    }

    return 1; // fully on screen
}
