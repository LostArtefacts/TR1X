#pragma once

#include "ati3dcif/ATI3DCIF.h"
#include "ati3dcif/TransDelay.hpp"
#include "gfx/gl/buffer.h"
#include "gfx/gl/vertex_array.h"

#include <vector>

namespace glrage {
namespace cif {

static const GLenum GLCIF_PRIM_MODES[] = {
    GL_LINES, // C3D_EPRIM_LINE
    GL_TRIANGLES, // C3D_EPRIM_TRI
    GL_TRIANGLES, // C3D_EPRIM_QUAD
    GL_TRIANGLES, // C3D_EPRIM_RECT
    GL_POINTS // C3D_EPRIM_POINT
};

class VertexStream {
public:
    VertexStream();
    ~VertexStream();
    bool addPrimStrip(C3D_VSTRIP vertStrip, C3D_UINT32 numVert);
    bool addPrimList(C3D_VLIST vertList, C3D_UINT32 numVert);
    void renderPending();
    void vertexType(C3D_EVERTEX vertexType);
    void primType(C3D_EPRIM primType);
    void bind();
    void renderPrims(std::vector<C3D_VTCF> prims);
    void setDelayer(std::function<BOOL(C3D_VTCF *)> delayer);

private:
    C3D_EVERTEX m_vertexType = C3D_EV_VTCF;
    C3D_EPRIM m_primType = C3D_EPRIM_TRI;
    size_t m_vertexBufferSize = 0;
    GFX_GL_Buffer m_vertexBuffer;
    GFX_GL_VertexArray m_vtcFormat;
    std::vector<C3D_VTCF> m_vtcBuffer;
    std::function<BOOL(C3D_VTCF *)> m_delayer;
};

}
}
