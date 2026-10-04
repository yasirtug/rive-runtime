/*
 * Copyright 2022 Rive
 */

#pragma once

#include "rive/math/raw_path.hpp"
#include "rive/renderer.hpp"

#include <memory>
#include <vector>

namespace rive
{
// RenderPath implementation for Rive's pixel local storage renderer.
class RiveRenderPath : public LITE_RTTI_OVERRIDE(RenderPath, RiveRenderPath)
{
public:
    RiveRenderPath() = default;
    RiveRenderPath(FillRule fillRule, RawPath& rawPath);

    void rewind() override;
    void fillRule(FillRule rule) override { m_fillRule = rule; }

    void moveTo(float x, float y) override;
    void lineTo(float x, float y) override;
    void cubicTo(float ox, float oy, float ix, float iy, float x, float y)
        override;
    void close() override;

    void addPath(CommandPath* p, const Mat2D& m) override
    {
        addRenderPath(p->renderPath(), m);
    }
    void addRenderPath(const RenderPath* path, const Mat2D& matrix) override;
    void addRenderPathBackwards(const RenderPath* path,
                                const Mat2D& transform) override;
    void addRawPath(const RawPath& path) override;
    const RawPath& getRawPath() const { return m_rawPath; }
    FillRule getFillRule() const { return m_fillRule; }

    const AABB& getBounds() const;
    // Approximates the area of the path by linearizing it with a coarse
    // tolerance of 8px in artboard space.
    constexpr static float kCoarseAreaTolerance = 8;
    float getCoarseArea() const;
    // Determine if the path's signed, post-transform area is positive.
    bool isClockwiseDominant(const Mat2D& viewMatrix) const;
    // True for a single contour whose control polygon turns one way through
    // exactly one revolution. Such a curve is convex, and convexity survives
    // any affine transform, so a midpoint fan of it emitted in its dominant
    // direction has no backward triangles.
    bool isConvex() const;
    uint64_t getRawPathMutationID() const;

    // Analysis a renderer derives from the raw path alone, kept while the path
    // is unchanged so later frames need not repeat it. `key` names what was
    // derived; the data is dropped as soon as the path changes.
    const void* findPreparation(uint32_t key) const;
    // Whether analysis just made is worth keeping: only once this unchanged
    // path has been analysed before, so a path rebuilt every frame (e.g. an
    // animation) never pays for copies it would use once.
    bool shouldKeepPreparation() const;
    void keepPreparation(uint32_t key, std::shared_ptr<const void> data) const;

    // 1-dimensional feathering along the normal vector quits looking like a
    // blur when there is strong curvature. This method returns a copy of the
    // path with shorter, flatter curves that will more accurately depict a
    // gaussian blur when drawn with the given feather.
    //
    // TODO: Move this work to the GPU.
    rcp<RiveRenderPath> makeSoftenedCopyForFeathering(float feather,
                                                      float matrixMaxScale);

#ifdef DEBUG
    // Allows ref holders to guarantee the rawPath doesn't mutate during a
    // specific time.
    void lockRawPathMutations() const { ++m_rawPathMutationLockCount; }
    void unlockRawPathMutations() const
    {
        assert(m_rawPathMutationLockCount > 0);
        --m_rawPathMutationLockCount;
    }
#endif

private:
    FillRule m_fillRule = FillRule::nonZero;
    RawPath m_rawPath;
    mutable AABB m_bounds;
    mutable float m_coarseArea;
    mutable uint64_t m_rawPathMutationID;
    mutable bool m_isConvex;

    struct Preparation
    {
        uint32_t key;
        std::shared_ptr<const void> data;
    };
    // Drops kept analysis if the path changed since it was made.
    void syncPreparations() const;
    mutable std::vector<Preparation> m_preparations;
    mutable uint64_t m_preparationsMutationID = 0;
    mutable uint32_t m_preparationRequests = 0;

    enum Dirt
    {
        kPathBoundsDirt = 1 << 0,
        kRawPathMutationIDDirt = 1 << 1,
        kPathCoarseAreaDirt = 1 << 2,
        kPathConvexityDirt = 1 << 3,
        kAllDirt = ~0,
    };
    mutable uint32_t m_dirt = kAllDirt;
    RIVE_DEBUG_CODE(mutable int m_rawPathMutationLockCount = 0;)
};
} // namespace rive
