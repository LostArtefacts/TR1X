#include "3dsystem/3d_gen.h"

#include "config.h"
#include "game/output.h"
#include "game/screen.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"

#include <math.h>

void phd_RotateLight(int16_t pitch, int16_t yaw)
{
    int32_t cp = Math_Cos(pitch);
    int32_t sp = Math_Sin(pitch);
    int32_t cy = Math_Cos(yaw);
    int32_t sy = Math_Sin(yaw);
    int32_t ls_x = TRIGMULT2(cp, sy);
    int32_t ls_y = -sp;
    int32_t ls_z = TRIGMULT2(cp, cy);
    g_LsVectorView.x = (g_W2VMatrix._00 * ls_x + g_W2VMatrix._01 * ls_y
                        + g_W2VMatrix._02 * ls_z)
        >> W2V_SHIFT;
    g_LsVectorView.y = (g_W2VMatrix._10 * ls_x + g_W2VMatrix._11 * ls_y
                        + g_W2VMatrix._12 * ls_z)
        >> W2V_SHIFT;
    g_LsVectorView.z = (g_W2VMatrix._20 * ls_x + g_W2VMatrix._21 * ls_y
                        + g_W2VMatrix._22 * ls_z)
        >> W2V_SHIFT;
}

void phd_AlterFOV(PHD_ANGLE fov)
{
    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (g_Config.fov_vertical) {
        double aspect_ratio =
            Screen_GetResWidth() / (double)Screen_GetResHeight();
        double fov_rad_h = fov * M_PI / 32760;
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * 32760);
    }

    int16_t c = Math_Cos(fov / 2);
    int16_t s = Math_Sin(fov / 2);
    g_PhdPersp = ((Screen_GetResWidth() / 2) * c) / s;
}
