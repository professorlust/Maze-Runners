#include "spartacus.h"

#include <cmath>
#include <iostream>
#include <queue>
#include <algorithm>
#include <vector>

using namespace std;

MazePoint operator + (const MazePoint& l, const MazePoint& r)
{
    return MazePoint{l.x + r.x, l.y + r.y};
}

MazePoint operator - (const MazePoint& l, const MazePoint& r)
{
    return MazePoint{l.x - r.x, l.y - r.y};
}

MazePoint operator != (const MazePoint& l, const MazePoint& r)
{
    return l.x != r.x || l.y != r.y;
}

double sqmag(const MazePoint& p)
{
    return p.x*p.x + p.y*p.y;
}

vector<MazePoint> Bresenham(MazePoint start, MazePoint end)
{
    //cerr << "Bresenham " << start.x << ", " << start.y << " -> " << end.x << ", " << end.y << endl;
    int deltax = end.x - start.x;
    int deltay = end.y - start.y;
    //cerr << "Deltas " << deltax << ", " << deltay << endl;

    vector<MazePoint> line;

    //Handle trivial cases
    if(deltax == 0)
    {
        //cerr << "Vertical line" << endl;
        int ydir = deltay/abs(deltay);
        for(int i=start.y; i != end.y; i+=ydir)
            line.push_back(MazePoint(start.x, i));
        return line;
    }

    if(deltay == 0)
    {
        //cerr << "Horizontal line" << endl;
        int xdir = deltax/abs(deltax);
        for(int i=start.x; i != end.x; i+=xdir)
            line.push_back(MazePoint(i, start.y));
        return line;
    }

    int xdir = deltax/abs(deltax);
    int ydir = deltay/abs(deltay);
    //cerr << "Directions " << xdir << ", " << ydir << endl;

    double deltaerr = abs((double)deltay / deltax);
    double error = deltaerr - 0.5;
    MazePoint curr = start;
    MazePoint diffReg, diffAdj;
    if(abs(deltay) > abs(deltax))
    {
        diffReg = MazePoint(0, ydir);
        diffAdj = MazePoint(xdir, 0);
    }
    else
    {
        diffReg = MazePoint(xdir, 0);
        diffAdj = MazePoint(0, ydir);
    }

    for(int i=0; i<max(abs(deltay), abs(deltax))+1; i++)
    {
        //cerr << "Point " << curr.x << ", " << curr.y << endl;
        line.push_back(curr);
        error += deltaerr;
        if(error >= 0.5)
        {
            curr = curr + diffAdj;
            error -= 1;
        }
        curr = curr + diffReg;
        //cerr << deltaerr << endl;
    }

    //cerr << "Done" << endl;

    return line;
}

void Spartacus::getValidDirections(const MazePoint& loc, vector<MazePoint>& out)
{
    out.clear();
    if(world.count(loc.x) == 0 && world[loc.x].count(loc.y) == 0)
    {
        //cerr << "Unknown location " << loc.x << ", " << loc.y << endl;
        return;
    }

    AdvancedMapTile& t = world[loc.x][loc.y];
    //cerr << "Exits from here: " << (int)t.exits << endl;

    if(t.exits & (unsigned char)MapTile::Direction::NORTH )
        out.push_back(MazePoint{0, -1});
    if(t.exits & (unsigned char)MapTile::Direction::SOUTH )
        out.push_back(MazePoint{0, 1});
    if(t.exits & (unsigned char)MapTile::Direction::EAST )
        out.push_back(MazePoint{1, 0});
    if(t.exits & (unsigned char)MapTile::Direction::WEST )
        out.push_back(MazePoint{-1, 0});
}

void Spartacus::setMazeSettings(const MazeSettings& settings)
{
    //cerr << "Spartacus maze set up" << endl;
    MazePoint curr(0,0);
    MazePoint target(settings.exit_x, settings.exit_y);
    exitLoc = target;

    targetLine = Bresenham(curr, target);
    targetLine.erase(targetLine.begin());

    location = MazePoint(0,0);
    lastMove = AdvancedPlayerMove();
    firstTurn = true;
    world.clear();
    visited.clear();
    exitDists.clear();
    exitDists[target.x][target.y] = 1;
    wallBreaks = 3;
}

