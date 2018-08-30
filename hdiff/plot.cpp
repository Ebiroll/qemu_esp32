#include "iostream"
#include <nana/gui.hpp>
#include "plot.h"
namespace nana
{
namespace plot
{

plot::plot( window parent )
    : myParent( parent )
{
    // arrange for the plot to be updated when needed
    myDrawing = new drawing( parent );
    myDrawing->draw([this](paint::graphics& graph)
    {
        if( ! myTrace.size() )
            return;

        // calculate scaling factors
        // so plot will fit
        CalcScale(
            graph.width(),
            graph.height() );

        myAxis->update( graph );

        // draw all the traces
        for( auto t : myTrace )
        {
            t->update( graph );
        }
    });

    myAxis = new axis( this );
}

trace& plot::AddPointTrace()
{
    trace * t = new trace();
    t->Plot( this );
    t->points();
    myTrace.push_back( t );
    return *t;
}

void plot::CalcScale( int w, int h )
{
    w *= 0.9;
    h *= 0.95;

    int maxCount = 0;
    myTrace[0]->bounds( myMinY, myMaxY );
    for( auto& t : myTrace )
    {
        if( t->size() > maxCount )
            maxCount = t->size();
        int tmin, tmax;
        t->bounds( tmin, tmax );
        if( tmin < myMinY )
            myMinY = tmin;
        if( tmax > myMaxY )
            myMaxY = tmax;
    }
    if( ! maxCount )
        return;
    myXinc = (float)w / maxCount;
    if( myMaxY == myMinY )
        myScale = 1;
    else
        myScale = 0.9 * h / ( myMaxY - myMinY );
    myXOffset = 0.05 * w;
    myYOffset = h + myScale * myMinY;
}
void trace::set( const std::vector< double >& y )
{
    if( myType != eType::plot )
        throw std::runtime_error("nanaplot error: plot data added to non plot trace");

    myY.clear();
    for( double s : y )
        myY.push_back( s );
    //myY = y;

    std::cout << "plot::trace::set " << myY.size() << "\n";
}
void trace::add( double y )
{
    if( myType != eType::realtime )
        throw std::runtime_error("nanaplot error: realtime data added to non realtime trace");
    myY[ myRealTimeNext++ ] = y;
    if( myRealTimeNext >= (int)myY.size() )
        myRealTimeNext = 0;

    myPlot->update();
}

void trace::add( double x, double y )
{
    if( myType != eType::point )
        throw std::runtime_error("nanaplot error: point data added to non point type trace");
    myX.push_back( x );
    myY.push_back( y );
}

void trace::bounds( int& tmin, int& tmax )
{
    if( ! myY.size() )
        return;
    tmin = myY[0];
    tmax = tmin;
    for( auto y : myY )
    {
        if( y < tmin )
            tmin = y;
        if( y > tmax )
            tmax = y;
    }
    tmin--;
    tmax++;
}

void trace::update( paint::graphics& graph )
{
    bool first = true;
    float x    = myPlot->XOffset();
    float xinc = myPlot->xinc();
    double s   = myPlot->Scale();
    int yOffset = myPlot->YOffset();
    double prev;


    switch( myType )
    {
    case eType::plot:

        // loop over data points
        for( auto y : myY )
        {
            // scale
            double ys =  myPlot->Y2Pixel( y );

            if( first )
            {
                first = false;
                prev = ys;
                continue;
            }

            // draw line from previous to this data point
            graph.line(
                point(x, prev),
                point(x+xinc, ys),
                myColor);

            x += xinc;
            prev = ys;

        }
        break;

    case eType::point:

        for( int k = 0; k < myX.size(); k++ )
        {
            double x = myPlot->XOffset() + xinc * myX[k];
            graph.rectangle(
                rectangle{ x-5,  myPlot->Y2Pixel( myY[ k ] )-5,
                           10, 10 },
                false,
                myColor );
        }
        break;

    case eType::realtime:

    {

        // loop over data points

        // they are stored in a circular buffer
        // so we have to start with the oldest data point
        int yidx = myRealTimeNext;
        do
        {
            double y = myPlot->Y2Pixel( myY[ yidx ] );

            // the next data point
            // with wrap-around if the end of the vector is reached
            yidx++;
            if( yidx >= (int)myY.size() )
                yidx = 0;

            if( first )
            {
                first = false;
                prev = y;
                continue;
            }
            // draw line from previous to this data point
            graph.line(
                point( x, prev ),
                point( x+xinc, y ),
                myColor);

            x += xinc;
            prev = y;

        }

        // check for end of circular buffer
        // ( most recent point )
        while( yidx != myRealTimeNext );

    }
    break;
    }
}

axis::axis( plot * p )
    : myPlot( p )
{
    myLabelMin = new label( myPlot->parent(),  rectangle{ 10, 10, 50, 50 } );
    myLabelMin->caption("test");
    myLabelMax = new label( myPlot->parent(),  rectangle{ 10, 10, 50, 50 } );
    myLabelMax->caption("test");
}

void axis::update( paint::graphics& graph )
{
    int mn = 10 * ( myPlot->minY() / 10 );
    std::stringstream ss;
    ss << mn;
    myLabelMin->caption(ss.str());
    myLabelMin->move( 5,  myPlot->Y2Pixel( mn ) );

    int mx = 10 * ( myPlot->maxY() / 10 );
    ss.str("");
    ss << mx;
    myLabelMax->caption(ss.str());
    myLabelMax->move( 5,   myPlot->Y2Pixel( mx ) - 15 );

    graph.line( point(2, myPlot->Y2Pixel( mn )),
                point(5, myPlot->Y2Pixel( mn )),
                colors::black );
    graph.line( point( 2, myPlot->Y2Pixel( mn ) ),
                point( 2, myPlot->Y2Pixel( mx ) ),
                colors::black );
    graph.line( point(2, myPlot->Y2Pixel( mx )),
                point(5, myPlot->Y2Pixel( mx )),
                colors::black );
}


}
}
