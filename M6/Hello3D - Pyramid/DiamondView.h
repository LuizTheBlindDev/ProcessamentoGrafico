#ifndef DIAMONDVIEW_H
#define DIAMONDVIEW_H

#include "TilemapView.h"

class DiamondView : public TilemapView {
public:
    void computeDrawPosition(const int col, const int row, const float tw, const float th, float& targetx, float& targety) const override {
        targetx = (col - row) * tw / 2.0f;
        targety = (col + row) * th / 2.0f;
    }

    void computeMouseMap(int& col, int& row, const float tw, const float th, const float mx, const float my) const override {
        float cx = mx / (tw / 2.0f);
        float cy = my / (th / 2.0f);
        col = static_cast<int>((cy + cx) / 2.0f);
        row = static_cast<int>((cy - cx) / 2.0f);
    }

    void computeTileWalking(int& col, int& row, const int direction) const override {
        switch (direction) {
        case 0: row--; break;           // NORTH
        case 1: col++; break;           // EAST
        case 2: row++; break;           // SOUTH
        case 3: col--; break;           // WEST
        }
    }
};

#endif