//Extends dead ends as far as possible until
//A exit path is hit
//An intersection with > 1 valid (non dead end) exits is hit
void Spartacus::bfsDeadEnds(const MazePoint& start)
{
    static vector<MazePoint> exits;
    MazePoint curr = start;
    MazePoint next;
    bool more = true;
    while(more)
    {
        more = false;
        //cerr << "Checking for dead end at " << curr.x << ", " << curr.y << endl;
        if(deadEnd[curr.x][curr.y]) 
        {
            //cerr << "Aready a dead end" << endl;
            continue;
        }
        if(exitDists[curr.x][curr.y] > 0)
        {
            //cerr << "Part of exit path!" << endl;
            continue;
        }
        if(visited[curr.x][curr.y]) continue;

        getValidDirections(curr, exits);
        int validExits = 0;
        //cerr << "Exits: ";
        for(auto& p : exits)
        {
            //cerr << "(" << p.x << ", " << p.y << ") ";
            next = curr + p;
            if(deadEnd[next.x][next.y]) continue;
            validExits++;
        }
        //cerr << endl;

        if(validExits == 1)
        {
            deadEnd[curr.x][curr.y] = true;
            more = true;
            //cerr << "New dead end: " << curr.x << ", " << curr.y << endl;
            curr = next;
        }
    }
}

void Spartacus::bfsExit(const MazePoint& start)
{
    static vector<MazePoint> exits;
    if(exitDists[start.x][start.y] == 0) return;
    queue<MazePoint> bfs;
    bfs.push(start);
    while(bfs.size())
    {
        MazePoint curr = bfs.front();
        bfs.pop();
        int currDist = exitDists[curr.x][curr.y];
        getValidDirections(curr, exits);
        for(auto& p : exits)
        {
            MazePoint next = curr + p;
            if(exitDists[next.x][next.y] == 0 || exitDists[next.x][next.y] > currDist + 1)
            {
                exitDists[next.x][next.y] = currDist+1;
                //cerr << "Exit dist " << next.x << ", " << next.y << ": " << currDist+1;
                bfs.push(next);
            }
        }
    }
}

std::vector<MazePoint> Spartacus::checkPathToTargetLine()
{
    return vector<MazePoint>();
}

AdvancedPlayerMove Spartacus::moveOntoExitPath()
{
    AdvancedPlayerMove out(AdvancedMapTile::Direction::NONE);
    AdvancedMapTile currTile = world[location.x][location.y];

    struct targetPoint
    {
        int distanceToExit;
        MazePoint move;
    };

    vector<targetPoint> moves;

    const static vector<MazePoint> dirs = {MazePoint{0, 1}, MazePoint{0, -1}, MazePoint{1, 0}, MazePoint{-1, 0}};
    for(const auto& p : dirs)
    {
        MazePoint next = location + p;
        if(exitDists[next.x][next.y] > 0)
        {
            moves.push_back(targetPoint{exitDists[next.x][next.y], p});
        }
    }

    sort(moves.begin(), moves.end(), [](const targetPoint& l, const targetPoint& r){return l.distanceToExit < r.distanceToExit;});
    for(auto& t : moves)
    {
        AdvancedMapTile::Direction thsDir;

        if(t.move.x == 1)
            thsDir = AdvancedMapTile::Direction::EAST;
        
        else if(t.move.x == -1)
            thsDir = AdvancedMapTile::Direction::WEST;
        
        else if(t.move.y == 1)
            thsDir = AdvancedMapTile::Direction::SOUTH;

        else thsDir = AdvancedMapTile::Direction::NORTH;

        if(currTile.exits & (int)thsDir)
        {
            out.attemptedMove = AdvancedPlayerMove::Move::MOVETO;
            out.destination = t.move;
            break;
        }
        else if(wallBreaks > 0)
        {
            wallBreaks--;
            out.attemptedMove = AdvancedPlayerMove::Move::WALLBREAK;
            out.dir = thsDir;
            break;
        }
    }

    return out;
}

