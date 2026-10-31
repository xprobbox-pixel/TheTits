#include "../PluginProcessor.h"
int TheTitsProcessor::factoryCount() const {
    return factoryBank.isArray() ? factoryBank.size() : 0;
}
juce::var TheTitsProcessor::factory(int i) const {
    return i >= 0 && i < factoryCount() ? factoryBank[i] : juce::var();
}
void TheTitsProcessor::loadFactory(int i, bool performance) {
    auto f = factory(i);
    if (f.isVoid())
        return;
    juce::String error;
    if (importJSON(juce::JSON::toString(f), error)) {
        presetIndex = i;
        if (performance) {
            set("build", 0);
            set("output", float(f.getProperty("output", 0)));
        }
        dirty = false;
    }
}
juce::String TheTitsProcessor::exportJSON() {
    auto *o = new juce::DynamicObject;
    o->setProperty("version", 1);
    o->setProperty("name", presetName);
    o->setProperty("category", category);
    o->setProperty("description", "User configuration");
    o->setProperty("mix", value("mix"));
    o->setProperty("output", value("output"));
    auto *performance = new juce::DynamicObject;
    for (auto name : {"build", "mode", "bars", "quantise", "endMode", "wave", "depth", "minimum",
                      "sync", "freeHz", "tempo", "tails", "protection"})
        performance->setProperty(name, value(name));
    o->setProperty("performance", juce::var(performance));
    juce::Array<juce::var> chain;
    auto c = config();
    for (const auto &s : c.slot) {
        auto *v = new juce::DynamicObject;
        v->setProperty("type", s.type);
        v->setProperty("enabled", s.enabled);
        juce::Array<juce::var> maps;
        for (auto &m : s.map) {
            juce::Array<juce::var> a{m.from, m.to, m.begin, m.end, m.curve, m.domain};
            maps.add(a);
        }
        v->setProperty("mappings", maps);
        chain.add(v);
    }
    o->setProperty("slots", chain);
    return juce::JSON::toString(juce::var(o), true);
}
bool TheTitsProcessor::importJSON(const juce::String &json, juce::String &error) {
    auto fail = [&](const char *s) {
        error = s;
        return false;
    };
    if (json.length() > 200000)
        return fail("Preset is too large");
    auto f = juce::JSON::parse(json);
    if (!f.isObject() || int(f.getProperty("version", 0)) != 1 || !f["name"].isString() ||
        f["name"].toString().length() > 100)
        return fail("Invalid version or name");
    auto chain = f["slots"];
    if (!chain.isArray() || chain.size() != tits::slots)
        return fail("Exactly six slots are required");
    tits::Config c;
    for (int s = 0; s < tits::slots; ++s) {
        auto v = chain[s];
        if (!v.isObject() || !v["type"].isInt() || int(v["type"]) < 0 ||
            int(v["type"]) >= tits::ModuleCount)
            return fail("Unknown module");
        c.slot[s].type = int(v["type"]);
        c.slot[s].enabled = bool(v.getProperty("enabled", true));
        auto maps = v["mappings"];
        if (!maps.isArray() || maps.size() != tits::controls)
            return fail("Eight mappings required per slot");
        for (int p = 0; p < tits::controls; ++p) {
            auto a = maps[p];
            if (!a.isArray() || a.size() != 6)
                return fail("Invalid mapping");
            float n[6];
            for (int j = 0; j < 6; ++j) {
                if (!(a[j].isDouble() || a[j].isInt() || a[j].isInt64()))
                    return fail("Mapping must contain numbers");
                n[j] = float(a[j]);
                if (!std::isfinite(n[j]))
                    return fail("Non-finite mapping");
            }
            if (std::abs(n[0]) > 192000 || std::abs(n[1]) > 192000 || n[2] < 0 || n[3] > 1 ||
                n[2] >= n[3] || n[4] < .15f || n[4] > 6 || n[5] < 0 || n[5] > 1 ||
                std::floor(n[5]) != n[5] || (n[5] == 1 && (n[0] <= 0 || n[1] <= 0)))
                return fail("Mapping outside safe range");
            c.slot[s].map[p] = {n[0], n[1], n[2], n[3], n[4], int(n[5])};
        }
    }
    float mix = float(f.getProperty("mix", 1));
    if (!std::isfinite(mix) || mix < 0 || mix > 1)
        return fail("Invalid mix");
    auto performance = f["performance"];
    if (!performance.isVoid()) {
        if (!performance.isObject())
            return fail("Invalid performance settings");
        juce::StringArray allowed{"build", "mode",  "bars",      "quantise", "endMode",
                                  "wave",  "depth", "minimum",   "sync",     "freeHz",
                                  "tempo", "tails", "protection"};
        for (auto &prop : performance.getDynamicObject()->getProperties()) {
            if (!allowed.contains(prop.name.toString()))
                return fail("Unknown performance setting");
            auto *param = state.getParameter(prop.name.toString());
            if (!param || !(prop.value.isInt() || prop.value.isDouble()) ||
                !std::isfinite(float(prop.value)) ||
                float(prop.value) < param->getNormalisableRange().start ||
                float(prop.value) > param->getNormalisableRange().end)
                return fail("Invalid performance parameter");
        }
    }
    loading = true;
    configGeneration.fetch_add(1);
    for (int s = 0; s < tits::slots; ++s) {
        auto prefix = "s" + juce::String(s);
        set(prefix + "on", c.slot[s].enabled ? 1 : 0);
        for (int p = 0; p < tits::controls; ++p) {
            auto id = prefix + "p" + juce::String(p);
            auto &m = c.slot[s].map[p];
            set(id + "from", m.from);
            set(id + "to", m.to);
            set(id + "begin", m.begin);
            set(id + "end", m.end);
            set(id + "curve", m.curve);
            set(id + "domain", float(m.domain));
        }
        set(prefix + "type", float(c.slot[s].type));
    }
    if (performance.isObject())
        for (auto &prop : performance.getDynamicObject()->getProperties())
            if (prop.name.toString() != "build" && prop.name.toString() != "output")
                set(prop.name.toString(), float(prop.value));
    set("mix", mix);
    presetName = f["name"].toString();
    category = f.getProperty("category", "User").toString();
    dirty = false;
    configGeneration.fetch_add(1);
    loading = false;
    return true;
}
