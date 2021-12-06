#include "ati3dcif/Renderer.hpp"

#include "gfx/context.h"
#include "gfx/gl/utils.h"

#include <cassert>

namespace glrage {
namespace cif {

Renderer::Renderer()
{
    // The vertex stream passes primitives to the delayer to test if they
    // have translucency and in that case delay them. When the delayer needs
    // to display the delayed primitives it calls back to the vertex stream
    // to do so.
    m_vertexStream.setDelayer(
        [this](C3D_VTCF *verts) { return m_transDelay.delayTriangle(verts); });

    GFX_GL_Sampler_Init(&m_sampler);
    GFX_GL_Sampler_Bind(&m_sampler, 0);
    GFX_GL_Sampler_Parameterf(&m_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

    GFX_GL_Program_Init(&m_program);
    GFX_GL_Program_AttachShader(
        &m_program, GL_VERTEX_SHADER, "shaders\\ati3dcif.vsh");
    GFX_GL_Program_AttachShader(
        &m_program, GL_FRAGMENT_SHADER, "shaders\\ati3dcif.fsh");
    GFX_GL_Program_Link(&m_program);

    m_loc_matProjection =
        GFX_GL_Program_UniformLocation(&m_program, "matProjection");
    m_loc_matModelView =
        GFX_GL_Program_UniformLocation(&m_program, "matModelView");
    m_loc_solidColor = GFX_GL_Program_UniformLocation(&m_program, "solidColor");
    m_loc_shadeMode = GFX_GL_Program_UniformLocation(&m_program, "shadeMode");
    m_loc_tmapEn = GFX_GL_Program_UniformLocation(&m_program, "tmapEn");
    m_loc_texOp = GFX_GL_Program_UniformLocation(&m_program, "texOp");
    m_loc_tmapLight = GFX_GL_Program_UniformLocation(&m_program, "tmapLight");
    m_loc_chromaKey = GFX_GL_Program_UniformLocation(&m_program, "chromaKey");
    m_loc_keyOnAlpha = GFX_GL_Program_UniformLocation(&m_program, "keyOnAlpha");

    GFX_GL_Program_FragmentData(&m_program, "fragColor");
    GFX_GL_Program_Bind(&m_program);

    // negate Z axis so the model is rendered behind the viewport, which is
    // better than having a negative z_near in the ortho matrix, which seems
    // to mess up depth testing
    GLfloat model_view[4][4] = {
        { +1.0f, +0.0f, +0.0, +0.0f },
        { +0.0f, +1.0f, +0.0, +0.0f },
        { +0.0f, +0.0f, -1.0, -0.0f },
        { +0.0f, +0.0f, +0.0, +1.0f },
    };
    GFX_GL_Program_UniformMatrix4fv(
        &m_program, m_loc_matModelView, 1, GL_FALSE, &model_view[0][0]);

    // TODO: make me configurable
    m_wireframe = false;

    GFX_GL_CheckError();
}

Renderer::~Renderer()
{
    GFX_GL_Program_Close(&m_program);
    GFX_GL_Sampler_Close(&m_sampler);
}

void Renderer::renderBegin()
{
    glEnable(GL_BLEND);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    GFX_GL_Program_Bind(&m_program);
    m_vertexStream.bind();
    GFX_GL_Sampler_Bind(&m_sampler, 0);

    // restore texture binding
    tmapRestore();

    // CIF always uses an orthographic view, the application deals with the
    // perspective when required
    const auto left = 0.0f;
    const auto top = 0.0f;
    const auto right = static_cast<float>(GFX_Context_GetDisplayWidth());
    const auto bottom = static_cast<float>(GFX_Context_GetDisplayHeight());
    const auto z_near = -1e6;
    const auto z_far = 1e6;
    GLfloat projection[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -2.0f / (z_far - z_near), 0.0f },
        { -(right + left) / (right - left), -(top + bottom) / (top - bottom),
          -(z_far + z_near) / (z_far - z_near), 1.0f }
    };

    GFX_GL_Program_UniformMatrix4fv(
        &m_program, m_loc_matProjection, 1, GL_FALSE, &projection[0][0]);

    GFX_GL_CheckError();
}

void Renderer::renderEnd()
{
    // make sure everything has been rendered
    m_vertexStream.renderPending();
    // including the delayed translucent primitives
    GFX_GL_Program_Uniform1i(&m_program, m_loc_keyOnAlpha, true);
    m_transDelay.render([this](std::vector<C3D_VTCF> verts) {
        m_vertexStream.renderPrims(verts);
    });

    // restore polygon mode
    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    GFX_GL_CheckError();
}

bool Renderer::textureReg(C3D_PTMAP ptmapToReg, C3D_PHTX phtmap)
{
    auto texture = std::make_shared<Texture>();
    texture->bind();
    if (!texture->load(ptmapToReg, m_palettes[ptmapToReg->htxpalTexPalette])) {
        return false;
    }

    // use id as texture handle
    *phtmap = reinterpret_cast<C3D_HTX>(texture->id());

    // store in texture map
    m_textures[*phtmap] = texture;

    // restore previously bound texture
    tmapRestore();

    GFX_GL_CheckError();
    return true;
}

bool Renderer::textureUnreg(C3D_HTX htxToUnreg)
{
    auto it = m_textures.find(htxToUnreg);
    if (it == m_textures.end()) {
        LOG_ERROR("Invalid texture handle");
        return false;
    }

    // unbind texture if currently bound
    if (htxToUnreg == m_tmapSelect) {
        tmapSelect(0);
    }

    std::shared_ptr<Texture> texture = it->second;
    m_textures.erase(htxToUnreg);
    return true;
}

bool Renderer::texturePaletteCreate(
    C3D_ECI_TMAP_TYPE epalette, void *pPalette, C3D_PHTXPAL phtpalCreated)
{
    if (epalette != C3D_ECI_TMAP_8BIT) {
        LOG_ERROR("Unsupported palette type: %d", epalette);
        return false;
    }

    // copy palette entries to vector
    auto palettePtr = static_cast<C3D_PPALETTENTRY>(pPalette);
    std::vector<C3D_PALETTENTRY> palette(palettePtr, palettePtr + 256);

    // create new palette handle
    auto handle = reinterpret_cast<C3D_HTXPAL>(m_paletteID++);

    // store palette
    m_palettes[handle] = palette;

    *phtpalCreated = handle;
    return true;
}

void Renderer::texturePaletteDestroy(C3D_HTXPAL htxpalToDestroy)
{
    m_palettes.erase(htxpalToDestroy);
}

void Renderer::renderPrimStrip(C3D_VSTRIP vStrip, C3D_UINT32 u32NumVert)
{
    GFX_Context_SetRendered();
    m_vertexStream.addPrimStrip(vStrip, u32NumVert);
}

void Renderer::renderPrimList(C3D_VLIST vList, C3D_UINT32 u32NumVert)
{
    GFX_Context_SetRendered();
    m_vertexStream.addPrimList(vList, u32NumVert);
}

bool Renderer::setState(C3D_ERSID eRStateID, C3D_PRSDATA pRStateData)
{
    switch (eRStateID) {
    case C3D_ERS_VERTEX_TYPE:
        vertexType(*reinterpret_cast<C3D_EVERTEX *>(pRStateData));
        break;
    case C3D_ERS_PRIM_TYPE:
        primType(*reinterpret_cast<C3D_EPRIM *>(pRStateData));
        break;
    case C3D_ERS_SOLID_CLR:
        solidColor(*reinterpret_cast<C3D_COLOR *>(pRStateData));
        break;
    case C3D_ERS_SHADE_MODE:
        shadeMode(*reinterpret_cast<C3D_ESHADE *>(pRStateData));
        break;
    case C3D_ERS_TMAP_EN:
        tmapEnable(*reinterpret_cast<C3D_BOOL *>(pRStateData));
        break;
    case C3D_ERS_TMAP_SELECT:
        tmapSelect(*reinterpret_cast<C3D_HTX *>(pRStateData));
        break;
    case C3D_ERS_TMAP_LIGHT:
        tmapLight(*reinterpret_cast<C3D_ETLIGHT *>(pRStateData));
        break;
    case C3D_ERS_TMAP_FILTER:
        tmapFilter(*reinterpret_cast<C3D_ETEXFILTER *>(pRStateData));
        break;
    case C3D_ERS_TMAP_TEXOP:
        tmapTexOp(*reinterpret_cast<C3D_ETEXOP *>(pRStateData));
        break;
    case C3D_ERS_ALPHA_SRC:
        alphaSrc(*reinterpret_cast<C3D_EASRC *>(pRStateData));
        break;
    case C3D_ERS_ALPHA_DST:
        alphaDst(*reinterpret_cast<C3D_EADST *>(pRStateData));
        break;
    case C3D_ERS_Z_CMP_FNC:
        zCmpFunc(*reinterpret_cast<C3D_EZCMP *>(pRStateData));
        break;
    case C3D_ERS_Z_MODE:
        zMode(*reinterpret_cast<C3D_EZMODE *>(pRStateData));
        break;
    default:
        LOG_ERROR("Unsupported state: %d", eRStateID);
        return false;
    }
    return true;
}

void Renderer::vertexType(C3D_EVERTEX value)
{
    m_vertexStream.renderPending();
    m_vertexStream.vertexType(value);
}

void Renderer::primType(C3D_EPRIM value)
{
    m_vertexStream.renderPending();
    m_vertexStream.primType(value);
}

void Renderer::solidColor(C3D_COLOR value)
{
    m_vertexStream.renderPending();
    C3D_COLOR color = value;
    GFX_GL_Program_Uniform4f(
        &m_program, m_loc_solidColor, color.r / 255.0f, color.g / 255.0f,
        color.b / 255.0f, color.a / 255.0f);
}

void Renderer::shadeMode(C3D_ESHADE value)
{
    m_vertexStream.renderPending();
    GFX_GL_Program_Uniform1i(&m_program, m_loc_shadeMode, value);
}

void Renderer::tmapEnable(C3D_BOOL value)
{
    m_vertexStream.renderPending();
    C3D_BOOL enable = value;
    GFX_GL_Program_Uniform1i(&m_program, m_loc_tmapEn, enable);
    m_transDelay.setTexturingEnabled(enable != C3D_FALSE);
    if (enable) {
        glEnable(GL_TEXTURE_2D);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
}

void Renderer::tmapSelect(C3D_HTX value)
{
    m_vertexStream.renderPending();
    m_tmapSelect = value;
    tmapSelectImpl(value);
}

void Renderer::tmapSelectImpl(C3D_HTX handle)
{
    // unselect texture if handle is zero
    if (handle == 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    // check if handle is correct
    auto it = m_textures.find(handle);
    assert(it != m_textures.end());

    // get texture object and bind it
    auto texture = it->second;
    texture->bind();
    // Tell the transparent primitive delayer what texture is currently in
    // use
    m_transDelay.setTexture(texture);

    // send chroma key color to shader
    auto ck = texture->chromaKey();
    GFX_GL_Program_Uniform3f(
        &m_program, m_loc_chromaKey, ck.r / 255.0f, ck.g / 255.0f,
        ck.b / 255.0f);
    GFX_GL_Program_Uniform1i(
        &m_program, m_loc_keyOnAlpha, texture->keyOnAlpha());
}

void Renderer::tmapRestore()
{
    tmapSelectImpl(m_tmapSelect);
}

void Renderer::tmapLight(C3D_ETLIGHT value)
{
    m_vertexStream.renderPending();
    GFX_GL_Program_Uniform1i(&m_program, m_loc_tmapLight, value);
}

void Renderer::tmapFilter(C3D_ETEXFILTER value)
{
    m_vertexStream.renderPending();
    auto filter = value;
    GFX_GL_Sampler_Parameteri(
        &m_sampler, GL_TEXTURE_MAG_FILTER, GLCIF_TEXTURE_MAG_FILTER[filter]);
    GFX_GL_Sampler_Parameteri(
        &m_sampler, GL_TEXTURE_MIN_FILTER, GLCIF_TEXTURE_MIN_FILTER[filter]);
}

void Renderer::tmapTexOp(C3D_ETEXOP value)
{
    m_vertexStream.renderPending();
    GFX_GL_Program_Uniform1i(&m_program, m_loc_texOp, value);
}

void Renderer::alphaSrc(C3D_EASRC value)
{
    m_vertexStream.renderPending();
    m_easrc = value;
    C3D_EASRC alphaSrc = value;
    C3D_EADST alphaDst = m_eadst;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}

void Renderer::alphaDst(C3D_EADST value)
{
    m_vertexStream.renderPending();
    m_eadst = value;
    C3D_EASRC alphaSrc = m_easrc;
    C3D_EADST alphaDst = value;
    glBlendFunc(GLCIF_BLEND_FUNC[alphaSrc], GLCIF_BLEND_FUNC[alphaDst]);
}

void Renderer::zCmpFunc(C3D_EZCMP value)
{
    m_vertexStream.renderPending();
    C3D_EZCMP func = value;
    if (func < C3D_EZCMP_MAX) {
        glDepthFunc(GLCIF_DEPTH_FUNC[func]);
    }
}

void Renderer::zMode(C3D_EZMODE value)
{
    m_vertexStream.renderPending();
    auto mode = value;
    glDepthMask(GLCIF_DEPTH_MASK[mode]);

    if (mode > C3D_EZMODE_TESTON) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

}
}
