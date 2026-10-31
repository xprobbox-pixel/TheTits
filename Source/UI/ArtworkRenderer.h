#pragma once
#include "JiggleAnimator.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace tits {
class ArtworkRenderer {
  public:
    ArtworkRenderer();
    void background(juce::Graphics &, juce::Rectangle<float>);
    void draw(juce::Graphics &, juce::Rectangle<float>, const JiggleAnimator &);

  private:
    juce::Image reference;
    juce::Path contour;
    juce::Point<float> deform(float, float, const JiggleAnimator &) const;
};
} // namespace tits