AdvancedPlayerMove Spartacus::bookkeeping(const AdvancedMapTile* surroundings,                //Const pointer to local area
                            const uint& area_width, const uint& area_height,    //Size of local area
                            const uint& loc_x, const uint& loc_y)
{
    AdvancedPlayerMove out;
    AdvancedMapTile currentTile = surroundings[area_width*loc_y + loc_x];
    if(!firstTurn && lastTile.uid != currentTile.uid)
    {
        if(lastMove.attemptedMove == AdvancedPlayerMove::Move::MOVETO)
            location = location + lastMove.destination;
        else if(lastMove.attemptedMove == AdvancedPlayerMove::Move::WALLBREAK ||
                lastMove.attemptedMove == AdvancedPlayerMove::Move::WALLPHASE)
            switch(lastMove.dir)
            {
                case AdvancedMapTile::Direction::NORTH:
                    location.y -= 1; break;
                case AdvancedMapTile::Direction::SOUTH:
                    location.y += 1; break;
                case AdvancedMapTile::Direction::WEST:
                    location.x -= 1; break;
                case AdvancedMapTile::Direction::EAST:
                    location.x += 1; break;
            }
    }

    lastTile = currentTile;
    firstTurn = false;
    lastMove = AdvancedPlayerMove();

    visited[location.x][location.y] = true;

    auto iter = surroundings;
    for(int i=location.y-loc_y; i<location.y + area_height-loc_y; i++)
    {
        for(int j=location.x-loc_x; j<location.x + area_width-loc_x; j++)
        {
            if(iter->exits != 0)
            {
                //cerr << "Got data for " << j << ", " << i << endl;
                world[j][i] = *iter;
                bfsDeadEnds(MazePoint(j, i));
                bfsExit(MazePoint(j, i));
            }
            ++iter;
        }
    }

    //Check if we can move onto (or next to) the exit path
    out = moveOntoExitPath();
    if(out.attemptedMove != AdvancedPlayerMove::Move::NOOP) return out;

    //See if we know how to get to somewhere further on the target line
    vector<MazePoint> newPath = checkPathToTargetLine();
    if(newPath.size() > 0) plannedPath = newPath;

    //If we previously planned a path, follow along it
    if(plannedPath.size())
    {
        out.attemptedMove = AdvancedPlayerMove::Move::MOVETO;
        out.destination = location-plannedPath.front();
        plannedPath.erase(plannedPath.begin());
    }

    return out;
}

MazePoint Spartacus::closestToExit(const vector<MazePoint>& goodMoves)
{
    MazePoint choiceMove = goodMoves[0];
    double closest = sqmag(exitLoc - goodMoves[0]);
    for(auto& p : goodMoves)
    {
        double d = sqmag(exitLoc - p);
        if(d < closest)
        {
            closest = d;
            choiceMove = p;
        }
    }

    return choiceMove;
}

bool Spartacus::pointEnclosed(const MazePoint& start, const MazePoint& end)
{
    unordered_map<int, unordered_map<int, bool>> _visited;
    queue<MazePoint> q;
    q.push(start);
    _visited[start.x][start.y] = true;
    int total = 0;
    while(q.size())
    {
        total++;
        MazePoint curr = q.front();
        q.pop();

        if(visited[curr.x][curr.y] && curr.x != start.x && curr.y != start.y) continue;

        //If it ever finds an unknown tile, assume it's the same section'
        if(world.count(curr.x) == 0 && world[curr.x].count(curr.y) == 0)
        {
            cout << total << " : unknown" << endl;
            return false;
        }

        if(curr.x == end.x && curr.y == end.y)
        {
            cout << total << " : success" << endl;
            return false;
        }

        const static vector<MazePoint> exits = {MazePoint{0, 1}, MazePoint{0, -1}, MazePoint{1, 0}, MazePoint{-1, 0}};
        for(auto& p : exits)
        {
            MazePoint to = curr + p;
            if(!_visited[to.x][to.y])
            {
                q.push(to);
                _visited[to.x][to.y] = true;
            }
        }
    }

    cout << total << " : fail" << endl;
    return true;
}

