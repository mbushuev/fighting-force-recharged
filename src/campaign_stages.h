#pragma once

namespace ff_campaign {
// Original English level-select labels, in the native campaign-index order.
// Drop only the repeated " LEVEL" suffix. Save records keep their numeric IDs.
inline const char* StageName(int stage) {
    static const char* const names[]={
        "CAR PARK", "RECEPTION", "LIFT", "CORRIDOR", "ZENGS OFFICE",
        "HIGH STREET", "HIGH STREET BOSS", "MALL", "PARK 1", "PARK 2",
        "BRIDGE", "BRONX", "SUBWAY", "SUBWAY BOSS", "TRAIN", "AIR BASE",
        "NAVAL BASE 2", "FLYING WING", "HOVERCRAFT", "NAVAL BASE 1",
        "AIR BASE BOSS", "NAVAL BASE BOSS", "ISLAND LAB", "ISLAND LIFT", "FINAL"
    };
    return stage>=0 && stage<25?names[stage]:"UNKNOWN LEVEL";
}
}
