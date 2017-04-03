#include "squarepartitioner.h"
#include <exception>

using namespace std;

MapTile* SquarePartitioner::getMazeSection(unsigned int& width, unsigned int& height,
                            const unsigned int& player, point& relative_loc,
                            const maze& m, MapTile* reuse)
{
    point target_loc = m.players[player];
    if(reuse == nullptr)
    {
        width = height = 11;
        reuse = new MapTile[width*height];
    }
    MapTile* outiter = reuse;
    auto initer = m.begin();
    unsigned int mwidth = m.width();
    unsigned int mheight = m.height();
    unsigned int w2 = width/2;
    unsigned int h2 = height/2;
    unsigned int y_start = target_loc.y-h2;
    unsigned int x_start = target_loc.x-w2;

    for(int i=0, i_ = y_start; i<height; i++, i_++)
    {
        int offset = i_*mwidth + (target_loc.x < w2 ? 0 : x_start);
        initer = m.begin() + std::max(offset, 0);

        for(int j=0, j_ = x_start; j<width; j++, j_++)
        {
            if(i_ >= 0 && i_ < mheight && j_ >= 0 && j_ < mwidth)
            {
                *outiter = *initer;
                ++initer;
            }
            else
                *outiter = MapTile{0};

            ++outiter;
        }
    }

    relative_loc = point{width/2, height/2};

    return reuse;
}
