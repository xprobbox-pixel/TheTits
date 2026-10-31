#include "ArtworkRenderer.h"
#include <BinaryData.h>
namespace tits {
ArtworkRenderer::ArtworkRenderer() {
    reference = juce::ImageCache::getFromMemory(BinaryData::poster_png, BinaryData::poster_pngSize);
    contour.startNewSubPath(151, 646);
    contour.cubicTo(146, 541, 236, 426, 337, 358);
    contour.cubicTo(369, 339, 399, 339, 421, 310);
    contour.cubicTo(459, 252, 515, 247, 574, 267);
    contour.cubicTo(647, 289, 708, 325, 769, 320);
    contour.cubicTo(855, 314, 932, 277, 1016, 289);
    contour.cubicTo(1070, 285, 1101, 302, 1126, 338);
    contour.cubicTo(1246, 397, 1347, 493, 1380, 625);
    contour.cubicTo(1415, 741, 1351, 870, 1248, 918);
    contour.cubicTo(1138, 979, 1019, 952, 915, 928);
    contour.cubicTo(825, 903, 778, 887, 704, 905);
    contour.cubicTo(571, 947, 429, 949, 315, 913);
    contour.cubicTo(201, 878, 151, 772, 151, 646);
    contour.closeSubPath();
    // The dial and its collar stay anchored; the functional dial overlays the
    // reference at the same position without exposing a second moving rim.
}
void ArtworkRenderer::background(juce::Graphics &g, juce::Rectangle<float> b) {
    g.setColour(juce::Colour(0xfff6ce51));
    g.fillRect(b);
    auto tile = reference.getClippedImage({600, 0, 200, 200});
    for (int y = 0; y < 1024; y += 200)
        for (int x = 0; x < 1536; x += 200) {
            bool flipX = (x / 200) % 2, flipY = (y / 200) % 2;
            auto transform =
                juce::AffineTransform::scale(flipX ? -1.f : 1.f, flipY ? -1.f : 1.f)
                    .translated(float(x + (flipX ? 200 : 0)), float(y + (flipY ? 200 : 0)));
            g.drawImageTransformed(tile, transform);
        }
    juce::Path title;
    title.startNewSubPath(865, 0);
    title.lineTo(1536, 0);
    title.lineTo(1536, 350);
    title.lineTo(1140, 350);
    title.lineTo(1097, 299);
    title.lineTo(1050, 285);
    title.lineTo(1000, 281);
    title.lineTo(945, 296);
    title.lineTo(865, 260);
    title.closeSubPath();
    g.saveState();
    g.reduceClipRegion(title);
    g.drawImageAt(reference, 0, 0);
    g.restoreState();
}
juce::Point<float> ArtworkRenderer::deform(float x, float y, const JiggleAnimator &a) const {
    int side = x < 768 ? 0 : 1;
    const auto &s = a.springs[side];
    double central = std::clamp(std::abs(x - 768) / 265., 0., 1.);
    double anchor = std::clamp((y - 280) / 300., 0., 1.);
    double distance = std::hypot(x - 768., y - 584.);
    double collar = std::clamp((distance - 258) / 110., 0., 1.);
    collar = collar * collar * (3 - 2 * collar);
    double weight = central * anchor * collar;
    double dy = s.x * 680 * weight;
    double dx = (x - (side == 0 ? 450 : 1095)) * s.x * .6 * weight;
    double rotation = s.secondary * .55 * weight;
    dx -= (y - 540) * rotation;
    dy += (x - (side == 0 ? 450 : 1095)) * rotation;
    return {float(x + dx), float(y + dy)};
}
void ArtworkRenderer::draw(juce::Graphics &g, juce::Rectangle<float> b, const JiggleAnimator &a) {
    auto base = juce::AffineTransform::scale(b.getWidth() / 1536.f, b.getHeight() / 1024.f)
                    .translated(b.getX(), b.getY());
    constexpr int nx = 32, ny = 22;
    for (int j = 0; j < ny; ++j)
        for (int i = 0; i < nx; ++i) {
            float x = 125 + i * 1300.f / nx, y = 240 + j * 745.f / ny, dx = 1300.f / nx,
                  dy = 745.f / ny;
            juce::Point<float> points[4]{{x, y}, {x + dx, y}, {x, y + dy}, {x + dx, y + dy}};
            for (int tri = 0; tri < 2; ++tri) {
                auto p0 = points[tri == 0 ? 0 : 3], p1 = points[1], p2 = points[2];
                auto q0 = deform(p0.x, p0.y, a), q1 = deform(p1.x, p1.y, a),
                     q2 = deform(p2.x, p2.y, a);
                auto warp =
                    juce::AffineTransform::fromTargetPoints(p0.x, p0.y, q0.x, q0.y, p1.x, p1.y,
                                                            q1.x, q1.y, p2.x, p2.y, q2.x, q2.y)
                        .followedBy(base);
                juce::Path triangle;
                triangle.addTriangle(p0, p1, p2);
                auto centre = (p0 + p1 + p2) / 3.f;
                triangle.applyTransform(
                    juce::AffineTransform::scale(1.055f, 1.055f, centre.x, centre.y));
                g.saveState();
                g.reduceClipRegion(triangle, warp);
                g.reduceClipRegion(contour, warp);
                g.drawImageTransformed(reference, warp);
                g.restoreState();
            }
        }
}
} // namespace tits
