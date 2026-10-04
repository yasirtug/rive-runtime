/*
 * Copyright 2023 Rive
 */

#pragma once

#include "rive/math/aabb.hpp"
#include "rive/renderer/gl/gles3.hpp"
#include "rive/refcnt.hpp"
#include "rive/renderer/gpu.hpp"

namespace rive::gpu
{
// Lightweight wrapper around common GL state.
class GLState : public RefCnt<GLState>
{
public:
    GLState(const GLCapabilities& capabilities) : m_capabilities(capabilities)
    {
        invalidate();

#ifndef RIVE_WEBGL
        if (m_capabilities.KHR_parallel_shader_compile)
        {
            // Configure the compiler once per GLState, not on every shared-context
            // invalidation. Repeating this call can stall the driver for ~200 ms.
            // Some drivers need an explicit setting to enable parallel compilation;
            // keep the existing two-thread limit rather than requesting all threads.
            glMaxShaderCompilerThreadsKHR(2);
        }
#endif
    }

    const GLCapabilities& capabilities() const { return m_capabilities; }

    void invalidate();

    // Set the scissor with a top-down oriented box. (GLState will Y-flip the
    // box before passing it to GL, which is why this function needs
    // renderTargetHeight.)
    void setScissor(IAABB, uint32_t renderTargetHeight);
    // Set the scissor with the raw values that will be passed to glScissor().
    void setScissorRaw(uint32_t left,
                       uint32_t top,
                       uint32_t width,
                       uint32_t height);
    void disableScissor();
    void setDepthStencilEnabled(bool depthEnabled, bool stencilEnabled);
    void setCullFace(GLenum);
    void setWriteMasks(bool colorWriteMask,
                       bool depthWriteMask,
                       uint8_t stencilWriteMask);
    void setBlendEquation(gpu::BlendEquation);
    void disableBlending() { setBlendEquation(gpu::BlendEquation::none); }
    void setPipelineState(const gpu::PipelineState&);

    void bindProgram(GLuint);
    void bindVAO(GLuint);
    void bindBuffer(GLenum target, GLuint);

    void deleteProgram(GLuint);
    void deleteVAO(GLuint);
    void deleteBuffer(GLuint);

private:
    // One face's stencil test and ops, exactly as passed to GL.
    struct StencilFaceGL
    {
        GLenum func;
        GLint reference;
        GLuint compareMask;
        GLenum failOp;
        GLenum depthFailOp;
        GLenum passOp;

        bool sameTest(const StencilFaceGL& other) const
        {
            return func == other.func && reference == other.reference &&
                   compareMask == other.compareMask;
        }
        bool sameOps(const StencilFaceGL& other) const
        {
            return failOp == other.failOp &&
                   depthFailOp == other.depthFailOp && passOp == other.passOp;
        }
    };
    // Issues only the stencil calls that change GL's state. Path draws switch
    // stencil settings on nearly every draw, and many consecutive draws repeat
    // them, so redundant calls are a large share of the WebGL command stream.
    void setStencilFaces(const StencilFaceGL& front, const StencilFaceGL& back);

    const GLCapabilities m_capabilities;
    std::array<uint32_t, 4> m_scissorBox;
    bool m_scissorEnabled;
    bool m_depthTestEnabled;
    bool m_stencilTestEnabled;
    bool m_colorWriteMask;
    bool m_depthWriteMask;
    gpu::BlendEquation m_blendEquation;
    GLuint m_stencilWriteMask;
    GLenum m_cullFace;
    GLuint m_boundProgramID;
    GLuint m_boundVAO;
    GLuint m_boundArrayBufferID;
    GLuint m_boundUniformBufferID;
    StencilFaceGL m_stencilFront;
    StencilFaceGL m_stencilBack;

    struct
    {
        bool scissorBox : 1;
        bool scissorEnabled : 1;
        bool depthStencilEnabled : 1;
        bool writeMasks : 1;
        bool blendEquation : 1;
        bool cullFace : 1;
        bool stencilFaces : 1;
        bool boundProgramID : 1;
        bool boundVAO : 1;
        bool boundArrayBufferID : 1;
        bool boundUniformBufferID : 1;
        bool boundPixelUnpackBufferID : 1;
    } m_validState;
};
} // namespace rive::gpu
