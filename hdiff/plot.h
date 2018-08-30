#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui.hpp>

namespace nana
{
namespace plot
{
class plot;

/// Single trace to be plotted
class trace
{
public:

    trace()
        : myType( eType::plot )
    {

    }
    /** Convert trace to real time operation
        @param[in] w number of data points to display

        Data points older than w scroll off the left edge of the plot and are lost
    */
    void realTime( int w )
    {
        myType = eType::realtime;
        myRealTimeNext = 0;
        myY.clear();
        myY.resize( w );
    }

    /** Convert trace to point operation for scatter plots */
    void points()
    {
        myType = eType::point;
        myY.clear();
        myX.clear();
    }

    /** set static data
        @param[in] y vector of data points to display

        Replaces any existing data.  Plot is NOT refreshed.
        An exception is thrown when this is called
        for a trace that has been converted to real time.
    */
    void set( const std::vector< double >& y );

    /** add new value to real time data
        @param[in] y the new data point

        An exception is thrown when this is called
        for a trace that has not been converted to real time
    */
    void add( double y );

    /** add point to point trace
        @param[in] x
        @param[in] y

        An exception is thrown when this is called
        for a trace that has not been converted to points
    */

    void add( double x, double y );

    /// set color
    void color( const colors & clr )
    {
        myColor = clr;
    }

    /// set plot where this trace will appear
    void Plot( plot * p )
    {
        myPlot = p;
    }

    int size()
    {
        return (int) myY.size();
    }

    void bounds( int& min, int& max );

    /// draw
    void update( paint::graphics& graph );

private:
    plot * myPlot;
    std::vector< double > myX;
    std::vector< double > myY;
    colors myColor;
    int myRealTimeNext;
    enum class eType
    {
        plot,
        realtime,
        point
    } myType;
};

class axis
{
public:
    axis( plot * p );

    ~axis()
    {
        delete myLabelMin;
        delete myLabelMax;
    }

    /// draw
    void update( paint::graphics& graph );

private:
    plot * myPlot;
    label * myLabelMin;
    label * myLabelMax;
};


/** 2D plotting */
class plot
{
public:

    /** CTOR
        @param[in parent window where plot will be drawn
    */
    plot( window parent );

    ~plot()
    {
        delete myAxis;
    }

    /** Add static trace
        @return reference to new trace

        The data in a static trace does not change
    */
    trace& AddStaticTrace()
    {
        trace * t = new trace();
        t->Plot( this );
        myTrace.push_back( t );
        return *t;
    }

    /** Add real time trace
        @param[in] w number of recent data points to display
        @return reference to new trace

        The data in a real time trace receives new values from time to time
        The display shows w recent values.  Older values scroll off the
        left hand side of the plot and disappear.
    */
    trace& AddRealTimeTrace( int w )
    {
        trace * t = new trace();
        t->Plot( this );
        t->realTime( w );
        myTrace.push_back( t );
        return *t;
    }

    /** Add point trace
        @return reference to new trace

        A static trace for scatter plots
    */
    trace& AddPointTrace();

    int Y2Pixel( double y ) const
    {
        return myYOffset - myScale * y;
    }

    float xinc()
    {
        return myXinc;
    }
    int minY()
    {
        return myMinY;
    }
    int maxY()
    {
        return myMaxY;
    }
    double Scale()
    {
        return myScale;
    }
    int XOffset()
    {
        return myXOffset;
    }
    int YOffset()
    {
        return myYOffset;
    }
    window parent()
    {
        return myParent;
    }

    void update()
    {
        API::refresh_window( myParent );
    }

    void debug()
    {
        for( auto t : myTrace )
        {
            std::cout << "debugtsize " << t->size() << "\n";
        }
    }

private:

    ///window where plot will be drawn
    window myParent;

    drawing * myDrawing;

    axis * myAxis;

    /// plot traces
    std::vector< trace* > myTrace;

    float myXinc;
    int myMinY, myMaxY;
    double myScale;
    int myXOffset;
    int myYOffset;

    /** calculate scaling factors so plot will fit in window client area
        @param[in] w width
        @param[in] h height
    */
    void CalcScale( int w, int h );

};

}
}
