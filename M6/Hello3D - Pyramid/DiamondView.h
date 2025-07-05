#ifndef DIAMONDVIEW_H
#define DIAMONDVIEW_H

#include "TilemapView.h"

class DiamondView : public TilemapView {
public:
    void computeDrawPosition(const int col, const int row, const float tw, const float th, float& targetx, float& targety) const override {
        targetx = (col * tw / 2.0f) + (row * tw / 2.0f) ;
        targety = (col * th / 2.0f) - (row * th / 2.0f) ;
    }

    void computeMouseMap(int& col, int& row, const float tw, const float th, const float mx, const float my) const override {
        
        float half_tw = tw / 2.0f;
        float half_th = th / 2.0f;

        col = ((mx / half_tw)/2.0f) + ((my / half_th) / 2.0f);
        row = ((mx / half_tw)/2.0f) - ((my / half_th) / 2.0f);
    }

    void computeTileWalking(int& col, int& row, const int direction) const override {
        switch (direction) {
        case DIRECTION_NORTH:
            col++;
            row--;
            break;
        case DIRECTION_SOUTH:
            col--;
            row++;
            break;
        case DIRECTION_WEST:
            col--;
            row--;
            break;
        case DIRECTION_EAST:
            col++;
            row++;
            break;
        case DIRECTION_NORTHEAST:
            col++;
            break;
        case DIRECTION_SOUTHEAST:
            row++;
            break;
        case DIRECTION_SOUTHWEST:
            col--;
            break;
        case DIRECTION_NORTHWEST:
            row--;
            break;
        }
    }
};

#endif