AdvancedPlayerMove Spartacus::move(const AdvancedMapTile* surroundings,                //Const pointer to local area
                            const uint& area_width, const uint& area_height,    //Size of local area
                            const uint& loc_x, const uint& loc_y)
{
    AdvancedPlayerMove out = bookkeeping(surroundings, area_width, area_height, loc_x, loc_y);
    if(out.attemptedMove != AdvancedPlayerMove::Move::NOOP) 
    {
        //cerr << "Spartacus bookkeeping move: " << (int)out.attemptedMove << endl;
        lastMove = out;
        return out;
    }

    static vector<MazePoint> moves;
    getValidDirections(location, moves);

    vector<MazePoint> goodMoves;
    for(auto& p : moves)
    {
        MazePoint to = location+p;
        if(!deadEnd[to.x][to.y] && !visited[to.x][to.y])
            goodMoves.push_back(to);
    }

    //cerr << moves.size() << " moves from here, " << goodMoves.size() << " are good" << endl;

    /*if(pointEnclosed(location, exitLoc) || pointEnclosed(exitLoc, location))
    {
        //cerr << "Enclosed!" << endl;
        goodMoves.clear();
    }*/

    if(goodMoves.size() > 1)
    {
        intersections.push(location);
    }
    if(goodMoves.size() > 0)
    {
        out.attemptedMove = AdvancedPlayerMove::Move::MOVETO;
        out.destination = closestToExit(goodMoves) - location;
        //cerr << "Move to " << out.destination.x << ", " << out.destination.y << endl;
    }
    else if(intersections.size())
    {
        do
        {
            MazePoint moveto = intersections.top();
            getValidDirections(moveto, moves);

            //cerr << "Jump back to " << moveto.x << ", " << moveto.y << endl;
            goodMoves.clear();
            for(auto& p : moves)
            {
                MazePoint to = moveto+p;
                if(!deadEnd[to.x][to.y] && !visited[to.x][to.y])
                    goodMoves.push_back(to);
            }

            //cerr << goodMoves.size() << " good moves" << endl;

            /*if(pointEnclosed(location, exitLoc) || pointEnclosed(exitLoc, location))
            {
                //cerr << "Jump to enclosed! " << endl;
                goodMoves.clear();
            }*/

            if(goodMoves.size() < 2)
            {
                //cin.get();
                if(intersections.size())
                    intersections.pop();
                else
                {
                    out.attemptedMove = AdvancedPlayerMove::Move::NOOP;
                    return out;
                }
            }
        }while(goodMoves.size() == 0);
        
        out.attemptedMove = AdvancedPlayerMove::Move::MOVETO;
        out.destination = closestToExit(goodMoves) - location;
        //cerr << "Jump to " << out.destination.x << ", " << out.destination.y << endl;
    }

    //cin.get();

    lastMove = out;
    return out;
}













/*
ostream& operator <<(ostream& out, queue<MazePoint> q)
{
    while(q.size())
    {
        out << "(" << q.front().x << ", " << q.front().y << ") ";
        q.pop();
    }
    return out;
}

int main()
{
    cout << "Trivial cases..." << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(0, 10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(0, -10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(10, 0)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(-10, 0)) << endl;

    cout << endl << "45 degrees..." << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(10, 10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(10, -10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(-10, -10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(-10, 10)) << endl;

    cout << endl << "Other..." << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(10, 5)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(5, 10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(-5, 10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(-10, 5)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(-10, -5)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(-5, -10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(5, -10)) << endl;
    cout << Bresenham(MazePoint(0, 0), MazePoint(10, -5)) << endl;

    cout << Bresenham(MazePoint(0, 0), MazePoint(11, 7)) << endl;
}
*/