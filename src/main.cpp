#include <Geode/Geode.hpp>
#include "Session.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("[BuilderSync] Loaded. Open the editor and press the BuilderSync button to start.");
}