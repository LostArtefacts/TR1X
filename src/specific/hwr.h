#ifndef T1M_SPECIFIC_HWR_H
#define T1M_SPECIFIC_HWR_H

#include "global/types.h"
#include "specific/ati.h"

#include <windows.h>
#include <ddraw.h>
#include <stdint.h>

// TODO: function naming, Render vs. Draw vs. Insert
// TODO: port ATI3DCIF to actual D3D calls

// clang-format off
#define HWR_DownloadTextures        ((void      (*)(int16_t level_num))0x004084DE)
#define HWR_SetPalette              ((void      (*)())0x004087EA)
#define HWR_DrawSprite              ((void      (*)(int32_t x1, int32_t x2, int32_t y1, int32_t y2, int32_t z, int16_t sprnum, int16_t shade))0x0040C425)
#define HWR_InitPolyList            ((void      (*)())0x0040D0F7)
#define HWR_OutputPolyList          ((void      (*)())0x0040D2E0)
#define HWR_DrawFlatTriangle        ((void (*)(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int32_t color))0x00409C0F)
#define HWR_DrawTexturedTriangle    ((void (*)(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage, uint16_t *u1, uint16_t *u2, uint16_t *u3))0x0040B510)
#define HWR_DrawTexturedQuad        ((void (*)(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, uint16_t tpage, uint16_t *u1, uint16_t *u2, uint16_t *u3, uint16_t *u4))0x0040BBE2)
// clang-format on

void HWR_CheckError(HRESULT result);
void HWR_RenderBegin();
void HWR_RenderEnd();
void HWR_RenderToggle();
void HWR_GetSurfaceAndPitch(
    LPDIRECTDRAWSURFACE surface, LPVOID *out_surface, int32_t *out_pitch);
void HWR_DisableTextures();
void HWR_ClearSurface(LPDIRECTDRAWSURFACE surface);
void HWR_ReleaseSurfaces();
void HWR_DumpScreen();
void HWR_ClearSurfaceDepth();
void HWR_FlipPrimaryBuffer();
void HWR_FadeToPal(int32_t fade_value, RGB888 *palette);
void HWR_FadeWait();
void HWR_BlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source);
void HWR_CopyPicture();
void HWR_DownloadPicture();
void HWR_RenderTriangleStrip(C3D_VTCF *vertices, int num);
void HWR_SelectTexture(int tex_num);
void HWR_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 color1,
    RGB888 color2);
void HWR_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br);
void HWR_DrawTranslucentQuad(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void HWR_PrintShadow(PHD_VBUF *vbufs, int clip);
void HWR_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2);
int32_t HWR_ClipVertices(int32_t num, C3D_VTCF *source);
int32_t HWR_ClipVertices2(int32_t num, C3D_VTCF *source);
void HWR_SwitchResolution();
int32_t HWR_SetHardwareVideoMode();
void HWR_InitialiseHardware();
void HWR_ShutdownHardware();
void HWR_PrepareFMV();
void HWR_FMVDone();
void HWR_FMVInit();
void HWR_SetupRenderContextAndRender();

const int16_t *HWR_InsertObjectG3(const int16_t *obj_ptr, int32_t number);
const int16_t *HWR_InsertObjectG4(const int16_t *obj_ptr, int32_t number);
const int16_t *HWR_InsertObjectGT3(const int16_t *obj_ptr, int32_t number);
const int16_t *HWR_InsertObjectGT4(const int16_t *obj_ptr, int32_t number);

void T1MInjectSpecificHWR();

#endif
