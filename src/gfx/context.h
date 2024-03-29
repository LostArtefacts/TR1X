#pragma once

#include "gfx/2d/2d_renderer.h"
#include "gfx/3d/3d_renderer.h"
#include "gfx/common.h"
#include "gfx/fbo/fbo_renderer.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct GFX_CONTEXT GFX_CONTEXT;

void GFX_Context_Attach(void *window_handle);
void GFX_Context_Detach(void);
void GFX_Context_SetVSync(bool vsync);
void GFX_Context_SetWindowSize(int32_t width, int32_t height);
void GFX_Context_SetDisplaySize(int32_t width, int32_t height);
void GFX_Context_SetRenderingMode(GFX_RENDER_MODE target_mode);
int32_t GFX_Context_GetDisplayWidth(void);
int32_t GFX_Context_GetDisplayHeight(void);
void GFX_Context_SwapBuffers(void);
void GFX_Context_SetRendered(void);
bool GFX_Context_IsRendered(void);
void GFX_Context_ScheduleScreenshot(const char *path);
GFX_2D_Renderer *GFX_Context_GetRenderer2D(void);
GFX_3D_Renderer *GFX_Context_GetRenderer3D(void);
GFX_FBO_Renderer *GFX_Context_GetRendererFBO(void);
